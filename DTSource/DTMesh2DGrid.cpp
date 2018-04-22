// Part of DTSource. Copyright 2004-2017. David Adalsteinsson.
// see http://www.visualdatatools.com/DTSource/license.html for more information.

#include "DTMesh2DGrid.h"

#include "DTError.h"
#include "DTIntArray.h"
#include "DTDoubleArray.h"
#include "DTRegion2D.h"
#include "DTShiftScale2D.h"
#include "DTUtilities.h"

DTMesh2DGrid::DTMesh2DGrid(ssize_t min,ssize_t nin)
: _m(0), _n(0), _origin(), _dx(1.0), _dy(1.0), maskDefined(false), mask()
{
	isEmpty = false;
	if (min<0 || nin<0) {
        DTErrorMessage("DTMeshGrid(m,n)","One of the size is negative.");
        isEmpty = true;
        return;
    }
    if (min*nin==0)
        min = nin = 0;
    _m = int(min);
    _n = int(nin);
}

DTMesh2DGrid::DTMesh2DGrid(DTPoint2D origin,double dxv,double dyv,ssize_t mv,ssize_t nv)
: _m(0), _n(0), _origin(), _dx(1.0), _dy(1.0), maskDefined(false), mask()
{
    isEmpty = false;
    if (mv<0 || nv<0) {
        DTErrorMessage("DTMeshGrid(origin,dx,dy,m,n)","One of the size is negative.");
		isEmpty = true;
        return;
    }
    if (dxv<0 || dyv<0) {
        DTErrorMessage("DTMeshGrid(origin,dx,dy,m,n)","One of the size is negative.");
		isEmpty = true;
        return;
    }
    if (dxv==0.0 || dyv==0.0) {
        DTErrorMessage("DTMeshGrid(origin,dx,dy,m,n)","One of the size is zero.");
 		isEmpty = true;
		return;
    }
    if (mv*nv==0)
        mv = nv = 0;
    _m = int(mv);
    _n = int(nv);
    _origin = origin;
    _dx = dxv;
    _dy = dyv;
}

DTMesh2DGrid::DTMesh2DGrid(DTPoint2D origin,double h)
: _m(0), _n(0), _origin(), _dx(1.0), _dy(1.0), maskDefined(false), mask()
{
    isEmpty = false;
    if (h<0) {
        DTErrorMessage("DTMeshGrid(origin,h)","Step size is negative.");
 		isEmpty = true;
		return;
    }
    if (h==0) {
        DTErrorMessage("DTMeshGrid(origin,h)","Step size is zero.");
		isEmpty = true;
        return;
    }
    _origin = origin;
    _dx = _dy = h;
}

DTMesh2DGrid::DTMesh2DGrid(DTPoint2D origin,double dxv,double dyv)
: _m(0), _n(0), _origin(), _dx(1.0), _dy(1.0), maskDefined(false), mask()
{
	isEmpty = false;
	if (dxv<=0 || dyv<=0) {
        DTErrorMessage("DTMeshGrid(origin,dx,dy)","One of the size is negative.");
		isEmpty = true;
        return;
    }
    _origin = origin;
    _dx = dxv;
    _dy = dyv;
}

DTMesh2DGrid::DTMesh2DGrid(DTPoint2D origin,double dxv,double dyv,const DTMask &theMask)
: _m(0), _n(0), _origin(), _dx(1.0), _dy(1.0), maskDefined(false), mask()
{
	isEmpty = false;
    if (theMask.o()>1) {
        DTErrorMessage("DTMeshGrid(origin,dx,dy,mask)","Passing in a 3D Mask.");
 		isEmpty = true;
		return;
    }

    _m = theMask.m();
    _n = theMask.n();
    _origin = origin;
    _dx = dxv;
    _dy = dyv;
    maskDefined = true;
    mask = theMask;
}

DTMask DTMesh2DGrid::Mask(void) const
{
    if (!maskDefined) {
        DTErrorMessage("DTMesh2DGrid::Mask","Mask is not defined");
    }
    return mask;
}

void DTMesh2DGrid::pinfo(void) const
{
    if (dx()==dy()) {
        std::cerr << m() << "x" << n() << " mesh, origin = (" << Origin().x << "," << Origin().y << "), h = " << dx();
    }
    else {
        std::cerr << m() << "x" << n() << " mesh, origin = (" << Origin().x << "," << Origin().y << "), dx = " << dx() << ", dy = " << dy();
    }
    if (MaskDefined()) {
        std::cerr << ". Mask = " << Mask().NumberOfPoints() << " points.";
    }
    std::cerr << std::endl << std::flush;
}

bool operator==(const DTMesh2DGrid &A,const DTMesh2DGrid &B)
{
    if (A.MaskDefined()!=B.MaskDefined())
        return false;
    if (A.m()!=B.m() || A.n()!=B.n())
        return false;
    if (A.dx()!=B.dx() || A.dy()!=B.dy())
        return false;
    if (A.Origin()!=B.Origin())
        return false;
    if (A.MaskDefined()) {
        return (A.Mask()==B.Mask());
    }
    else {
        return true;
    }
}

bool operator!=(const DTMesh2DGrid &A,const DTMesh2DGrid &B)
{
    return (!(A==B));
}

bool SameExceptForMask(const DTMesh2DGrid &A,const DTMesh2DGrid &B)
{
    if (A.m()!=B.m() || A.n()!=B.n())
        return false;
    if (A.dx()!=B.dx() || A.dy()!=B.dy())
        return false;
	DTPoint2D AO = A.Origin();
	DTPoint2D BO = B.Origin();
	if (fabs(AO.x-BO.x)>1e-10*A.dx() || fabs(AO.y-BO.y)>1e-10*A.dy()) return false; // Allow for a small rounding error shift.
		
    return true;
}

DTRegion2D BoundingBox(const DTMesh2DGrid &v)
{
    if (v.m()==0)
        return DTRegion2D();

    DTPoint2D origin = v.Origin();

    return DTRegion2D(origin,DTPoint2D(origin.x+v.dx()*(v.m()-1),
                                       origin.y+v.dy()*(v.n()-1)));
}

DTMesh2DGrid ChangeSize(const DTMesh2DGrid &Grid,ssize_t m,ssize_t n)
{
    return DTMesh2DGrid(Grid.Origin(),Grid.dx(),Grid.dy(),m,n);
}

DTMesh2DGrid ApplyMask(const DTMesh2DGrid &Grid,const DTMask &mask)
{
    return DTMesh2DGrid(Grid.Origin(),Grid.dx(),Grid.dy(),mask);
}

DTMesh2DGrid RemoveMask(const DTMesh2DGrid &Grid)
{
    return DTMesh2DGrid(Grid.Origin(),Grid.dx(),Grid.dy(),Grid.m(),Grid.n());
}

DTMesh2DGrid RemoveSize(const DTMesh2DGrid &grid)
{
    return DTMesh2DGrid(grid.Origin(),grid.dx(),grid.dy());
}

DTMesh2DGrid CellCenters(const DTMesh2DGrid &Grid)
{
    DTPoint2D origin = Grid.Origin();
    double dx = Grid.dx();
    double dy = Grid.dy();
    origin.x += dx*0.5;
    origin.y += dy*0.5;
    
    return DTMesh2DGrid(origin,dx,dy,(Grid.m() ? Grid.m()-1 : 0),(Grid.n() ? Grid.n()-1 : 0));
}

DTMesh2DGrid GridShift(const DTMesh2DGrid &grid,int i,int j)
{
    DTPoint2D newPoint = grid.Origin();
    double dx = grid.dx();
    double dy = grid.dy();
    newPoint.x += i*dx;
    newPoint.y += j*dy;
    
    return DTMesh2DGrid(newPoint,dx,dy,grid.m(),grid.n());
}

DTMesh2DGrid Crop(const DTMesh2DGrid &grid,const DTRegion2D &B)
{
	if (grid.IsEmpty()) return grid;
	
	double dx = grid.dx();
	double dy = grid.dy();
	ssize_t m = grid.m();
	ssize_t n = grid.n();
	
    int shiftM = 0,shiftN = 0;
	DTPoint2D O = grid.Origin();
	double xz = O.x;
	if (xz<B.xmin) {
        shiftM = int(ceil((B.xmin-O.x)/dx-1e-10));
        xz = O.x + shiftM*dx;
        if (m>0) m-=shiftM;
    }
	double yz = O.y;
	if (yz<B.ymin) {
        shiftN = int(ceil((B.ymin-O.y)/dy-1e-10));
        yz = O.y + shiftN*dy;
        if (n>0) n-=shiftN;
    }
	
	if (m==0 && n==0) {
        m = int(floor((B.xmax-xz)/dx+1e-10));
	}
    else {
        if (xz+dx*(m-1)>B.xmax)
            m = int(floor((B.xmax-xz)/dx+1e-10))+1; // trim
        if (yz+dy*(n-1)>B.ymax)
            n = int(floor((B.ymax-yz)/dy+1e-10))+1;
    }
    if (m<0 || n<0)
        return DTMesh2DGrid();
    
    return DTMesh2DGrid(DTPoint2D(xz,yz),dx,dy,m,n);
}

DTMesh2DGrid Region(const DTMesh2DGrid &grid,const DTRange &iRange,const DTRange &jRange)
{
    double dx = grid.dx();
    double dy = grid.dy();
    return DTMesh2DGrid(grid.Origin()+DTPoint2D(dx*iRange.start,dx*jRange.start),dx,dy,iRange.length,jRange.length);
}

DTMesh2DGrid operator*(const DTShiftScale2D &s,const DTMesh2DGrid &grid)
{
	if (s.ScaleX()>0 && s.ScaleY()>0) {
		DTMesh2DGrid toReturn = DTMesh2DGrid(s*grid.Origin(),grid.dx()*s.ScaleX(),grid.dy()*s.ScaleY(),grid.m(),grid.n());
		if (grid.MaskDefined())
			toReturn = ApplyMask(toReturn,grid.Mask());
		return toReturn;
	}
	else {
		// Aversion to copying data.
		DTErrorMessage("DTShiftScale2D*DTMesh2DGrid","Flips are not supported.");
		return DTMesh2DGrid();
	}
}

void Read(const DTDataStorage &input,const std::string &name,DTMesh2DGrid &toReturn)
{
    DTDoubleArray base;

    base = input.ReadDoubleArray(name);
    if (base.Length()==0) {
        toReturn = DTMesh2DGrid();
        return;
    }
    else if (base.Length()!=4) {
        DTErrorMessage("ReadFromArray(DTMesh2DGrid)","Incorrect size array.");
        toReturn = DTMesh2DGrid();
        return;
    }

    DTIntArray size;
    std::string sizeName = name+"_size";
    if (input.Contains(sizeName))
        size = input.ReadIntArray(sizeName);
    if (size.Length()>0 && size.Length()!=2) {
        DTErrorMessage("ReadFromArray(DTMesh2DGrid)","Incorrect size array.");
        toReturn = DTMesh2DGrid();
        return;
    }

    if (size.Length()==0)
        toReturn = DTMesh2DGrid(DTPoint2D(base(0),base(1)),base(2),base(3),0,0);
    else
        toReturn = DTMesh2DGrid(DTPoint2D(base(0),base(1)),base(2),base(3),
                                size(0),size(1));

    if (input.Contains(name+"_mask")) {
        DTMask mask;
        Read(input,name+"_mask",mask);
        toReturn = ApplyMask(toReturn,mask);
    }
}

void Write(DTDataStorage &output,const std::string &name,const DTMesh2DGrid &theVar)
{
    // First save [xzero yzero dx dy]
    DTMutableDoubleArray base(4);
    base(0) = theVar.Origin().x;
    base(1) = theVar.Origin().y;
    base(2) = theVar.dx();
    base(3) = theVar.dy();

    // Save the size if it's not zero.
    if (theVar.m()) {
        DTMutableIntArray size(2);
        size(0) = int(theVar.m());
        size(1) = int(theVar.n());
        output.Save(size,name+"_size");
    }
    
    if (theVar.MaskDefined()) {
        Write(output,name+"_mask",theVar.Mask());
    }

    output.Save(base,name);
}

void WriteOne(DTDataStorage &output,const std::string &name,const DTMesh2DGrid &toWrite)
{
    Write(output,name,toWrite);
    Write(output,"Seq_"+name,"2D Mesh Grid");
    output.Flush();
}

void WriteNoSize(DTDataStorage &output,const std::string &name,const DTMesh2DGrid &theVar)
{
    // First save [xzero yzero dx dy]
    DTMutableDoubleArray base(4);
    base(0) = theVar.Origin().x;
    base(1) = theVar.Origin().y;
    base(2) = theVar.dx();
    base(3) = theVar.dy();

    output.Save(base,name);
}
