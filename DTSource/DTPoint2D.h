// Part of DTSource. Copyright 2004-2017. David Adalsteinsson.
// see http://www.visualdatatools.com/DTSource/license.html for more information.

#ifndef DTPoint2D_H
#define DTPoint2D_H

#include "DTDataStorage.h"
#include <math.h>

struct DTPoint2D {
    DTPoint2D() : x(0.0), y(0.0) {}
    DTPoint2D(double xv,double yv) : x(xv), y(yv) {}

	void operator+=(const DTPoint2D &P) {x+=P.x; y+=P.y;}
	void operator-=(const DTPoint2D &P) {x-=P.x; y-=P.y;}
	
    void pinfo(void) const;

    double x,y;
};

inline DTPoint2D operator+(const DTPoint2D &A,const DTPoint2D &B) {return DTPoint2D(A.x+B.x,A.y+B.y);}
inline DTPoint2D operator-(const DTPoint2D &A,const DTPoint2D &B) {return DTPoint2D(A.x-B.x,A.y-B.y);}
inline DTPoint2D operator*(const DTPoint2D &A,double v) {return DTPoint2D(A.x*v,A.y*v);}
inline DTPoint2D operator*(double v,const DTPoint2D &A) {return DTPoint2D(A.x*v,A.y*v);}
inline DTPoint2D operator/(const DTPoint2D &A,double v) {return DTPoint2D(A.x/v,A.y/v);}
inline DTPoint2D operator-(const DTPoint2D &A) {return DTPoint2D(-A.x,-A.y);}

inline double Distance(const DTPoint2D &A,const DTPoint2D &B) {return sqrt((A.x-B.x)*(A.x-B.x)+(A.y-B.y)*(A.y-B.y));}
inline double Norm(const DTPoint2D &A) {return sqrt(A.x*A.x+A.y*A.y);}
inline double Dot(const DTPoint2D &A,const DTPoint2D &B) {return (A.x*B.x+A.y*B.y);}

inline bool operator==(const DTPoint2D &A,const DTPoint2D &B) {return (A.x==B.x && A.y==B.y);}
inline bool operator!=(const DTPoint2D &A,const DTPoint2D &B) {return (A.x!=B.x || A.y!=B.y);}

// Reading and writing
extern void Read(const DTDataStorage &input,const std::string &name,DTPoint2D &toReturn);
extern void Write(DTDataStorage &output,const std::string &name,const DTPoint2D &theVar);
extern void WriteOne(DTDataStorage &output,const std::string &name,const DTPoint2D &toWrite); // One time value, self documenting.

#endif
