// Part of DTSource. Copyright 2004-2017. David Adalsteinsson.
// see http://www.visualdatatools.com/DTSource/license.html for more information.

#ifndef DTMesh2D_H
#define DTMesh2D_H

#include "DTFloatArray.h"
#include "DTDoubleArray.h"
#include "DTDataStorage.h"
#include "DTMesh2DGrid.h"

struct DTRegion1D;
struct DTShiftScale2D;
class DTMutableMesh2D;

// 2D Mesh.  Values on a uniform grid.  The grid is specified by a DTMesh2DGrid object.
// The grid might possibly have a mask defined.

// The data in the mesh in unchangeable, and this class is not meant to be a base class for
// computations, but instead a IO repository.

// To be able to change the values, you can use
// meshvariable.Data().Copy()
// to get a copy of the data.  Note that when you create a mesh with
//    DTMesh2D(newValues,oldMesh.Grid())
// or equivalent, the data in the newValues array will be used, so if this entry is not saved immediately, 
// any subsequent changes in newValues will affect the data array stored in the mesh.  You can make sure
// this will not happen by doing
//    DTMesh2D(newValues.Copy(),oldMesh.Grid())

class DTMesh2D {
public:
    DTMesh2D() : _grid(), _floatData(), _doubleData() {};
    DTMesh2D(const DTMesh2DGrid &grid,const DTDoubleArray &input);
    DTMesh2D(const DTMesh2DGrid &grid,const DTFloatArray &input);
    
    bool IsEmpty(void) const {return (_floatData.IsEmpty() && _doubleData.IsEmpty());}
    
    bool FloatPrecision(void) const {return (_floatData.NotEmpty());}
    bool DoublePrecision(void) const {return (_doubleData.NotEmpty());}
    
    DTDoubleArray DoubleData(void) const;
    DTFloatArray FloatData(void) const;
    DTMesh2DGrid Grid(void) const {return _grid;}
    
    ssize_t m(void) const {return _grid.m();}
    ssize_t n(void) const {return _grid.n();}

    // Functions for the debugging.
    void pinfo(void) const;

protected:
    DTMesh2DGrid _grid;
    DTFloatArray _floatData;
    DTDoubleArray _doubleData;
};

extern bool CompatibleMeshes(const DTMesh2D &,const DTMesh2D &); // Same grid, both float/double

extern bool operator==(const DTMesh2D &,const DTMesh2D &);
extern bool operator!=(const DTMesh2D &,const DTMesh2D &);

extern DTMesh2D ApplyMask(const DTMesh2D &,const DTMask &);
extern DTMesh2D RemoveMask(const DTMesh2D &);
extern DTMesh2D Crop(const DTMesh2D &,const DTRegion2D &);
extern DTMesh2D operator*(const DTShiftScale2D &,const DTMesh2D &);
extern DTMutableMesh2D operator-(const DTMesh2D &,const DTMesh2D &);
extern DTMutableMesh2D operator+(const DTMesh2D &,double);
extern DTMutableMesh2D operator*(const DTMesh2D &,double);
extern DTMutableMesh2D operator-(const DTMesh2D &);

extern DTMask FindRange(const DTMesh2D &,const DTRegion1D &);
extern DTMesh2D ConvertToFloat(DTMesh2D);
extern DTMesh2D ConvertToDouble(DTMesh2D);

extern DTRegion2D BoundingBox(const DTMesh2D &);
extern DTRegion1D ValueRange(const DTMesh2D &);
extern double SquareAverage(const DTMesh2D &);
extern double Maximum(const DTMesh2D &);

// Reading and writing
extern void Read(const DTDataStorage &input,const std::string &name,DTMesh2D &toReturn);
extern void Write(DTDataStorage &output,const std::string &name,const DTMesh2D &theMesh);
extern void WriteOne(DTDataStorage &output,const std::string &name,const DTMesh2D &toWrite); // One time value, self documenting.

extern double Minimum(const DTMesh2D &);
extern double Maximum(const DTMesh2D &);
extern DTMutableMesh2D Minimum(const DTMesh2D &,const DTMesh2D &);
extern DTMutableMesh2D Maximum(const DTMesh2D &,const DTMesh2D &);

// Mutable mesh version.  Maintains the grid. When you combine two meshes they have to have the
// same resolution (float/double)

class DTMutableMesh2D : public DTMesh2D
{
public:
    DTMutableMesh2D() : DTMesh2D() {}
    DTMutableMesh2D(const DTMesh2DGrid &grid); // Double array
    DTMutableMesh2D(const DTMesh2DGrid &grid,const DTMutableDoubleArray &input);
    DTMutableMesh2D(const DTMesh2DGrid &grid,const DTMutableFloatArray &input);

    DTMutableDoubleArray DoubleData(void) const {return _mutableDoubleData;}
    DTMutableFloatArray FloatData(void) const {return _mutableFloatData;}

    void operator-=(const DTMesh2D &);
    void operator+=(const DTMesh2D &);
    void operator*=(const DTMesh2D &);
    void operator*=(const DTDoubleArray &);
    void operator/=(const DTMesh2D &);
    void operator*=(double);

    void operator=(double);
	void SetMaskedOutValues(double); // Only set values outside the mask.

private:
    DTMutableDoubleArray _mutableDoubleData;
    DTMutableFloatArray _mutableFloatData;
};

extern DTMutableMesh2D Copy(const DTMesh2D &);

extern DTMutableMesh2D ConvertToFloat(const DTMutableMesh2D &);
extern DTMutableMesh2D ConvertToDouble(const DTMutableMesh2D &);

extern DTMutableMesh2D ApplyMask(const DTMutableMesh2D &mesh,const DTMask &mask);
extern DTMutableMesh2D RemoveMask(const DTMutableMesh2D &);


#endif
