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
#include "DTDoubleArrayOperators.h"
#include <math.h>
#include <Eigen/Sparse>
#include <Eigen/Core>
#include <vector>
#include <boost/program_options.hpp>

namespace po = boost::program_options;

typedef Eigen::SparseMatrix<double> SpMat; // declares a column-major sparse matrix type of double
typedef Eigen::Triplet<double> T;


DTMutableDoubleArray getSparseSol(const DTMesh2D& f, const DTFunction2D& g)
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
        toReturn(0,j) = g(0, y);
        toReturn(M-1,j) = g(xm, y);
    }
    // fill boundary columns
    for (int i = 0; i < M; i++) {
        double x = xzero + i*h;
        toReturn(i,0) = g(x, 0);
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

double calcError(const DTMutableDoubleArray ref, const DTMutableDoubleArray tar)
{
    DTMutableDoubleArray diff = ref - tar;
    double absmax = std::max(Maximum(diff), - Minimum(diff));
    return absmax;
}


typedef struct grid
{
    DTMutableMesh2D f;  // rhs
    DTMutableMesh2D v;  // solution
}gridtype;


void direct_sovle(gridtype &p)
{

};

void relax(gridtype &p, int Niter, double omega)  // sweep
{

}

void coarsen(const DTDoubleArray &fine, DTMutableDoubleArray &coarse) // restrict
{

}

void refine(const DTDoubleArray &coarse, DTMutableDoubleArray &fine)  // interpolate
{

}

DTMutableDoubleArray residual(gridtype &p)
{
    return DTMutableDoubleArray(1, 1);
}


void MultiGrid(gridtype &prob, int Nv, int Ndown, int Nup, double omega, int coarsest)
{
    // Initialization
    int depth = int(log2(1.0f * (prob.f.DoubleData().m() - 1) / coarsest) + 0.5f);
    gridtype* Grids = new gridtype[depth + 1];
    Grids[0] = prob;

    // Allocate space just once
    for(int d = 1; d <= depth; d++)
    {
        int newdim = (Grids[d-1].f.DoubleData().m() - 1) / 2 + 1;
        auto prevGrid = Grids[d-1].f.Grid();
        DTMutableDoubleArray dData(newdim, newdim);
        DTMesh2DGrid grid = DTMesh2DGrid(prevGrid.Origin(), prevGrid.dx() * 2.0, prevGrid.dy() * 2.0, dData.m(),dData.n());
        Grids[d].f = DTMutableMesh2D(grid, dData.Copy());
        Grids[d].v = DTMutableMesh2D(grid, dData.Copy());
    }

    // Do Nv V cycles
    for(int iter = 0; iter < Nv; iter++)
    {
        // Sweep down
        for(int iDown = 0; iDown < depth; iDown++)
        {
            relax(Grids[iDown], Ndown, omega);  // Jacobi iterations before refinement
            auto res = residual(Grids[iDown]);
            auto next = Grids[iDown + 1].f.DoubleData();
            coarsen(res, next);
        }

        // Apply direct solver to the coarsest grid
        direct_sovle(Grids[depth]);

        // Sweep up
        for(int iUp = depth-1; iUp >= 0; iUp--)
        {
            auto prev = Grids[iUp + 1].v.DoubleData();
            auto cur = Grids[iUp].v.DoubleData();
            auto refined = prev.Copy();
            refine(prev, refined);
            cur += refined;
            relax(Grids[iUp], Nup, omega);  // Jacobi iterations after refinement
        }
    }

    delete[] Grids;
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
            ( "omega,o", po::value< float >()->default_value( 1.5 ), "relaxation parameter" )
            ( "coarsest,c", po::value< int >()->default_value( 8 ), "threshold dimension to use a direct solver" );


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
    double omega = vm["omega"].as< float >();
    int coarsest = vm["coarsest"].as< int >();

//    DTSetArguments(argc, argv);

    DTMatlabDataFile inputFile("Input.mat",DTFile::ReadOnly);
    // Read in the input variables.
    DTMesh2D f;
    Read(inputFile, "f", f);
    DTFunction2D g;
    Read(inputFile, "g", g);

    DTMutableDoubleArray groundtruth = getSparseSol(f, g);

    DTMesh2DGrid grid = f.Grid();
//    DTMatlabDataFile outputFile("Output.mat",DTFile::NewReadWrite);
//    DTSeriesMesh2D computed(outputFile,"Var");


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
        u(0,j) = g(0, y);
        u(M-1,j) = g(xm, y);
    }
    // fill boundary columns
    for (int i = 0; i < M; i++) {
        double x = xzero + i*h;
        u(i,0) = g(x, 0);
        u(i,N-1) = g(x, yn);
    }


    gridtype problem;
    problem.f = DTMutableMesh2D(grid, fData.Copy());
    problem.v = DTMutableMesh2D(grid, u.Copy());
    MultiGrid(problem, Nv, Ndown, Nup, omega, coarsest);


//    DTMatlabDataFile outputErrorFile("Error.mat", DTFile::NewReadWrite);
//    DTMutableDoubleArray Errors(Niter / stride + 1);
//    int cnt = 0;
//    Errors(0) = calcError(groundtruth, u);
//    printf("initial error:%4f\n", Errors(0));

//    double t = 0;
//    computed.Add(DTMesh2D(grid,u),t); // Saves the time value to disk


    clock_t t_before = clock();
    // Iterate, and increment t
//    for(int iter = 0; iter < Niter; iter++){
//
//        // update red
//        for(int j = 1; j < N-1; j++){
//            if (j % 2 == 0){
//                for(int i = 1; i < M-1; i+=2){
//                    u(i, j) = u(i, j) * nomega + ((u(i-1,j)+u(i+1,j)+u(i,j-1)+u(i,j+1)+fData(i,j)*h2) / 4.0) * omega;
//                }
//            }else{
//                for(int i = 2; i < M-1; i+=2){
//                    u(i, j) = u(i, j) * nomega + ((u(i-1,j)+u(i+1,j)+u(i,j-1)+u(i,j+1)+fData(i,j)*h2) / 4.0) * omega;
//                }
//            }
//        }
//    }
//    outputErrorFile.Save(Errors, "errors");
    // The following variables are only needed if you use DataTank to read the output
//    outputErrorFile.Save("NumberList", "Seq_errors");

    return 0;
}