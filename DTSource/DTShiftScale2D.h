// Part of DTSource. Copyright 2004-2017. David Adalsteinsson.
// see http://www.visualdatatools.com/DTSource/license.html for more information.

#ifndef DTShiftScale2D_H
#define DTShiftScale2D_H

#include "DTDataStorage.h"
#include "DTPoint2D.h"

class DTShiftScale2D {
public:
    DTShiftScale2D() : shift(), scaleX(1.0), scaleY(1.0) {}
    DTShiftScale2D(DTPoint2D P,double sX,double sY) : shift(P), scaleX(sX), scaleY(sY) {}
    
    double ScaleX(void) const {return scaleX;}
    double ScaleY(void) const {return scaleY;}
    DTPoint2D Shift(void) const {return shift;}
    
    double TransformX(double x) const {return shift.x+x*scaleX;}
    double TransformY(double y) const {return shift.y+y*scaleY;}
    DTPoint2D Transform(const DTPoint2D &P) const {return DTPoint2D(shift.x+P.x*scaleX,shift.y+P.y*scaleY);}
    
    void pinfo(void) const;

private:
    DTPoint2D shift;
    double scaleX;
    double scaleY;
};

extern DTPoint2D operator*(const DTShiftScale2D &,const DTPoint2D &);
extern DTShiftScale2D ShiftBy(const DTPoint2D &);

// Reading and writing
extern void Read(const DTDataStorage &input,const std::string &name,DTShiftScale2D &toReturn);
extern void Write(DTDataStorage &output,const std::string &name,const DTShiftScale2D &theVar);
extern void WriteOne(DTDataStorage &output,const std::string &name,const DTShiftScale2D &toWrite); // One time value, self documenting.

#endif
