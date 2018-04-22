// Part of DTSource. Copyright 2004-2017. David Adalsteinsson.
// see http://www.visualdatatools.com/DTSource/license.html for more information.

#ifndef DTFunction1D_H
#define DTFunction1D_H

#include "DTFunction.h"

class DTDataStorage;
class DTDoubleArray;
class DTMutableDoubleArray;

// Allows you to hand in function objects as arguments, and define them with syntax like:
// DTFunction1D x = DTFunction::x();
// DTFunction1D f = sin(M_PI*x);
// And then evaluate
// double v = f(0.2);
// to compute sin(M_PI*0.2);


class DTFunction1D {
public:
    DTFunction1D() : fcn(DTFunction::Value(0)) {}
    DTFunction1D(const DTFunction &); // Run time error, if this is not a function of a single variable.
    
    double operator()(double v) const {return fcn.Evaluate(&v);} // If f is a DTFunction1D object, then f(3) evaluates that function at 3.
    DTMutableDoubleArray operator()(const DTDoubleArray &x) const; // Evaluates f at each index.
    
    void pinfo(void) const;
    std::string StringVersion(void) const {return fcn.StringVersion();}

    static DTFunction1D x(void) {return DTFunction1D(DTFunction::Constant("x"));}
    static DTFunction1D one(void) {return DTFunction1D(DTFunction::Value(1.0));}
    static DTFunction1D zero(void) {return DTFunction1D(DTFunction::Value(0.0));}
    static DTFunction1D Value(double v) {return DTFunction1D(DTFunction::Value(v));}
    
    DTFunction Function(void) const {return fcn;}
    
private:
    DTFunction fcn;
};

// Reading and writing
extern void Read(const DTDataStorage &input,const std::string &name,DTFunction1D &toReturn);
extern void Write(DTDataStorage &output,const std::string &name,const DTFunction1D &theVar);
extern void WriteOne(DTDataStorage &output,const std::string &name,const DTFunction1D &toWrite); // One time value, self documenting.

extern std::ostream &operator<<(std::ostream &,const DTFunction1D &);


// Operators 
extern DTFunction1D operator+(const DTFunction1D &,const DTFunction1D &);
extern DTFunction1D operator-(const DTFunction1D &,const DTFunction1D &);
extern DTFunction1D operator*(const DTFunction1D &,const DTFunction1D &);
extern DTFunction1D operator/(const DTFunction1D &,const DTFunction1D &);
extern DTFunction1D operator-(const DTFunction1D &);

extern DTFunction1D operator*(const DTFunction1D &,double); // constant + function
extern DTFunction1D operator*(double,const DTFunction1D &); // constant * function
extern DTFunction1D operator/(const DTFunction1D &,double);
extern DTFunction1D operator/(double,const DTFunction1D &);
extern DTFunction1D operator+(const DTFunction1D &,double);
extern DTFunction1D operator+(double,const DTFunction1D &);
extern DTFunction1D operator-(const DTFunction1D &,double);
extern DTFunction1D operator-(double,const DTFunction1D &);

extern DTFunction1D abs(const DTFunction1D &);
extern DTFunction1D sin(const DTFunction1D &);
extern DTFunction1D cos(const DTFunction1D &);
extern DTFunction1D asin(const DTFunction1D &);
extern DTFunction1D acos(const DTFunction1D &);
extern DTFunction1D tan(const DTFunction1D &);
extern DTFunction1D atan(const DTFunction1D &);
extern DTFunction1D sinh(const DTFunction1D &);
extern DTFunction1D cosh(const DTFunction1D &);
extern DTFunction1D tanh(const DTFunction1D &);
extern DTFunction1D j0(const DTFunction1D &);
extern DTFunction1D sqrt(const DTFunction1D &);
extern DTFunction1D gamma(const DTFunction1D &);
extern DTFunction1D loggamma(const DTFunction1D &);
extern DTFunction1D exp(const DTFunction1D &);
extern DTFunction1D log(const DTFunction1D &);
extern DTFunction1D log10(const DTFunction1D &);
extern DTFunction1D erfc(const DTFunction1D &);
extern DTFunction1D floor(const DTFunction1D &);
extern DTFunction1D ceil(const DTFunction1D &);
extern DTFunction1D round(const DTFunction1D &);
extern DTFunction1D fact(const DTFunction1D &);
extern DTFunction1D pow(const DTFunction1D &,double);


#endif
