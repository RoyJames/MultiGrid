// Part of DTSource. Copyright 2004-2017. David Adalsteinsson.
// see http://www.visualdatatools.com/DTSource/license.html for more information.

#include "DTFunction1D.h"

#include "DTDataStorage.h"
#include "DTDoubleArray.h"
#include "DTError.h"

DTFunction1D::DTFunction1D(const DTFunction &f)
{
    // In order for this to valid, it is required to have only an x argument.
    if (f.ConstantNames().Length()>1) {
        DTErrorMessage("DTFunction1D(DTFunction)","Can only have one unknown argument");
        fcn = DTFunction();
    }
    else if (f.ConstantNames().Length()==1 && f.ConstantNames()(0)!="x") {
        DTErrorMessage("DTFunction1D(DTFunction)","Can only depend on \"x\"");
        fcn = DTFunction();
    }
    else {
        fcn = f;
    }
}

DTMutableDoubleArray DTFunction1D::operator()(const DTDoubleArray &xin) const
{
    DTMutableDoubleArray toReturn = xin.Copy();
    double *ret = toReturn.Pointer();
    ssize_t howMany = toReturn.Length();
    int i;
    for (i=0;i<howMany;i++) {
        toReturn(i) = fcn.Evaluate(ret+i);
    }
    return toReturn;
}

void DTFunction1D::pinfo(void) const
{
    fcn.pinfo();
}

void Read(const DTDataStorage &input,const std::string &name,DTFunction1D &toReturn)
{
    DTFunction fcn;
    Read(input,name,fcn);
    toReturn = DTFunction1D(fcn);
}

void Write(DTDataStorage &output,const std::string &name,const DTFunction1D &theVar)
{
    Write(output,name,theVar.Function());
}

void WriteOne(DTDataStorage &output,const std::string &name,const DTFunction1D &toWrite)
{
    Write(output,name,toWrite);
    Write(output,"Seq_"+name,"1D Function");
    output.Flush();
}

DTFunction1D operator+(const DTFunction1D &A,const DTFunction1D &B)
{
    return DTFunction1D(A.Function()+B.Function());
}

DTFunction1D operator-(const DTFunction1D &A,const DTFunction1D &B)
{
    return DTFunction1D(A.Function()-B.Function());
}

DTFunction1D operator*(const DTFunction1D &A,const DTFunction1D &B)
{
    return DTFunction1D(A.Function()*B.Function());
}

DTFunction1D operator/(const DTFunction1D &A,const DTFunction1D &B)
{
    return DTFunction1D(A.Function()/B.Function());
}

DTFunction1D operator-(const DTFunction1D &A)
{
    return DTFunction1D(-A.Function());
}

DTFunction1D operator*(const DTFunction1D &A,double b)
{
    return DTFunction1D(A.Function()*b);
}

DTFunction1D operator*(double a,const DTFunction1D &B)
{
    return DTFunction1D(a*B.Function());
}

DTFunction1D operator/(const DTFunction1D &A,double b)
{
    return DTFunction1D(A.Function()/b);
}

DTFunction1D operator/(double a,const DTFunction1D &B)
{
    return DTFunction1D(a/B.Function());
}

DTFunction1D operator+(const DTFunction1D &A,double b)
{
    return DTFunction1D(A.Function()+b);
}

DTFunction1D operator+(double a,const DTFunction1D &B)
{
    return DTFunction1D(a+B.Function());
}

DTFunction1D operator-(const DTFunction1D &A,double b)
{
    return DTFunction1D(A.Function()-b);
}

DTFunction1D operator-(double a,const DTFunction1D &B)
{
    return DTFunction1D(a-B.Function());
}

DTFunction1D abs(const DTFunction1D &F)
{
    return DTFunction1D(abs(F.Function()));
}

DTFunction1D sin(const DTFunction1D &F)
{
    return DTFunction1D(sin(F.Function()));
}

DTFunction1D cos(const DTFunction1D &F)
{
    return DTFunction1D(cos(F.Function()));
}

DTFunction1D asin(const DTFunction1D &F)
{
    return DTFunction1D(asin(F.Function()));
}

DTFunction1D acos(const DTFunction1D &F)
{
    return DTFunction1D(acos(F.Function()));
}

DTFunction1D tan(const DTFunction1D &F)
{
    return DTFunction1D(tan(F.Function()));
}

DTFunction1D atan(const DTFunction1D &F)
{
    return DTFunction1D(atan(F.Function()));
}

DTFunction1D sinh(const DTFunction1D &F)
{
    return DTFunction1D(sinh(F.Function()));
}

DTFunction1D cosh(const DTFunction1D &F)
{
    return DTFunction1D(cosh(F.Function()));
}

DTFunction1D tanh(const DTFunction1D &F)
{
    return DTFunction1D(tanh(F.Function()));
}

DTFunction1D j0(const DTFunction1D &F)
{
    return DTFunction1D(j0(F.Function()));
}

DTFunction1D sqrt(const DTFunction1D &F)
{
    return DTFunction1D(sqrt(F.Function()));
}

DTFunction1D gamma(const DTFunction1D &F)
{
    return DTFunction1D(gamma(F.Function()));
}

DTFunction1D loggamma(const DTFunction1D &F)
{
    return DTFunction1D(loggamma(F.Function()));
}

DTFunction1D exp(const DTFunction1D &F)
{
    return DTFunction1D(exp(F.Function()));
}

DTFunction1D log(const DTFunction1D &F)
{
    return DTFunction1D(log(F.Function()));
}

DTFunction1D log10(const DTFunction1D &F)
{
    return DTFunction1D(log10(F.Function()));
}

DTFunction1D erfc(const DTFunction1D &F)
{
    return DTFunction1D(erfc(F.Function()));
}

DTFunction1D floor(const DTFunction1D &F)
{
    return DTFunction1D(floor(F.Function()));
}

DTFunction1D ceil(const DTFunction1D &F)
{
    return DTFunction1D(ceil(F.Function()));
}

DTFunction1D round(const DTFunction1D &F)
{
    return DTFunction1D(round(F.Function()));
}

DTFunction1D fact(const DTFunction1D &F)
{
    return DTFunction1D(fact(F.Function()));
}

DTFunction1D pow(const DTFunction1D &F,double p)
{
    return DTFunction1D(pow(F.Function(),p));
}

