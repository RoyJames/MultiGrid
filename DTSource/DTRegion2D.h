// Part of DTSource. Copyright 2004-2017. David Adalsteinsson.
// see http://www.visualdatatools.com/DTSource/license.html for more information.

#ifndef DTRegion2D_H
#define DTRegion2D_H

class DTDataStorage;

#include "DTPoint2D.h"

struct DTRegion2D {
    DTRegion2D() : isSet(false), xmin(0.0), xmax(0.0), ymin(0.0), ymax(0.0) {}
    DTRegion2D(DTPoint2D ll,DTPoint2D ur) : isSet(true), xmin(ll.x), xmax(ur.x), ymin(ll.y), ymax(ur.y) {}
    
    bool IsEmpty(void) const {return (xmin==xmax && ymin==ymax);}
    void pinfo(void) const;
	
	bool PointLiesInside(const DTPoint2D &P) const {return (xmin<=P.x && P.x<=xmax && ymin<=P.y && P.y<=ymax);}
	
	DTPoint2D LowerLeft(void) const {return DTPoint2D(xmin,ymin);}
	DTPoint2D UpperRight(void) const {return DTPoint2D(xmax,ymax);}
    
    bool isSet;
    double xmin,xmax,ymin,ymax;
};

bool operator==(const DTRegion2D &,const DTRegion2D &);
bool operator!=(const DTRegion2D &,const DTRegion2D &);

extern DTRegion2D BoundingBox2D(const DTDoubleArray &points);

extern DTRegion2D Union(const DTRegion2D &,const DTRegion2D &);
extern DTRegion2D Intersection(const DTRegion2D &,const DTRegion2D &);
extern DTRegion2D AddBorder(const DTRegion2D &,double);

extern void Read(const DTDataStorage &input,const std::string &name,DTRegion2D &region);
extern void Write(DTDataStorage &output,const std::string &name,const DTRegion2D &region);
extern void WriteOne(DTDataStorage &output,const std::string &name,const DTRegion2D &toWrite); // One time value, self documenting.

#endif
