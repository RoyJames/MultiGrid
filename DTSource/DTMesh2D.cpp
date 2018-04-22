// Part of DTSource. Copyright 2004-2017. David Adalsteinsson.
// see http://www.visualdatatools.com/DTSource/license.html for more information.

#include "DTMesh2D.h"

#include "DTError.h"
#include "DTRegion1D.h"
#include "DTRegion2D.h"
#include "DTArrayConversion.h"
#include "DTShiftScale2D.h"
#include "DTDoubleArrayOperators.h"
#include "DTFloatArrayOperators.h"
#include "DTUtilities.h"

DTMesh2D::DTMesh2D(const DTMesh2DGrid &gr,const DTFloatArray &input)
: _grid(), _floatData(), _doubleData()
{
    if (input.IsEmpty()) return;
    
    if (input.o()!=1) {
        DTErrorMessage("DTMesh(Grid,Array)","Array is three dimensional.");
        return;
    }
    
    if (gr.m()!=0 && (gr.m()!=input.m() || gr.n()!=input.n())) {
        DTWarningMessage("DTMesh(Grid,Array)","Grid size is incompatible.");
    }
    
    _grid = gr;
    if (gr.m()!=input.m() || gr.n()!=input.n()) {
        _grid = ChangeSize(gr,input.m(),input.n());
    }
    _floatData = input;
}

DTMesh2D::DTMesh2D(const DTMesh2DGrid &gr,const DTDoubleArray &input)
: _grid(), _floatData(), _doubleData()
{
    if (input.IsEmpty()) return;

    if (input.o()!=1) {
        DTErrorMessage("DTMesh(Grid,Array)","Array is three dimensional.");
        return;
    }
    
    if (input.n()==1 && gr.MaskDefined() && gr.Mask().NumberOfPoints()==input.m()) {
        // Flattened array
        DTIntArray offsets = gr.Mask().Offsets();
        DTMutableDoubleArray values(gr.m(),gr.n());
        ssize_t i;
        ssize_t howMany = offsets.Length();
        values = 0.0;
        for (i=0;i<howMany;i++)
            values(offsets(i)) = input(i);
        _doubleData = values;
    }
    else if (gr.m()!=0 && (gr.m()!=input.m() || gr.n()!=input.n())) {
        DTWarningMessage("DTMesh(Grid,Array)","Grid size is incompatible.");
        _doubleData = input;
    }
    else {
        _doubleData = input;
    }
    
    _grid = gr;
    if (gr.m()!=_doubleData.m() || gr.n()!=_doubleData.n()) {
        _grid = ChangeSize(gr,input.m(),input.n());
    }
}

DTDoubleArray DTMesh2D::DoubleData(void) const
{
    if (_floatData.NotEmpty()) {
        DTErrorMessage("DTMesh2D::DoubleData","Array saved as float.");
        return DTDoubleArray();
    }
    else {
        return _doubleData;
    }
}

DTFloatArray DTMesh2D::FloatData(void) const
{
    if (_doubleData.NotEmpty()) {
        DTErrorMessage("DTMesh2D::FloatData","Array saved as double.");
        return DTFloatArray();
    }
    else {
        return _floatData;
    }
}

void DTMesh2D::pinfo(void) const
{
    if (IsEmpty()) {
        std::cerr << "Empty\n";
        return;
    }
        
    if (_grid.dx()==_grid.dy()) {
        std::cerr << m() << "x" << n() << " mesh, origin = (" << _grid.Origin().x << "," << _grid.Origin().y << "), h = " << _grid.dx();
    }
    else {
        std::cerr << m() << "x" << n() << " mesh, origin = (" << _grid.Origin().x << "," << _grid.Origin().y << "), dx = " << _grid.dx() << ", dy = " << _grid.dy();
    }
    
    if (FloatPrecision()) {
        std::cerr << ", Float";
    }
    else {
        std::cerr << ", Double";
    }

    if (_grid.MaskDefined()) {
        std::cerr << ", Mask = " << _grid.Mask().NumberOfPoints() << " points";
    }
    
    std::cerr << std::endl << std::flush;
}

bool CompatibleMeshes(const DTMesh2D &A,const DTMesh2D &B)
{
    if (A.FloatPrecision()!=B.FloatPrecision())
        return false;
	if (A.Grid()!=B.Grid())
		return false;
    return true;
}

bool operator==(const DTMesh2D &A,const DTMesh2D &B)
{
	if (A.Grid()!=B.Grid())
		return false;
	if (A.FloatPrecision()!=B.FloatPrecision())
		return false;
	if (A.FloatPrecision()) {
		return (A.FloatData()==B.FloatData());
	}
	else {
		return (A.DoubleData()==B.DoubleData());
	}
}

bool operator!=(const DTMesh2D &A,const DTMesh2D &B)
{
	if (A.Grid()!=B.Grid())
		return true;
	if (A.FloatPrecision()!=B.FloatPrecision())
		return true;
	if (A.FloatPrecision()) {
		return (A.FloatData()!=B.FloatData());
	}
	else {
		return (A.DoubleData()!=B.DoubleData());
	}
}

DTMesh2D ApplyMask(const DTMesh2D &mesh,const DTMask &mask)
{
	if (mesh.m()==mask.m() && mesh.n()==mask.n() && 1==mask.o()) {
		if (mesh.FloatPrecision()) {
			return DTMesh2D(ApplyMask(mesh.Grid(),mask),mesh.FloatData());
		}
		else {
			return DTMesh2D(ApplyMask(mesh.Grid(),mask),mesh.DoubleData());
		}
	}
	else {
		DTErrorMessage("ApplyMask(DTMesh2D,DTMask)","Incompatible mask size.");
		return mesh;
	}
}

DTMesh2D RemoveMask(const DTMesh2D &mesh)
{
    if (mesh.FloatPrecision()) {
        return DTMesh2D(RemoveMask(mesh.Grid()),mesh.FloatData());
    }
    else {
        return DTMesh2D(RemoveMask(mesh.Grid()),mesh.DoubleData());
    }
}

DTMesh2D Crop(const DTMesh2D &mesh,const DTRegion2D &B)
{
	if (mesh.IsEmpty()) return mesh;

	DTMesh2DGrid grid = mesh.Grid();
	double dx = grid.dx();
	double dy = grid.dy();
	ssize_t m = grid.m();
	ssize_t n = grid.n();
	
    ssize_t shiftM = 0,shiftN = 0;
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
        return DTMesh2D();
	
	DTMesh2DGrid newGrid = Crop(grid,B);
	DTMesh2D toReturn;
	if (mesh.FloatPrecision())
        toReturn = DTMesh2D(newGrid,Region(mesh.FloatData(),DTRange(shiftM,m),DTRange(shiftN,n)));
	else
		toReturn = DTMesh2D(newGrid,SubArray(mesh.DoubleData(),shiftM,m,shiftN,n,0,1));
    
	return toReturn;
}

DTMesh2D operator*(const DTShiftScale2D &s,const DTMesh2D &mesh)
{
	if (s.ScaleX()>0 && s.ScaleY()>0) {
		if (mesh.FloatPrecision()) {
			return DTMesh2D(s*mesh.Grid(),mesh.FloatData());
		}
		else {
			return DTMesh2D(s*mesh.Grid(),mesh.DoubleData());
		}
	}
	else {
		// Aversion to copying data.
		DTErrorMessage("DTShiftScale2D*DTMesh2D","Flips are not supported.");
		return DTMesh2D();
	}
}

DTMutableMesh2D operator-(const DTMesh2D &A,const DTMesh2D &B)
{
	if (SameExceptForMask(A.Grid(),B.Grid())) {
		DTMesh2DGrid grid = A.Grid();
		if (A.FloatPrecision()==B.FloatPrecision()) {
			if (A.FloatPrecision()) {
				DTMutableFloatArray floatReturn(grid.m(),grid.n());
				float *returnF = floatReturn.Pointer();
				const float *AF = A.FloatData().Pointer();
				const float *BF = B.FloatData().Pointer();
				ssize_t len = floatReturn.Length();
				for (int i=0;i<len;i++) {
					returnF[i] = AF[i]-BF[i];
				}
				return DTMutableMesh2D(grid,floatReturn);
			}
			else {
				DTMutableDoubleArray doubleReturn(grid.m(),grid.n());
				double *returnD = doubleReturn.Pointer();
				const double *AD = A.DoubleData().Pointer();
				const double *BD = B.DoubleData().Pointer();
				ssize_t len = doubleReturn.Length();
				for (int i=0;i<len;i++) {
					returnD[i] = AD[i]-BD[i];
				}
				return DTMutableMesh2D(grid,doubleReturn);
			}
		}
		else {
			// Problem here is that you are subtracting double-float or float-double.
			// This could be the source of strange errors, where you think everything is double, but a single
			// mesh is float and ruins your resolution.  This is therefore considered an error.
			// You can convert a mesh to double or float relatively easily - ConvertToDouble(mesh) or ConvertToFloat(mesh)
			DTErrorMessage("DTMesh2D-DTMesh2D","Different number format");
			return DTMutableMesh2D();
		}
	}
	else {
		DTErrorMessage("DTMesh2D-DTMesh2D","Incompatible grids");
		return DTMutableMesh2D();
	}
}

DTMutableMesh2D operator+(const DTMesh2D &m,double v)
{
    if (m.DoublePrecision())
        return DTMutableMesh2D(m.Grid(),m.DoubleData()+v);
    else
        return DTMutableMesh2D(m.Grid(),m.FloatData()+float(v));
}

DTMutableMesh2D operator*(const DTMesh2D &m,double v)
{
    if (m.DoublePrecision())
        return DTMutableMesh2D(m.Grid(),m.DoubleData()*v);
    else
        return DTMutableMesh2D(m.Grid(),m.FloatData()*float(v));
}

DTMutableMesh2D operator-(const DTMesh2D &m)
{
    if (m.DoublePrecision())
        return DTMutableMesh2D(m.Grid(),-m.DoubleData());
    else
        return DTMutableMesh2D(m.Grid(),-m.FloatData());
}

DTMesh2D ConvertToFloat(DTMesh2D m)
{
    if (m.FloatPrecision())
        return m;
    else
        return DTMesh2D(m.Grid(),ConvertToFloat(m.DoubleData()));
}

DTMesh2D ConvertToDouble(DTMesh2D m)
{
    if (m.DoublePrecision())
        return m;
    else
        return DTMesh2D(m.Grid(),ConvertToDouble(m.FloatData()));
}

DTRegion2D BoundingBox(const DTMesh2D &v)
{
    return BoundingBox(v.Grid());
}

DTRegion1D ValueRange(const DTMesh2D &v)
{
    if (v.DoublePrecision())
        return ValueRange(v.DoubleData());
    else
        return ValueRange(v.FloatData());
}

DTMask FindRange(const DTMesh2D &v,const DTRegion1D &r)
{
	DTMutableCharArray zerosAndOnes(v.m(),v.n());
	char *maskD = zerosAndOnes.Pointer();
	ssize_t ij,mn = zerosAndOnes.Length();
	
	if (v.DoublePrecision()) {
		DTDoubleArray da = v.DoubleData();
		double minV = r.minV;
		double maxV = r.maxV;
		const double *D = da.Pointer();
		for (ij=0;ij<mn;ij++) {
			maskD[ij] = (D[ij]>=minV && D[ij]<=maxV);
		}
	}
	else {
		DTFloatArray fa = v.FloatData();
		float minV = float(r.minV);
		float maxV = float(r.maxV);
		const float *D = fa.Pointer();
		for (ij=0;ij<mn;ij++) {
			maskD[ij] = (D[ij]>=minV && D[ij]<=maxV);
		}
	}
	return DTMask(zerosAndOnes);
}

double SquareAverage(const DTMesh2D &v)
{
	if (v.Grid().MaskDefined()) {
		DTErrorMessage("SquareAverage","Not supported for a masked array - yet");
		// E-mail david@visualdatatools.com to ask for this to be implemented.
		return NAN;
	}
	// Average the squares of M.  Independent of grid size, and does not 
	// attempt to approximate an integral.
    if (v.DoublePrecision()) {
		DTDoubleArray D = v.DoubleData();
		ssize_t i,howMany = D.Length();
		const double *DD = D.Pointer();
		double sum = 0;
		for (i=0;i<howMany;i++) {
			sum += DD[i]*DD[i];
		}
		return sqrt(sum/howMany);
	}
	else {
		DTFloatArray F = v.FloatData();
		ssize_t i,howMany = F.Length();
		const float *FD = F.Pointer();
		double sum = 0;
		for (i=0;i<howMany;i++) {
			sum += FD[i]*FD[i];
		}
		return sqrt(sum/howMany);
	}
}

double Minimum(const DTMesh2D &v)
{
    if (v.DoublePrecision())
        return Minimum(v.DoubleData());
	else
		return Minimum(v.FloatData());
}

double Maximum(const DTMesh2D &v)
{
    if (v.DoublePrecision())
        return Maximum(v.DoubleData());
	else
		return Maximum(v.FloatData());
}

DTMutableMesh2D Minimum(const DTMesh2D &A,const DTMesh2D &B)
{
    if (CompatibleMeshes(A, B)==false) {
        DTErrorMessage("Minimum(DTMesh2D,DTMesh2D)","Incompatible meshes");
        return DTMutableMesh2D();
    }

    if (A.DoublePrecision()) {
        return DTMutableMesh2D(A.Grid(),Minimum(A.DoubleData(),B.DoubleData()));
    }
    else {
        return DTMutableMesh2D(A.Grid(),Minimum(A.FloatData(),B.FloatData()));
    }
}

DTMutableMesh2D Maximum(const DTMesh2D &A,const DTMesh2D &B)
{
    if (CompatibleMeshes(A, B)==false) {
        DTErrorMessage("Maximum(DTMesh2D,DTMesh2D)","Incompatible meshes");
        return DTMutableMesh2D();
    }
    
    if (A.DoublePrecision()) {
        return DTMutableMesh2D(A.Grid(),Maximum(A.DoubleData(),B.DoubleData()));
    }
    else {
        return DTMutableMesh2D(A.Grid(),Maximum(A.FloatData(),B.FloatData()));
    }
}

void Read(const DTDataStorage &input,const std::string &name,DTMesh2D &toReturn)
{
    DTDoubleArray dData;
    DTFloatArray fData;
    
    if (input.SavedAsDouble(name)) {
        Read(input,name,dData);
    }
    else {
        Read(input,name,fData);
    }

    if (dData.o()>1 || fData.o()>1) {
        DTErrorMessage("Read(DTMesh2D)","Can not read a three dimensional array.");
        toReturn = DTMesh2D();
        return;
    }

    DTMesh2DGrid grid;
    std::string locName = name+"_loc";
    if (input.Contains(locName)) {
        Read(input,locName,grid);
        if (dData.NotEmpty())
            grid = ChangeSize(grid,dData.m(),dData.n());
        else
            grid = ChangeSize(grid,fData.m(),fData.n());
    }
    else {
        if (dData.NotEmpty())
            grid = DTMesh2DGrid(dData.m(),dData.n());
        else
            grid = DTMesh2DGrid(fData.m(),fData.n());
    }

    std::string domName = name+"_dom";
    if (input.Contains(domName)) {
        DTMask mask;
        Read(input,domName,mask);
        grid = ApplyMask(grid,mask);
        mask = grid.Mask(); // In case it wasn't valid.
        
        ssize_t howManyPoints = mask.NumberOfPoints();
        if (fData.Length()==howManyPoints || dData.Length()==howManyPoints) {
            // Saved as a flattened array.  Only the points in the domain are saved.
            DTIntArray offsets = mask.Offsets();
            int i;
            if (fData.NotEmpty()) {
                DTMutableFloatArray arr(mask.m(),mask.n());
                arr = 0.0;
                for (i=0;i<howManyPoints;i++) {
                    arr(offsets(i)) = fData(i);
                }
                fData = arr;
            }
            else if (dData.NotEmpty()) {
                DTMutableDoubleArray arr(mask.m(),mask.n());
                arr = 0.0;
                for (i=0;i<howManyPoints;i++) {
                    arr(offsets(i)) = dData(i);
                }
                dData = arr;
            }
        }
    }
    
    if (fData.NotEmpty())
        toReturn = DTMesh2D(grid,fData);
    else
        toReturn = DTMesh2D(grid,dData);
}

void Write(DTDataStorage &output,const std::string &name,const DTMesh2D &theMesh)
{
    Write(output,name+"_bbox2D",BoundingBox(theMesh));
    
    if (!theMesh.Grid().IsStandard()) {
        WriteNoSize(output,name+"_loc",theMesh.Grid());
    }

    if (theMesh.Grid().MaskDefined()) {
        Write(output,name+"_dom",theMesh.Grid().Mask());
    }
    
    if (theMesh.DoublePrecision())
        Write(output,name,theMesh.DoubleData());
    else
        Write(output,name,theMesh.FloatData());
}

void WriteOne(DTDataStorage &output,const std::string &name,const DTMesh2D &toWrite)
{
    Write(output,name,toWrite);
    Write(output,"Seq_"+name,"2D Mesh");
    output.Flush();
}

#pragma mark Mutable array

DTMutableMesh2D::DTMutableMesh2D(const DTMesh2DGrid &gr)
{
    _grid = gr;
    _mutableDoubleData = DTMutableDoubleArray(_grid.m(),_grid.n());
    _doubleData = _mutableDoubleData;
}

DTMutableMesh2D::DTMutableMesh2D(const DTMesh2DGrid &gr,const DTMutableDoubleArray &input)
: DTMesh2D(gr,input)
{
    if (gr==_grid) {
        _mutableDoubleData = input;
    }
}

DTMutableMesh2D::DTMutableMesh2D(const DTMesh2DGrid &gr,const DTMutableFloatArray &input)
: DTMesh2D(gr,input)
{
    if (gr==_grid) {
        _mutableFloatData = input;
    }
}

void DTMutableMesh2D::operator-=(const DTMesh2D &A)
{
    if (CompatibleMeshes(*this,A)==false) {
        DTErrorMessage("DTMutableMesh2D -= DTMesh2D","Incompatible meshes");
        return;
    }
    if (FloatPrecision())
        _mutableFloatData -= A.FloatData();
    else
        _mutableDoubleData -= A.DoubleData();
}

void DTMutableMesh2D::operator+=(const DTMesh2D &A)
{
    if (CompatibleMeshes(*this,A)==false) {
        DTErrorMessage("DTMutableMesh2D += DTMesh2D","Incompatible meshes");
        return;
    }
    _mutableDoubleData += A.DoubleData();
}

void DTMutableMesh2D::operator*=(const DTMesh2D &A)
{
    _mutableDoubleData *= A.DoubleData();
}

void DTMutableMesh2D::operator*=(const DTDoubleArray &A)
{
    if (FloatPrecision()) {
        DTErrorMessage("DTMutableMesh2D *= DTDoubleArray","mesh is float");
        return;
    }
    else {
        _mutableDoubleData *= A;
    }
}

void DTMutableMesh2D::operator/=(const DTMesh2D &A)
{
    if (CompatibleMeshes(*this,A)==false) {
        DTErrorMessage("DTMutableMesh2D /= DTMesh2D","Incompatible meshes");
        return;
    }
    _mutableDoubleData /= A.DoubleData();
}

void DTMutableMesh2D::operator*=(double a)
{
    _mutableDoubleData *= a;
}

void DTMutableMesh2D::operator=(double v)
{
    _mutableDoubleData = v;
}

void DTMutableMesh2D::SetMaskedOutValues(double v)
{
	if (_grid.MaskDefined()==false) return;
	DTMask theMask = _grid.Mask();
	DTIntArray intervals = theMask.Intervals();
	ssize_t i,interval,howMany = intervals.n();
	ssize_t startAt = 0,endAt;
    if (DoublePrecision()) {
        double *D = _mutableDoubleData.Pointer();
        for (interval=0;interval<howMany;interval++) {
            endAt = intervals(0,interval);
            for (i=startAt;i<endAt;i++)
                D[i] = v;
            startAt = intervals(1,interval)+1;
        }
        endAt = _mutableDoubleData.Length();
        for (i=startAt;i<endAt;i++)
		D[i] = v;
    }
    else {
        float fv = float(v);
        float *D = _mutableFloatData.Pointer();
        for (interval=0;interval<howMany;interval++) {
            endAt = intervals(0,interval);
            for (i=startAt;i<endAt;i++)
                D[i] = fv;
            startAt = intervals(1,interval)+1;
        }
        endAt = _mutableDoubleData.Length();
        for (i=startAt;i<endAt;i++)
            D[i] = fv;
    }
}

DTMutableMesh2D ConvertToFloat(const DTMutableMesh2D &m)
{
    if (m.FloatPrecision())
        return m;
    else
        return DTMutableMesh2D(m.Grid(),ConvertToFloat(m.DoubleData()));
}

DTMutableMesh2D ConvertToDouble(const DTMutableMesh2D &m)
{
    if (m.DoublePrecision())
        return m;
    else
        return DTMutableMesh2D(m.Grid(),ConvertToDouble(m.FloatData()));
}

DTMutableMesh2D Copy(const DTMesh2D &m)
{
    if (m.FloatPrecision())
        return DTMutableMesh2D(m.Grid(),m.FloatData().Copy());
    else
        return DTMutableMesh2D(m.Grid(),m.DoubleData().Copy());
}

DTMutableMesh2D ApplyMask(const DTMutableMesh2D &mesh,const DTMask &mask)
{
	if (mesh.m()==mask.m() && mesh.n()==mask.n() && 1==mask.o()) {
		if (mesh.FloatPrecision()) {
			return DTMutableMesh2D(ApplyMask(mesh.Grid(),mask),mesh.FloatData());
		}
		else {
			return DTMutableMesh2D(ApplyMask(mesh.Grid(),mask),mesh.DoubleData());
		}
	}
	else {
		DTErrorMessage("ApplyMask(DTMesh2D,DTMask)","Incompatible mask size.");
		return mesh;
	}
}

DTMutableMesh2D RemoveMask(const DTMutableMesh2D &mesh)
{
    if (mesh.FloatPrecision()) {
        return DTMutableMesh2D(RemoveMask(mesh.Grid()),mesh.FloatData());
    }
    else {
        return DTMutableMesh2D(RemoveMask(mesh.Grid()),mesh.DoubleData());
    }
}


