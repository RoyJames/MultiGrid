// Part of DTSource. Copyright 2004-2017. David Adalsteinsson.
// see http://www.visualdatatools.com/DTSource/license.html for more information.

#ifndef DTFunction2D_H
#define DTFunction2D_H

#include "DTFunction.h"

class DTDataStorage;
class DTFunction1D;

class DTFunction2D {
public:
    DTFunction2D() : fcn(DTFunction::Value(0)) {}
    DTFunction2D(const DTFunction &);
    
    double operator()(double xv,double yv) const {double v[2]; v[0] = xv; v[1] = yv; return fcn.Evaluate(v);}
    
    DTFunction2D operator()(const DTFunction2D &,const DTFunction2D &); // g(function,function)
    
    void pinfo(void) const;
    std::string StringVersion(void) const {return fcn.StringVersion();}
    
    static DTFunction2D x(void) {return DTFunction2D(DTFunction::Constant("x"));}
    static DTFunction2D y(void) {return DTFunction2D(DTFunction::Constant("y"));}
    static DTFunction2D one(void) {return DTFunction2D(DTFunction::Value(1.0));}
    static DTFunction2D Value(double v) {return DTFunction2D(DTFunction::Value(v));}

    DTFunction Function(void) const {return fcn;}
    
private:
    DTFunction fcn;
};

// Reading and writing
extern void Read(const DTDataStorage &input,const std::string &name,DTFunction2D &toReturn);
extern void Write(DTDataStorage &output,const std::string &name,const DTFunction2D &theVar);
extern void WriteOne(DTDataStorage &output,const std::string &name,const DTFunction2D &toWrite); // One time value, self documenting.

extern std::ostream &operator<<(std::ostream &,const DTFunction2D &);

// Operators 
extern DTFunction2D operator+(const DTFunction2D &,const DTFunction2D &);
extern DTFunction2D operator-(const DTFunction2D &,const DTFunction2D &);
extern DTFunction2D operator*(const DTFunction2D &,const DTFunction2D &);
extern DTFunction2D operator/(const DTFunction2D &,const DTFunction2D &);
extern DTFunction2D operator-(const DTFunction2D &);

extern DTFunction2D operator+(const DTFunction1D &,const DTFunction2D &);
extern DTFunction2D operator-(const DTFunction1D &,const DTFunction2D &);
extern DTFunction2D operator*(const DTFunction1D &,const DTFunction2D &);
extern DTFunction2D operator/(const DTFunction1D &,const DTFunction2D &);

extern DTFunction2D operator+(const DTFunction2D &,const DTFunction1D &);
extern DTFunction2D operator-(const DTFunction2D &,const DTFunction1D &);
extern DTFunction2D operator*(const DTFunction2D &,const DTFunction1D &);
extern DTFunction2D operator/(const DTFunction2D &,const DTFunction1D &);

extern DTFunction2D operator*(const DTFunction2D &,double);
extern DTFunction2D operator*(double,const DTFunction2D &);
extern DTFunction2D operator/(const DTFunction2D &,double);
extern DTFunction2D operator/(double,const DTFunction2D &);
extern DTFunction2D operator+(const DTFunction2D &,double);
extern DTFunction2D operator+(double,const DTFunction2D &);
extern DTFunction2D operator-(const DTFunction2D &,double);
extern DTFunction2D operator-(double,const DTFunction2D &);

extern DTFunction2D abs(const DTFunction2D &);
extern DTFunction2D sin(const DTFunction2D &);
extern DTFunction2D cos(const DTFunction2D &);
extern DTFunction2D asin(const DTFunction2D &);
extern DTFunction2D acos(const DTFunction2D &);
extern DTFunction2D tan(const DTFunction2D &);
extern DTFunction2D atan(const DTFunction2D &);
extern DTFunction2D sinh(const DTFunction2D &);
extern DTFunction2D cosh(const DTFunction2D &);
extern DTFunction2D tanh(const DTFunction2D &);
extern DTFunction2D sqrt(const DTFunction2D &);
extern DTFunction2D gamma(const DTFunction2D &);
extern DTFunction2D loggamma(const DTFunction2D &);
extern DTFunction2D exp(const DTFunction2D &);
extern DTFunction2D log(const DTFunction2D &);
extern DTFunction2D log10(const DTFunction2D &);
extern DTFunction2D erfc(const DTFunction2D &);
extern DTFunction2D floor(const DTFunction2D &);
extern DTFunction2D ceil(const DTFunction2D &);
extern DTFunction2D round(const DTFunction2D &);
extern DTFunction2D fact(const DTFunction2D &);
extern DTFunction2D pow(const DTFunction2D &,double);

#endif
