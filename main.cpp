#include "DTArguments.h"
#include "DTSaveError.h"

#include "DTMatlabDataFile.h"

// Common utilities
#include "DTTimer.h"
#include "DTUtilities.h"
#include "DTDictionary.h"
#include "DTIntArray.h"
#include "DTError.h"
#include "DTMesh2D.h"
#include "DTFunction2D.h"
#include "DTSeriesMesh2D.h"
#include "DTTimer.h"
#include "DTDoubleArrayOperators.h"
#include <math.h>
#include <Eigen/Sparse>
#include <Eigen/Core>
#include <vector>
#include <boost/program_options.hpp>

namespace po = boost::program_options;

typedef Eigen::SparseMatrix<double> SpMat; // declares a column-major sparse matrix type of double
typedef Eigen::Triplet<double> T;


typedef struct grid
{
    DTMutableMesh2D f;  // rhs
    DTMutableMesh2D v;  // solution
}gridtype;

typedef struct OutputWrapper
{
    DTDoubleArray ResidualNorms;
    DTDoubleArray Times;
    OutputWrapper(DTDoubleArray _res, DTDoubleArray _times) : ResidualNorms(_res), Times(_times) {}
}MGOutputs;


double boundary_func(double x, double y)
{
    return 0;
//    return 3*x+5*y;
}

DTMutableDoubleArray getSparseSol(const DTMesh2D& f, double g(double, double))
{
    DTMesh2DGrid grid = f.Grid();
    double h = grid.dx();
    DTDoubleArray fData = f.DoubleData();

    int M = fData.m();
    int N = fData.n();
//    printf("grid dim: %dx%d\n", M, N);

    // Fill boundary condition
    DTMutableDoubleArray toReturn(M, N);
    double xzero = grid.Origin().x;
    double yzero = grid.Origin().y;
    double xm = xzero + (M-1)*h;
    double yn = yzero + (N-1)*h;
    // fill boundary rows
    for (int j = 0; j < N; j++) {
        double y = yzero + j*h;
        toReturn(0,j) = g(xzero, y);
        toReturn(M-1,j) = g(xm, y);
    }
    // fill boundary columns
    for (int i = 0; i < M; i++) {
        double x = xzero + i*h;
        toReturn(i,0) = g(x, yzero);
        toReturn(i,N-1) = g(x, yn);
    }

    // fill A and b;
    int ukn = (M-2)*(N-2); // unknowns in total
    int k = M-2;    // bandwidth for storage

    std::vector<T> coefficients;            // list of non-zeros coefficients
    Eigen::VectorXd bvec(ukn);                   // the right hand side-vector resulting from the constraints

    for (int i = 0; i < ukn; i++) {
        coefficients.push_back(T(i,i,4.0));
        if( i-k >= 0 ) coefficients.push_back(T(i,i-k,-1.0));
        if( i+k < ukn ) coefficients.push_back(T(i,i+k,-1.0));
        if( ( (i+1)%k != 0 ) && (i+1 < ukn) ) coefficients.push_back(T(i,i+1,-1.0));
        if( ( i%k != 0 ) && (i-1 >= 0) ) coefficients.push_back(T(i,i-1,-1.0));
    }

    SpMat A(ukn, ukn);
    A.setFromTriplets(coefficients.begin(), coefficients.end());

    // Fill in right hand side as a 2-D array
    DTMutableDoubleArray b(M-2, N-2);
    double h2 = h * h;
    for(int j = 0; j < N-2; j++){
        for(int i = 0; i < M-2; i++){
            b(i, j) = -h2 * fData(i+1,j+1);
        }
    }
    // Add boundary condition to rhs
    for(int i = 0; i < M-2; i++){
        b(i, 0) += toReturn(i+1, 0);
        b(i, N-3) += toReturn(i+1, N-1);    // double check the index
    }
    for(int j = 0; j < N-2; j++){
        b(0, j) += toReturn(0, j+1);
        b(M-3, j) += toReturn(M-1, j+1);    // double check too
    }

    for(int i = 0; i < ukn; i++){
        bvec[i] = b(i);
    }

    // Solving:
    Eigen::SimplicialCholesky<SpMat> chol(A);  // performs a Cholesky factorization of A
    Eigen::VectorXd x = chol.solve(bvec);         // use the factorization to solve for the given right hand side

    // Retrieve output
    int cnt = 0;
    for(int j = 1; j < N-1; j++){
        for(int i = 1; i < M-1; i++){
            toReturn(i, j) = x[cnt++];
        }
    }
    return toReturn;
}

void printMatrix(const DTDoubleArray &p)
{
    printf("Dimension: %dx%d\n", p.m(), p.n());
    for (int i = 0; i < p.m(); i++)
    {
        for (int j = 0; j < p.n(); j++)
        {
            printf("%.3f\t", p(i,j));
        }
        printf("\n");
    }
    printf("\n");
}


double calcNorm(const DTDoubleArray ref)
{
    return std::max<double>(Maximum(ref), - Minimum(ref));
}


void direct_solve(gridtype &p)
{
    auto u = p.v.DoubleData();
    auto fData = p.f.DoubleData();
    int N = p.v.n();
    int M = p.v.m();
    assert(M == N);
    assert(M % 2 == 1);
    double h2 = p.v.Grid().dx() * p.v.Grid().dx();
    double factor = 0.25;
    if(M == 3)
    {
        u(1, 1) = (u(0,1)+u(2,1)+u(1,0)+u(1,2)-fData(1,1)*h2) * factor;
    }else{
        printf("Error: Direct solver not defined for %dx%d matrices!\n", M, M);
    }
};

void relax(gridtype &p, int Niter, double omega)  // Jacobi iteration
{
    auto u = p.v.DoubleData();
    auto fData = p.f.DoubleData();
    int N = p.v.n();
    int M = p.v.m();
    assert(M == N);
    assert(M % 2 == 1);
    double nomega = 1 - omega;
    double h2 = p.v.Grid().dx() * p.v.Grid().dx();
    DTMutableDoubleArray u_old;
    double factor = 0.25;
    auto ptr_new = u.Pointer();
    for(int iter = 0; iter < Niter; iter++)
    {
        u_old = u.Copy();
        auto ptr = u_old.Pointer();
        for(int j = 1; j < N-1; j++)
        {
            for(int i = 1; i < M-1; i++)
            {
//                u(i + j*M) = u_old(i + j*M) * nomega +
//                        ((u_old(i-1 + j*M)+u_old(i+1 + j*M)+u_old(i + (j-1)*M)+u_old(i + (j+1)*M)-fData(i + j*M)*h2) * factor) * omega;
                *(ptr_new + i + j*M) = *(ptr + i + j*M) * nomega +
                             ((*(ptr + i-1 + j*M) + *(ptr + i+1 + j*M) + *(ptr + i + (j-1)*M) + *(ptr + i + (j+1)*M) - fData(i + j*M)*h2) * factor) * omega;
            }
        }
    }
}

void coarsen(const DTDoubleArray &fine, DTMutableDoubleArray &coarse) // restrict
{
    int M = coarse.m();
    int N = coarse.n();
    assert(M == N);
    assert(M % 2 == 1);

    double selfw = 1.0 / 4.0;
    double neighborw = 1.0 / 8.0;
    double cornerw = 1.0 / 16.0;
    coarse = 0;
    for(int i = 1; i < M-1; i++)
    {
        for(int j = 1; j < N-1; j++)
        {
            coarse(i, j) = fine(i*2, j*2) * selfw +
                    (fine(i*2-1, j*2) + fine(i*2+1, j*2) + fine(i*2, j*2-1) + fine(i*2, j*2+1)) * neighborw +
                    (fine(i*2-1, j*2-1) + fine(i*2+1, j*2-1) + fine(i*2+1, j*2-1) + fine(i*2+1, j*2+1)) * cornerw;
        }
    }
}

void refine(const DTDoubleArray &coarse, DTMutableDoubleArray &fine)  // interpolate
{
    int M = fine.m();
    int N = fine.n();
    assert(M == N);
    assert(M % 2 == 1);
    fine = 0;
    for(int j = 1; j < N-1; j++)
    {
        for(int i = 1; i < M-1; i++)
        {
            if( i % 2 == 0 && j % 2 == 0)
            {
                fine(i,j) = coarse(i/2, j/2);
            } else if (i % 2 == 0 && j % 2 == 1)
            {
                fine(i,j) = 0.5 * ( coarse(i/2, j/2) + coarse(i/2, j/2+1) );
            } else if (i % 2 == 1 && j % 2 == 0)
            {
                fine(i,j) = 0.5 * ( coarse(i/2, j/2) + coarse(i/2+1, j/2) );
            } else if (i % 2 == 1 && j % 2 == 1)
            {
                fine(i,j) = 0.25 * ( coarse(i/2, j/2) + coarse(i/2+1, j/2) + coarse(i/2, j/2+1) + coarse(i/2+1, j/2+1) );
            }
        }
    }
//    printf("fine grid:\n");
//    printMatrix(fine);
//    printf("coarse grid:\n");
//    printMatrix(coarse);
}

DTMutableDoubleArray residual(const gridtype &p)
{
    auto u = p.v.DoubleData();
    auto fData = p.f.DoubleData();
    int N = p.v.n();
    int M = p.v.m();
    assert(M == N);
    assert(M % 2 == 1);
    double h2 = p.v.Grid().dx() * p.v.Grid().dx();


    auto res = DTMutableDoubleArray(M, N);
    res = 0;
    double invh2 = 1.0 / h2;
    auto ptr = u.Pointer();
    auto ptr_res = res.Pointer();
    auto ptr_f = fData.Pointer();
    for(int j = 1; j < N-1; j++)
    {
        for(int i = 1; i < M-1; i++)
        {
//            res(i, j) = fData(i, j) - ( u(i-1, j) + u(i+1, j) + u(i, j-1) + u(i, j+1) - u(i, j) * 4.0) * invh2;
            *(ptr_res + i + j*M) = *(ptr_f + i + j*M) - ( *(ptr + i-1 + j*M) + *(ptr + i+1 + j*M) + *(ptr + i + (j-1)*M) + *(ptr + i + (j+1)*M) - *(ptr + i + j*M) * 4.0) * invh2;
        }
    }
    return res;
}

MGOutputs MultiGrid(gridtype &prob, int Nv, int Ndown, int Nup, double omega, int coarsest, bool pureJacobi = false)
{
    // Initialization
    int depth = int(log2(1.0f * (prob.f.DoubleData().m() - 1) / coarsest) + 0.5f);
    gridtype* Grids = new gridtype[depth + 1];
    Grids[0] = prob;

    // Allocate memory just once
    for(int d = 1; d <= depth; d++)
    {
        int newdim = (Grids[d-1].f.DoubleData().m() - 1) / 2 + 1;
        auto prevGrid = Grids[d-1].f.Grid();
        DTMutableDoubleArray dData(newdim, newdim);
        dData = 0;
        DTMesh2DGrid grid = DTMesh2DGrid(prevGrid.Origin(), prevGrid.dx() * 2.0, prevGrid.dy() * 2.0, dData.m(),dData.n());
        Grids[d].f = DTMutableMesh2D(grid, dData.Copy());
        Grids[d].v = DTMutableMesh2D(grid, dData.Copy());
    }


    // Do Nv V cycles
    DTTimer timer;
    DTMutableDoubleArray resnorm(Nv+1);
    DTMutableDoubleArray times(Nv+1);
    resnorm(0) = calcNorm(residual(Grids[0]));
    times(0) = 0;
    for(int iter = 0; iter < Nv; iter++)
    {
        double before = calcNorm(residual(Grids[0]));
        double lowest = -1;
        double time_singleV = 0;
        if (!pureJacobi)
        {
            // Sweep down
            for(int iDown = 0; iDown < depth; iDown++)
            {
//                auto before_refine = calcNorm(residual(Grids[iDown]));
                timer.Start();
                relax(Grids[iDown], Ndown, omega);  // Jacobi iterations before refinement
                time_singleV += timer.Stop();
//                auto after_refine = calcNorm(residual(Grids[iDown]));
//                printf("level %d:%.20f -> %.20f\n", iDown, before_refine,after_refine);
                auto res = residual(Grids[iDown]);
                auto next = Grids[iDown + 1].f.DoubleData();
                coarsen(res, next);
            }

            // Apply direct solver to the coarsest grid
            direct_solve(Grids[depth]);
            lowest = calcNorm(residual(Grids[depth]));

            // Sweep up
            for(int iUp = depth-1; iUp >= 0; iUp--)
            {
                auto prev = Grids[iUp + 1].v.DoubleData();
                auto cur = Grids[iUp].v.DoubleData();
                auto refined = DTMutableDoubleArray(cur.m(), cur.n());
                refine(prev, refined);
                cur += refined;
                timer.Start();
                relax(Grids[iUp], Nup, omega);  // Jacobi iterations after refinement
                time_singleV += timer.Stop();
                prev = 0;   // clear previous solution
            }
        }else{
            relax(Grids[0], 1, omega);
        }
        times(iter+1) = times(iter) + time_singleV;
        auto after = calcNorm(residual(Grids[0]));
//        printf("iteration %d: before=%.20f\tafter=%.20f\tlowest=%.9f\n", iter+1, before, after, lowest);
        resnorm(iter+1) = after;
    }
    delete[] Grids;
    return MGOutputs(resnorm, times);
}


int main(int argc,const char *argv[])
{
    // Parse program parameters
    po::options_description desc( "Allowed options" );
    desc.add_options()
            ( "help,h", "produce help message" )
            ( "Nv,v", po::value< int >()->default_value( 100 ), "number of V cycles to sweep")
            ( "Nbefore,b", po::value< int >()->default_value( 3 ), "number of Jacobi sweeps before refinement" )
            ( "Nafter,a", po::value< int >()->default_value( 3 ), "number of Jacobi sweeps after refinement" )
            ( "omega,o", po::value< double >()->default_value( 0.6 ), "relaxation parameter" )
            ( "coarsest,c", po::value< int >()->default_value( 2 ), "threshold dimension to use a direct solver" );


    po::positional_options_description _p;
    _p.add( "Nv", 1 );

    po::variables_map vm;
    po::store( po::command_line_parser( argc, argv ).options( desc ).positional( _p ).run(), vm );

    if ( vm.count( "help" ))
    {
        std::cout << desc << "\n";
        return 0;
    }

    int Nv = vm["Nv"].as< int >();
    int Ndown = vm["Nbefore"].as< int >();
    int Nup = vm["Nafter"].as< int >();
    double omega = vm["omega"].as< double >();
    int coarsest = vm["coarsest"].as< int >();

//    DTSetArguments(argc, argv);

    DTMatlabDataFile inputFile("Input.mat", DTFile::ReadOnly);
    // Read in the input variables.
    DTMesh2D f;
    Read(inputFile, "f", f);

//    DTMutableDoubleArray groundtruth = getSparseSol(f, boundary_func);

    DTMesh2DGrid grid = f.Grid();

    double h = grid.dx();
    double h2 = h * h;
    DTDoubleArray fData = f.DoubleData();

    int M = fData.m();
    int N = fData.n();
    if (M != N)
    {
        printf("Error: Input is not square matrix!\n");
        exit(1);
    }
    if (M % 2 == 0)
    {
        printf("Error: Input has even rows and columns!\n");
        exit(1);
    }

    // Set initial guess to all zeros
    DTMutableDoubleArray u(M, N);
    u = 0;

    // Set the boundary of u to the values of g
    double xzero = grid.Origin().x;
    double yzero = grid.Origin().y;
    double xm = xzero + (M-1)*h;
    double yn = yzero + (N-1)*h;
    // fill boundary rows
    for (int j = 0; j < N; j++) {
        double y = yzero + j*h;
        u(0,j) = boundary_func(xzero, y);
        u(M-1,j) = boundary_func(xm, y);
    }
    // fill boundary columns
    for (int i = 0; i < M; i++) {
        double x = xzero + i*h;
        u(i,0) = boundary_func(x, yzero);
        u(i,N-1) = boundary_func(x, yn);
    }


    gridtype problem;
    problem.f = DTMutableMesh2D(grid, fData.Copy());
    problem.v = DTMutableMesh2D(grid, u.Copy());
    auto output = MultiGrid(problem, Nv, Ndown, Nup, omega, coarsest, 0);

//    auto mgres = calcNorm(residual(problem));
//    problem.v = DTMutableMesh2D(grid, groundtruth.Copy());
//    auto gdres = calcNorm(residual(problem));
//    printf("MGres=%.20f\ngdres=%.20f\n", mgres, gdres);

    DTMatlabDataFile outputFile("Output.mat",DTFile::NewReadWrite);
    outputFile.Save(problem.v.DoubleData(), "Sol");
    outputFile.Save(output.ResidualNorms, "ResNorms");
    outputFile.Save(output.Times, "Times");
//    outputFile.Save(groundtruth, "Groundtruth");

    return 0;
}