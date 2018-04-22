// Part of DTSource. Copyright 2004-2017. David Adalsteinsson.
// see http://www.visualdatatools.com/DTSource/license.html for more information.

#ifndef DTMesh2DGrid_H
#define DTMesh2DGrid_H

#include "DTDataStorage.h"
#include "DTPoint2D.h"
#include "DTMask.h"

struct DTRegion2D;
class DTShiftScale2D;

// This class describes the location in space for several variable types, such as DTMesh2D and DTBitmap2D.
// This maps into the "2D Mesh Grid" variable in DataTank.

// If you want a non-uniform grid use the DTStructuredGrid2D variable type.

class DTMesh2DGrid {
public:
    DTMesh2DGrid() : isEmpty(true), _m(0), _n(0), _origin(0,0), _dx(1.0), _dy(1.0), maskDefined(false), mask() {};
    DTMesh2DGrid(ssize_t m,ssize_t n);
    DTMesh2DGrid(DTPoint2D origin,double h);
    DTMesh2DGrid(DTPoint2D origin,double dx,double dy);
    DTMesh2DGrid(DTPoint2D origin,double dx,double dy,ssize_t m,ssize_t n);
    DTMesh2DGrid(DTPoint2D origin,double dx,double dy,const DTMask &);

    bool IsEmpty(void) const {return isEmpty;}
    int m(void) const {return _m;}
    int n(void) const {return _n;}

    DTPoint2D Origin(void) const {return _origin;}
    double dx(void) const {return _dx;}
    double dy(void) const {return _dy;}

    bool IsStandard(void) const {return (_origin.x==0.0 && _origin.y==0.0 && _dx==1.0 && _dy==1.0 && maskDefined==false);}
    bool MaskDefined(void) const {return maskDefined;}
    DTMask Mask(void) const;
    
    DTPoint2D GridToSpace(const DTPoint2D &P) const {return DTPoint2D(P.x*_dx + _origin.x,P.y*_dy + _origin.y);}
    DTPoint2D SpaceToGrid(const DTPoint2D &P) const {return DTPoint2D((P.x-_origin.x)/_dx,(P.y-_origin.y)/_dy);}

    // Functions for the debugging.
    void pinfo(void) const;

private:
    bool isEmpty;
    int _m,_n;
    DTPoint2D _origin;
    double _dx,_dy;
    bool maskDefined;
    DTMask mask;
};

extern bool operator==(const DTMesh2DGrid &,const DTMesh2DGrid &);
extern bool operator!=(const DTMesh2DGrid &,const DTMesh2DGrid &);

extern bool SameExceptForMask(const DTMesh2DGrid &,const DTMesh2DGrid &); // Allows for the origin to be slightly different to allow for rounding errors.

extern DTRegion2D BoundingBox(const DTMesh2DGrid &v);

extern DTMesh2DGrid ApplyMask(const DTMesh2DGrid &Grid,const DTMask &);
extern DTMesh2DGrid RemoveMask(const DTMesh2DGrid &Grid);
extern DTMesh2DGrid RemoveSize(const DTMesh2DGrid &Grid);
extern DTMesh2DGrid ChangeSize(const DTMesh2DGrid &Grid,ssize_t m,ssize_t n);
extern DTMesh2DGrid CellCenters(const DTMesh2DGrid &); // Strips the mask
extern DTMesh2DGrid Crop(const DTMesh2DGrid &,const DTRegion2D &);
extern DTMesh2DGrid Region(const DTMesh2DGrid &,const DTRange &iRange,const DTRange &jRange);
extern DTMesh2DGrid operator*(const DTShiftScale2D &,const DTMesh2DGrid &);

extern DTMesh2DGrid GridShift(const DTMesh2DGrid &,int i,int j);


// Reading and writing
extern void Read(const DTDataStorage &input,const std::string &name,DTMesh2DGrid &toReturn);
extern void Write(DTDataStorage &output,const std::string &name,const DTMesh2DGrid &theVar);
extern void WriteOne(DTDataStorage &output,const std::string &name,const DTMesh2DGrid &toWrite); // One time value, self documenting.
extern void WriteNoSize(DTDataStorage &output,const std::string &name,const DTMesh2DGrid &theVar);

#endif
