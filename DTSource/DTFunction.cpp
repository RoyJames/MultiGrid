// Part of DTSource. Copyright 2004-2017. David Adalsteinsson.
// see http://www.visualdatatools.com/DTSource/license.html for more information.

#include "DTFunction.h"
#include "DTDataStorage.h"
#include "DTDoubleArray.h"
#include "DTUtilities.h"
#include "DTDoubleArrayOperators.h"
#include "DTDictionary.h"
#include "DTError.h"

#include <iostream>
#include <sstream>
#include <math.h>
#include <limits>

//#if defined(WIN32)
//#define LGAMMA(x) (log((((((.0112405826571654074 / ((x) + 5.00035898848319255) + .502197227033920907) / ((x) + 3.99999663000075089) + 2.09629553538949977) / ((x) + 3.00000004672652415) + 2.25023047535618168) / ((x) + 1.99999999962010231) + .851370813165034183) / ((x) + 1.00000000000065532) + .122425977326879918) / (x) + .0056360656189756065) + ((x) - .5) * log((x) + 6.09750757539068576) - (x))
//#endif

static double simpleFactorial(double x);
static DTFunction BinaryOperator(DTFunctionLeaf::LeafType t,const DTFunctionStorage &A,const DTFunctionStorage &B);
static double dtRect(double x);
static double dtTri(double x);
static double dtH(double x);
static double dtSgn(double x);
static double dtSinc(double x);
static double dtYear(double sec);
static double dtMonth(double sec);
static double dtDay(double sec);
static double dtHour(double sec);
static double dtMinute(double sec);
static double dtIsAM(double sec);
static double dtIsPM(double sec);
static double dtIsNaN(double x);
static double dtIsFinite(double x);

typedef double (*DTFunctionFunctionPointer)(double);

double dtSinc(double x)
{
    return (x==0.0 ? 1.0 : sin(x)/x);
}

double simpleFactorial(double x)
{
    int n = int(round(x));
#if defined(WIN32) && !defined(INFINITY)
#define INFINITY std::numeric_limits<float>::infinity();
#endif
#if defined(WIN32) && !defined(NAN)
#define NAN std::numeric_limits<float>::quiet_NaN();
#endif
    if (n<0) return NAN;
    if (n>170) return INFINITY;
    
    double toReturn = 1.0;
    for (int i=2;i<=n;i++) {
        toReturn*=i;
    }
    
    return toReturn;
}

double dtRect(double x)
{
    if (x<-0.5)
        return 0.0;
    else if (x==-0.5)
        return 0.5;
    else if (x<0.5)
        return 1.0;
    else if (x==0.5)
        return 0.5;
    else
        return 0.0;
}

double dtTri(double x)
{
    if (x<-1.0)
        return 0.0;
    else if (x<1.0)
        return 1.0-fabs(x);
    else
        return 0.0;
}

double dtH(double x)
{
    if (x<0.0)
        return 0.0;
    else if (x==0.0)
        return 0.5;
    else
        return 1.0;
}

double dtSgn(double x)
{
    if (x<0.0)
        return -1.0;
    else if (x==0.0)
        return 0.0;
    else
        return 1.0;
}

double dtYear(double )
{
    return 0.0;
}

double dtMonth(double )
{
    return 0.0;
}

double dtDay(double )
{
    return 0.0;
}

double dtHour(double sec)
{
    return floor(sec/3600 - floor(sec/(24*3600))*24);
}

double dtMinute(double sec)
{
    return floor(sec/60.0 - floor(sec/3600)*60);
}

double dtIsAM(double sec)
{
    double hour = floor(sec/3600 - floor(sec/(24*3600))*24);
    return (hour<12);
}

double dtIsPM(double sec)
{
    double hour = floor(sec/3600 - floor(sec/(24*3600))*24);
    return (hour>=12);
}

double dtIsNaN(double x)
{
    return isnan(x);
}

double dtIsFinite(double x)
{
    return isfinite(x);
}

DTFunction SingleOperator(DTFunctionLeaf::FunctionName type,const DTFunctionStorage &A);

void DTFunctionLeaf::ComputeIfThisLeafIsAList(DTMutableList<DTFunctionLeaf> &leafs)
{
    if (type==ListOfValues)
        leafIsAList = true;
    else if (type==Plus || type==Minus || type==Times || type==Divide) {
        leafs(leftReference).ComputeIfThisLeafIsAList(leafs);
        leafs(rightReference).ComputeIfThisLeafIsAList(leafs);
        leafIsAList = (leafs(leftReference).leafIsAList || leafs(rightReference).leafIsAList);
    }
}

DTMutableDoubleArray DTFunctionLeaf::EvaluateFromLists(const DTList<DTDoubleArray> &v,const DTFunctionStorage &storage) const
{
	// Return a 1x1 array if all of the values are the same but a list of length len if they are not.
	DTMutableDoubleArray toReturn;
	DTMutableDoubleArray temp1,temp2;
	ssize_t i,howMany;
    DTDoubleArray x;
	double c;
    
    switch (type) {
        case Value:
			temp1 = DTMutableDoubleArray(1);
            temp1 = value;
			toReturn = temp1;
            break;
        case Constant:
            toReturn = v(constantNumber).Copy();
            break;
        case Plus:
			temp1 = storage.leafs(leftReference).EvaluateFromLists(v,storage);
			temp2 = storage.leafs(rightReference).EvaluateFromLists(v,storage);
			if (temp2.Length()==1) {
				temp1 += temp2(0);
				toReturn = temp1;
			}
			else if (temp1.Length()==1) {
				temp2 += temp1(0);
				toReturn = temp2;
			}
			else {
				temp1+=temp2;
				toReturn = temp1;
			}
            break;
        case Minus:
			temp1 = storage.leafs(leftReference).EvaluateFromLists(v,storage);
			temp2 = storage.leafs(rightReference).EvaluateFromLists(v,storage);
			if (temp2.Length()==1) {
				temp1 -= temp2(0);
				toReturn = temp1;
			}
			else if (temp1.Length()==1) {
				temp2 -= temp1(0);
				temp2 *= -1;
				toReturn = temp2;
			}
			else {
				temp1-=temp2;
				toReturn = temp1;
			}
            break;
        case Times:
			temp1 = storage.leafs(leftReference).EvaluateFromLists(v,storage);
			temp2 = storage.leafs(rightReference).EvaluateFromLists(v,storage);
			if (temp2.Length()==1) {
				temp1 *= temp2(0);
				toReturn = temp1;
			}
			else if (temp1.Length()==1) {
				temp2 *= temp1(0);
				toReturn = temp2;
			}
			else {
				temp1*=temp2;
				toReturn = temp1;
			}
            break;
        case Divide:
			temp1 = storage.leafs(leftReference).EvaluateFromLists(v,storage);
			temp2 = storage.leafs(rightReference).EvaluateFromLists(v,storage);
			if (temp2.Length()==1) {
				temp1 /= temp2(0);
				toReturn = temp1;
			}
			else if (temp1.Length()==1) {
				// Inefficient constant/List
				toReturn = temp1(0)/temp2; // Really should be able to reuse temp2
			}
			else {
				temp1/=temp2;
				toReturn = temp1;
			}
            break;
        case Power:
			temp1 = storage.leafs(leftReference).EvaluateFromLists(v,storage);
			temp2 = storage.leafs(rightReference).EvaluateFromLists(v,storage);
			if (temp1.Length()==1 && temp2.Length()==1) {
				temp1(0) = pow(temp1(0),temp2(0));
				toReturn = temp1;
			}
			else if (temp2.Length()==1) {
				howMany = temp1.Length();
				c = temp2(0);
				for (i=0;i<howMany;i++) temp1(i) = pow(temp1(i),c);
				toReturn = temp1;
			}
			else if (temp1.Length()==1) {
				howMany = temp2.Length();
				c = temp1(0);
				for (i=0;i<howMany;i++) temp2(i) = pow(c,temp2(i));
				toReturn = temp2;
			}
			else {
				howMany = temp2.Length();
				for (i=0;i<howMany;i++) temp1(i) = pow(temp1(i),temp2(i));
				toReturn = temp1;
			}
            break;
        case PredefinedFunction:
        {
            if (storage.leafs(leftReference).leafIsAList) {
                DTErrorMessage("DTFunctionLeaf::EvaluateFromLists","Not done yet");
                /*
                // List of numbers.  Only a few functions will allow this.
                DTList<DTMutableDoubleArray> lists = storage.leafs(leftReference).EvaluateFromLists(v,storage);
                int i;
                int howMany = theArr.Length();
                switch (functionReference) {
                    case Min:
                    {
                        toReturn = theArr(0);
                        for (i=1;i<howMany;i++) {
                            if (toReturn>theArr(i)) toReturn = theArr(i);
                        }
                        break;
                    }
                    case Max:
                    {
                        toReturn = theArr(0);
                        for (i=1;i<howMany;i++) {
                            if (toReturn<theArr(i)) toReturn = theArr(i);
                        }
                        break;
                    }
                    case Norm:
                    {
                        toReturn = theArr(0)*theArr(0);
                        for (i=1;i<howMany;i++)
                            toReturn += theArr(i)*theArr(i);
                        toReturn = sqrt(toReturn);
                        break;
                    }
                    case SignSwitch:
                    {
                        if (howMany==3) {
                            if (theArr(0)<=0.0)
                                toReturn = theArr(1);
                            else
                                toReturn = theArr(2);
                        }
                        else {
                            if (theArr(0)<0.0)
                                toReturn = theArr(1);
                            else if (theArr(0)==0.0)
                                toReturn = theArr(2);
                            else
                                toReturn = theArr(3);
                        }
                        break;
                    }
                    case Angle:
                    {
                        toReturn = atan2(theArr(1),theArr(0));
                        break;
                    }
                    case Jn:
                    {
                        toReturn = jn(theArr(0),theArr(1));
                        break;
                    }
                    case Yn:
                    {
                        toReturn = yn(theArr(0),theArr(1));
                        break;
                    }
                    case Mod:
                    {
                        toReturn = fmod(theArr(0),theArr(1));
                        break;
                    }
                    default:
                        // Not supported for this function.
                        DTErrorMessage("DTFunctionLeaf::Evaluate","Not supported yet");
                        toReturn = 0.0;
                        break;
                }
                 */
            }
            else {
                // Single argument
                temp1 = storage.leafs(leftReference).EvaluateFromLists(v,storage);
                DTFunctionFunctionPointer fcn = 0;

                switch (functionReference) {
                    case Abs:      fcn = fabs;       break;
                    case Sin:      fcn = sin;        break;
                    case Cos:      fcn = cos;        break;
                    case ASin:     fcn = asin;       break;
                    case ACos:     fcn = acos;       break;
                    case Tan:      fcn = tan;        break;
                    case ATan:     fcn = atan;       break;
                    case Sinh:     fcn = sinh;       break;
                    case Cosh:     fcn = cosh;       break;
                    case Tanh:     fcn = tanh;       break;
                    case Sqrt:     fcn = sqrt;       break;
                    case Gamma:    fcn = tgamma;     break;
                    case LogGamma: fcn = lgamma;     break;
                    case Exp:      fcn = exp;        break;
                    case Log:      fcn = log;        break;
                    case Log10:    fcn = log10;      break;
                    case Log2:     fcn = log2;       break;
                    case Erfc:     fcn = erfc;       break;
                    case Erf:      fcn = erf;        break;
                    case Floor:    fcn = floor;      break;
                    case Ceil:     fcn = ceil;       break;
                    case Round:    fcn = round;      break;
                    case Fact:     fcn = simpleFactorial;      break;
                    case Norm:     fcn = fabs;       break;
                        
                    case J0:       fcn = j0;         break;
                    case J1:       fcn = j1;         break;
                    case Y0:       fcn = y0;         break;
                    case Y1:       fcn = y1;         break;
                    case Sinc:     fcn = dtSinc;     break;
                    case Rect:     fcn = dtRect;     break;
                    case Tri:      fcn = dtTri;      break;
                    case H:        fcn = dtH;        break;
                    case Sgn:      fcn = dtSgn;      break;
                        
                    case Year:     fcn = dtYear;     break;
                    case Month:    fcn = dtMonth;    break;
                    case Day:      fcn = dtDay;      break;
                    case Hour:     fcn = dtHour;     break;
                    case Minute:   fcn = dtMinute;   break;
                    case IsAM:     fcn = dtIsAM;     break;
                    case IsPM:     fcn = dtIsPM;     break;
                    case Log1p:    fcn = log1p;      break;
                    case Cbrt:     fcn = cbrt;       break;
                    case Isnan:    fcn = dtIsNaN;    break;
                    case Isfinite: fcn = dtIsFinite; break;
                        
                    default:
                        DTErrorMessage("DTFunctionLeaf::EvaluateFromList","Not implemented yet");
                        // Identity,Min,Max all don't change the number
                        break;
                }

                if (fcn) {
                    // Apply this to every entry
                    howMany = temp1.Length();
                    for (i=0;i<howMany;i++) temp1(i) = fcn(temp1(i));
                }
                else {
                    temp1 = DTMutableDoubleArray(1);
                    temp1 = NAN;
                }
                toReturn = temp1;
            }
            break;
        }
            /*
        case Function:
        {
            DTFunction foo = storage.SubFunction(functionNumber);
            if (storage.leafs(leftReference).leafIsAList) {
                // List of numbers.  Only a few functions will allow this.
                DTDoubleArray theArr = storage.leafs(leftReference).EvaluateArray(v,storage);
                toReturn = foo.Evaluate(theArr.Pointer());
            }
            else {
                toReturn = storage.leafs(leftReference).Evaluate(v,storage);
                toReturn = foo.Evaluate(&toReturn);
            }
            break;
        }
             */
        default:
            DTErrorMessage("DTFunctionLeaf::Evaluate","Not supported yet");
            toReturn = 0.0;
    }
	
    return toReturn;
}


DTMutableDoubleArray DTFunctionLeaf::EvaluateArray(const double *v,const DTFunctionStorage &storage) const
{
    DTMutableDoubleArray toRet;

    switch (type) {
        case ListOfValues:
        {
            // Evaluate each entry.
            DTIntArray theIndicies = storage.argumentLists(listNumber);
            ssize_t howMany = theIndicies.Length();
            toRet = DTMutableDoubleArray(howMany);
            ssize_t i;
            for (i=0;i<howMany;i++) {
                toRet(i) = storage.leafs(theIndicies(i)).Evaluate(v,storage);
            }
            break;
        }
        case Minus:
        {
            DTFunctionLeaf left = storage.leafs(leftReference);
            DTFunctionLeaf right = storage.leafs(rightReference);
            if (left.leafIsAList) {
                toRet = left.EvaluateArray(v,storage);
                if (right.leafIsAList) {
                    toRet -= right.EvaluateArray(v,storage);
                }
                else {
                    toRet -= right.Evaluate(v,storage);
                }
            }
            else {
                toRet = right.EvaluateArray(v,storage);
                toRet -= left.Evaluate(v,storage);
                toRet *= -1.0;
            }
            break;
        }
        case Plus:
        {
            DTFunctionLeaf left = storage.leafs(leftReference);
            DTFunctionLeaf right = storage.leafs(rightReference);
            if (left.leafIsAList) {
                toRet = left.EvaluateArray(v,storage);
                if (right.leafIsAList) {
                    toRet += right.EvaluateArray(v,storage);
                }
                else {
                    toRet += right.Evaluate(v,storage);
                }
            }
            else {
                toRet = right.EvaluateArray(v,storage);
                toRet += left.Evaluate(v,storage);
            }
            break;
        }
        case Times:
        {
            DTFunctionLeaf left = storage.leafs(leftReference);
            DTFunctionLeaf right = storage.leafs(rightReference);
            if (left.leafIsAList) {
                toRet = left.EvaluateArray(v,storage);
                if (right.leafIsAList) {
                    toRet *= right.EvaluateArray(v,storage);
                }
                else {
                    toRet *= right.Evaluate(v,storage);
                }
            }
            else {
                toRet = right.EvaluateArray(v,storage);
                toRet *= left.Evaluate(v,storage);
            }
            break;
        }
        case Divide:
        {
            DTFunctionLeaf left = storage.leafs(leftReference);
            DTFunctionLeaf right = storage.leafs(rightReference);
            if (left.leafIsAList) {
                toRet = left.EvaluateArray(v,storage);
                if (right.leafIsAList) {
                    toRet /= right.EvaluateArray(v,storage);
                }
                else {
                    toRet /= right.Evaluate(v,storage);
                }
            }
            else {
                toRet = left.Evaluate(v,storage)/right.EvaluateArray(v,storage);
            }
            break;
        }
        default:
            DTErrorMessage("DTFunctionLeaf::EvaluateArray","Not supported yet");
    }
    
    return toRet;
}

double DTFunctionLeaf::Evaluate(const double *v,const DTFunctionStorage &storage) const
{
    double toReturn;
    
    switch (type) {
        case Value:
            toReturn = value;
            break;
        case Constant:
            toReturn = v[constantNumber]; // Assumed to be valid.
            break;
        case Plus:
            toReturn = (storage.leafsD[leftReference].Evaluate(v,storage) +
                        storage.leafsD[rightReference].Evaluate(v,storage));
            break;
        case Minus:
            toReturn = (storage.leafsD[leftReference].Evaluate(v,storage) -
                        storage.leafsD[rightReference].Evaluate(v,storage));
            break;
        case Times:
            toReturn = (storage.leafsD[leftReference].Evaluate(v,storage) *
                        storage.leafsD[rightReference].Evaluate(v,storage));
            break;
        case Divide:
            toReturn = (storage.leafsD[leftReference].Evaluate(v,storage) /
                        storage.leafsD[rightReference].Evaluate(v,storage));
            break;
        case Power:
            toReturn = pow(storage.leafsD[leftReference].Evaluate(v,storage),
                           storage.leafsD[rightReference].Evaluate(v,storage));
            break;
        case PredefinedFunction:
        {
            if (storage.leafsD[leftReference].leafIsAList) {
                // List of numbers.  Only a few functions will allow this.
                DTDoubleArray theArr = storage.leafs(leftReference).EvaluateArray(v,storage);
                ssize_t i;
                ssize_t howMany = theArr.Length();
                switch (functionReference) {
                    case Min:
                    {
                        toReturn = theArr(0);
                        for (i=1;i<howMany;i++) {
                            if (toReturn>theArr(i)) toReturn = theArr(i);
                        }
                        break;
                    }
                    case Max:
                    {
                        toReturn = theArr(0);
                        for (i=1;i<howMany;i++) {
                            if (toReturn<theArr(i)) toReturn = theArr(i);
                        }
                        break;
                    }
                    case Norm:
                    {
                        toReturn = theArr(0)*theArr(0);
                        for (i=1;i<howMany;i++)
                            toReturn += theArr(i)*theArr(i);
                        toReturn = sqrt(toReturn);
                        break;
                    }
                    case SignSwitch:
                    {
                        if (howMany==3) {
                            if (theArr(0)<=0.0)
                                toReturn = theArr(1);
                            else
                                toReturn = theArr(2);
                        }
                        else {
                            if (theArr(0)<0.0)
                                toReturn = theArr(1);
                            else if (theArr(0)==0.0)
                                toReturn = theArr(2);
                            else
                                toReturn = theArr(3);
                        }
                        break;
                    }
                    case Angle:
                    {
                        toReturn = atan2(theArr(1),theArr(0));
                        break;
                    }
                    case Jn:
                    {
                        toReturn = jn(int(theArr(0)),theArr(1));
                        break;
                    }
                    case Yn:
                    {
                        toReturn = yn(int(theArr(0)),theArr(1));
                        break;
                    }
                    case Mod:
                    {
                        toReturn = fmod(theArr(0),theArr(1));
                        break;
                    }
                    default:
                        // Not supported for this function.
                        DTErrorMessage("DTFunctionLeaf::Evaluate","Not supported yet");
                        toReturn = 0.0;
                        break;
                }
            }
            else {
                // Single number
                double x = storage.leafsD[leftReference].Evaluate(v,storage);
                switch (functionReference) {
                    case Abs:      toReturn = fabs(x);       break;
                    case Sin:      toReturn = sin(x);        break;
                    case Cos:      toReturn = cos(x);        break;
                    case ASin:     toReturn = asin(x);       break;
                    case ACos:     toReturn = acos(x);       break;
                    case Tan:      toReturn = tan(x);        break;
                    case ATan:     toReturn = atan(x);       break;
                    case Sinh:     toReturn = sinh(x);       break;
                    case Cosh:     toReturn = cosh(x);       break;
                    case Tanh:     toReturn = tanh(x);       break;
                    case Sqrt:     toReturn = sqrt(x);       break;
                    case Gamma:    toReturn = tgamma(x);     break;
                    case LogGamma: toReturn = lgamma(x);     break;
                    case Exp:      toReturn = exp(x);        break;
                    case Log:      toReturn = log(x);        break;
                    case Log10:    toReturn = log10(x);      break;
                    case Log2:     toReturn = log2(x);       break;
                    case Erfc:     toReturn = erfc(x);       break;
                    case Erf:      toReturn = erf(x);        break;
                    case Floor:    toReturn = floor(x);      break;
                    case Ceil:     toReturn = ceil(x);       break;
                    case Round:    toReturn = round(x);      break;
                    case Fact:     toReturn = simpleFactorial(x);      break;
                    case Norm:     toReturn = fabs(x);       break;
                        
                    case J0:       toReturn = j0(x);         break;
                    case J1:       toReturn = j1(x);         break;
                    case Y0:       toReturn = y0(x);         break;
                    case Y1:       toReturn = y1(x);         break;
                    case Sinc:     toReturn = (x==0.0 ? 1.0 : sin(x)/x);  break;
                    case Rect:
                    {
                        if (x<-0.5)
                            toReturn = 0.0;
                        else if (x==-0.5)
                            toReturn = 0.5;
                        else if (x<0.5)
                            toReturn = 1.0;
                        else if (x==0.5)
                            toReturn = 0.5;
                        else 
                            toReturn = 0.0;
                        break;
                    }
                    case Tri:
                    {
                        if (x<-1.0)
                            toReturn = 0.0;
                        else if (x<1.0)
                            toReturn = 1.0-fabs(x);
                        else 
                            toReturn = 0.0;
                        break;
                    }
                    case H:
                    {
                        if (x<0.0)
                            toReturn = 0.0;
                        else if (x==0.0)
                            toReturn = 0.5;
                        else
                            toReturn = 1.0;
                        break;
                    }
                    case Sgn:
                    {
                        if (x<0.0)
                            toReturn = -1.0;
                        else if (x==0.0)
                            toReturn = 0.0;
                        else
                            toReturn = 1.0;
                        break;
                    }
                        
                    default:
                        // Identity,Min,Max all don't change the number
                        toReturn = 0.0;
                        break;
                }
            }
            break;
        }        
        case Function:
        {
            DTFunction foo = storage.SubFunction(functionNumber);
            if (storage.leafs(leftReference).leafIsAList) {
                // List of numbers.  Only a few functions will allow this.
                DTDoubleArray theArr = storage.leafs(leftReference).EvaluateArray(v,storage);
                toReturn = foo.Evaluate(theArr.Pointer());
            }
            else {
                toReturn = storage.leafs(leftReference).Evaluate(v,storage);
                toReturn = foo.Evaluate(&toReturn);
            }
            break;
        }
        default:
            DTErrorMessage("DTFunctionLeaf::Evaluate","Not supported yet");
            toReturn = 0.0;
    }

    return toReturn;
}

void DTFunctionLeaf::outputIntoStream(std::ostream &os,const DTFunctionStorage &storage) const
{
    DTFunctionLeaf left,right;
    
    bool encloseParenthesis = true;
    switch (type) {
        case Value:
            os << value;
            break;
        case Constant:
            os << storage.constantNames(constantNumber);
            break;
        case Plus:
        {
            encloseParenthesis = (storage.leafs(leftReference).leafIsAList);
            if (encloseParenthesis) os << "(";
            storage.leafs(leftReference).outputIntoStream(os,storage);
            if (encloseParenthesis) os << ")";
            os << " + ";
            encloseParenthesis = (storage.leafs(rightReference).leafIsAList);
            if (encloseParenthesis) os << "(";
            storage.leafs(rightReference).outputIntoStream(os,storage);
            if (encloseParenthesis) os << ")";
            break;
        }
        case Minus:
        {
            encloseParenthesis = (storage.leafs(leftReference).leafIsAList);
            if (encloseParenthesis) os << "(";
            storage.leafs(leftReference).outputIntoStream(os,storage);
            if (encloseParenthesis) os << ")";
            os << " - ";
            encloseParenthesis = (storage.leafs(rightReference).type==Plus ||
                                  storage.leafs(rightReference).type==Minus);
            if (storage.leafs(rightReference).leafIsAList) encloseParenthesis = true;
            if (encloseParenthesis) os << "(";
            storage.leafs(rightReference).outputIntoStream(os,storage);
            if (encloseParenthesis) os << ")";
            break;
        }
        case Times:
        {
            left = storage.leafs(leftReference);
            right = storage.leafs(rightReference);
            if (left.type==PredefinedFunction) encloseParenthesis = false;
            if (left.type==Power) encloseParenthesis = false;
            if (left.type==Function) encloseParenthesis = false;
            if (left.type==Value && left.value>=0.0) encloseParenthesis = false;
            if (left.type==Constant) encloseParenthesis = false;
            if (encloseParenthesis) os << "(";
            left.outputIntoStream(os,storage);
            if (encloseParenthesis) os << ")";
            os << "*";
            encloseParenthesis = true;
            if (right.type==PredefinedFunction) encloseParenthesis = false;
            if (right.type==Power) encloseParenthesis = false;
            if (right.type==Function) encloseParenthesis = false;
            if (right.type==Value && right.value>=0.0) encloseParenthesis = false;
            if (right.type==Constant) encloseParenthesis = false;
            if (encloseParenthesis) os << "(";
            right.outputIntoStream(os,storage);
            if (encloseParenthesis) os << ")";
        }
            break;
        case Divide:
        {
            left = storage.leafs(leftReference);
            right = storage.leafs(rightReference);
            if (left.type==PredefinedFunction || left.type==Power || left.type==Function || left.type==Constant) encloseParenthesis = false;
            if (left.type==Value && left.value>=0.0) encloseParenthesis = false;
            if (encloseParenthesis) os << "(";
            left.outputIntoStream(os,storage);
            if (encloseParenthesis) os << ")";
            os << "/";
            encloseParenthesis = true;
            if (right.type==PredefinedFunction || right.type==Power || right.type==Function || right.type==Constant) encloseParenthesis = false;
            if (right.type==Value && right.value>=0.0) encloseParenthesis = false;
            if (encloseParenthesis) os << "(";
            right.outputIntoStream(os,storage);
            if (encloseParenthesis) os << ")";
        }
            break;
        case Power:
        {
            left = storage.leafs(leftReference);
            right = storage.leafs(rightReference);
            if (left.type==PredefinedFunction || left.type==Function || left.type==Constant) encloseParenthesis = false;
            if (left.type==Value && left.value>=0.0) encloseParenthesis = false;
            if (encloseParenthesis) os << "(";
            left.outputIntoStream(os,storage);
            if (encloseParenthesis) os << ")";
            os << "^";
            encloseParenthesis = true;
            if (right.type==PredefinedFunction || right.type==Function || right.type==Constant) encloseParenthesis = false;
            if (right.type==Value && right.value>=0.0) encloseParenthesis = false;
            if (encloseParenthesis) os << "(";
            right.outputIntoStream(os,storage);
            if (encloseParenthesis) os << ")";
        }
            break;
            
        case Or:
        {
            left = storage.leafs(leftReference);
            right = storage.leafs(rightReference);
            if (encloseParenthesis) os << "(";
            left.outputIntoStream(os,storage);
            if (encloseParenthesis) os << ")";
            os << " || ";
            encloseParenthesis = true;
            if (encloseParenthesis) os << "(";
            right.outputIntoStream(os,storage);
            if (encloseParenthesis) os << ")";
        }
            break;
            
        case And:
        {
            left = storage.leafs(leftReference);
            right = storage.leafs(rightReference);
            if (encloseParenthesis) os << "(";
            left.outputIntoStream(os,storage);
            if (encloseParenthesis) os << ")";
            os << " && ";
            encloseParenthesis = true;
            if (encloseParenthesis) os << "(";
            right.outputIntoStream(os,storage);
            if (encloseParenthesis) os << ")";
        }
            break;
            
        case NotEqual:
        {
            left = storage.leafs(leftReference);
            right = storage.leafs(rightReference);
            if (encloseParenthesis) os << "(";
            left.outputIntoStream(os,storage);
            if (encloseParenthesis) os << ")";
            os << " != ";
            encloseParenthesis = true;
            if (encloseParenthesis) os << "(";
            right.outputIntoStream(os,storage);
            if (encloseParenthesis) os << ")";
        }
            break;
            
        case Equal:
        {
            left = storage.leafs(leftReference);
            right = storage.leafs(rightReference);
            if (encloseParenthesis) os << "(";
            left.outputIntoStream(os,storage);
            if (encloseParenthesis) os << ")";
            os << " == ";
            encloseParenthesis = true;
            if (encloseParenthesis) os << "(";
            right.outputIntoStream(os,storage);
            if (encloseParenthesis) os << ")";
        }
            break;
            
        case Less:
        {
            left = storage.leafs(leftReference);
            right = storage.leafs(rightReference);
            if (encloseParenthesis) os << "(";
            left.outputIntoStream(os,storage);
            if (encloseParenthesis) os << ")";
            os << "<";
            encloseParenthesis = true;
            if (encloseParenthesis) os << "(";
            right.outputIntoStream(os,storage);
            if (encloseParenthesis) os << ")";
        }
            break;
            
        case Greater:
        {
            left = storage.leafs(leftReference);
            right = storage.leafs(rightReference);
            if (encloseParenthesis) os << "(";
            left.outputIntoStream(os,storage);
            if (encloseParenthesis) os << ")";
            os << ">";
            encloseParenthesis = true;
            if (encloseParenthesis) os << "(";
            right.outputIntoStream(os,storage);
            if (encloseParenthesis) os << ")";
        }
            break;
            
        case LessEqual:
        {
            left = storage.leafs(leftReference);
            right = storage.leafs(rightReference);
            if (encloseParenthesis) os << "(";
            left.outputIntoStream(os,storage);
            if (encloseParenthesis) os << ")";
            os << "<=";
            encloseParenthesis = true;
            if (encloseParenthesis) os << "(";
            right.outputIntoStream(os,storage);
            if (encloseParenthesis) os << ")";
        }
            break;
            
        case GreaterEqual:
        {
            left = storage.leafs(leftReference);
            right = storage.leafs(rightReference);
            if (encloseParenthesis) os << "(";
            left.outputIntoStream(os,storage);
            if (encloseParenthesis) os << ")";
            os << ">=";
            encloseParenthesis = true;
            if (encloseParenthesis) os << "(";
            right.outputIntoStream(os,storage);
            if (encloseParenthesis) os << ")";
        }
            break;
            
        case PredefinedFunction:
        {
            switch (functionReference) {
                case Abs:        os << "abs";        break;
                case Sin:        os << "sin";        break;
                case Cos:        os << "cos";        break;
                case ASin:       os << "asin";       break;
                case ACos:       os << "acos";       break;
                case Tan:        os << "tan";        break;
                case ATan:       os << "atan";       break;
                case Sinh:       os << "sinh";       break;
                case Cosh:       os << "cosh";       break;
                case Tanh:       os << "tanh";       break;
                case Sqrt:       os << "sqrt";       break;
                case Gamma:      os << "gamma";      break;
                case LogGamma:   os << "loggamma";   break;
                case Exp:        os << "exp";        break;
                case Log:        os << "log";        break;
                case Log10:      os << "log10";      break;
                case Log2:       os << "log2";       break;
                case Erfc:       os << "erfc";       break;
                case Erf:        os << "erf";        break;
                case Floor:      os << "floor";      break;
                case Ceil:       os << "ceil";       break;
                case Round:      os << "round";      break;
                case Fact:       os << "fact";       break;
                case Min:        os << "min";        break;
                case Max:        os << "max";        break;
                case Norm:       os << "norm";       break;
                case SignSwitch: os << "signswitch"; break;
                case Angle:      os << "angle";      break;
                case J0:         os << "j0";         break;
                case J1:         os << "j1";         break;
                case Jn:         os << "jn";         break;
                case Y0:         os << "y0";         break;
                case Y1:         os << "y1";         break;
                case Yn:         os << "yn";         break;
                case Mod:        os << "mod";        break;
                case Sinc:       os << "sinc";       break;
                case Rect:       os << "rect";       break;
                case Tri:        os << "tri";        break;
                case H:          os << "H";          break;
                case Sgn:        os << "sgn";        break;
                // case Seconds:    os << "seconds";    break;
                case Year:       os << "year";       break;
                case Month:      os << "month";      break;
                case Day:        os << "day";        break;
                case Hour:       os << "hour";       break;
                case Minute:     os << "minute";     break;
                case IsAM:       os << "isAm";       break;
                case IsPM:       os << "isPM";       break;
                case Log1p:      os << "log1p";      break;
                case Rem:        os << "rem";        break;
                case Cbrt:       os << "cbrt";       break;
                case If:         os << "if";         break;
                case Isnan:      os << "isnan";      break;
                case Isfinite:   os << "isfinite";   break;

                case Identity:                       break;
            }
            os << "(";
            storage.leafs(leftReference).outputIntoStream(os,storage);
            os << ")";
            break;
        }
        case ListOfValues:
        {
            DTIntArray starts = storage.argumentLists(listNumber);
            ssize_t howMany = starts.Length();
            ssize_t i;
            for (i=0;i<howMany;i++) {
                storage.leafs(starts(i)).outputIntoStream(os,storage);
                if (i<howMany-1) os << ",";
            }
            break;
        }
        case Function:
        {
            os << "f";
            os << (functionNumber+1);
            os << "(";
            storage.leafs(leftReference).outputIntoStream(os,storage);
            os << ")";
            break;
        }
        default:
            DTErrorMessage("stream << function","Not done");
            os << "(not done)";
            break;
    }
}

DTFunctionStorage::DTFunctionStorage(const DTList<DTFunctionLeaf> &entries)
 : subFunctions(NULL), howManySubFunctions(0)
{
    subFunctions = NULL;
    howManySubFunctions = 0;
    leafs = entries;
    leafsD = leafs.Pointer();
}

DTFunctionStorage::~DTFunctionStorage()
{
    if (subFunctions) {
        delete [] subFunctions;
        subFunctions = NULL;
    }
}

DTFunction DTFunctionStorage::SubFunction(ssize_t i) const
{
    if (i<0 || i>=howManySubFunctions) 
        return DTFunction();
    else
        return subFunctions[i];
}

void DTFunctionStorage::SetSubFunctions(const DTFunction *ptr,ssize_t howMany)
{
    if (subFunctions) {
        delete [] subFunctions;
        subFunctions = NULL;
    }
    howManySubFunctions = howMany;
    if (howManySubFunctions) {
        subFunctions = new DTFunction[howManySubFunctions];
        int i;
        for (i=0;i<howManySubFunctions;i++) {
            subFunctions[i] = ptr[i];
        }
    }
}

bool DTFunctionStorage::CheckValidity(void) const
{
    // Consistency checks.  The function is stored as several interconnected lists, and this is a way to see if the structure is valid.
    
    ssize_t howManyLeafs = leafs.Length();
    ssize_t howManyLists = argumentLists.Length();
    ssize_t howManyConstants = constantNames.Length();
    ssize_t howManyFunctions = howManySubFunctions;
    ssize_t leafN,listNumber;
    DTFunctionLeaf singleLeaf,argumentLeaf;
    bool toReturn = true;
    DTMutableIntArray haveUsedLeafs(howManyLeafs);
    haveUsedLeafs = 0;
    haveUsedLeafs(0)++;
    DTMutableIntArray haveUsedLists(howManyLists);
    haveUsedLists = 0;
    DTIntArray singleList;
    
    for (leafN=0;leafN<howManyLeafs;leafN++) {
        singleLeaf = leafs(leafN);
        
        switch (singleLeaf.type) {
            case DTFunctionLeaf::Value:
                if (singleLeaf.listNumber!=-1 || singleLeaf.constantNumber!=-1 || singleLeaf.functionNumber!=-1 || singleLeaf.functionReference!=DTFunctionLeaf::Identity
                    || singleLeaf.leftReference!=0 || singleLeaf.rightReference!=0) {
#ifndef DG_NOSTDErrOut
                    std::cerr << "Leaf " << leafN << " not properly initialized for a value" << std::endl;
#endif
                    toReturn = false;
                }
                break;
            case DTFunctionLeaf::ListOfValues:
                if (singleLeaf.constantNumber!=-1 || singleLeaf.functionNumber!=-1 || singleLeaf.functionReference!=DTFunctionLeaf::Identity
                    || singleLeaf.leftReference!=0 || singleLeaf.rightReference!=0) {
#ifndef DG_NOSTDErrOut
                    std::cerr << "Leaf " << leafN << " not properly initialized for a list of values" << std::endl;
#endif
                    toReturn = false;
                }
                if (singleLeaf.listNumber<0 || singleLeaf.listNumber>=howManyLists) {
#ifndef DG_NOSTDErrOut
                    std::cerr << "Leaf " << leafN << " points to a list out of bounds" << std::endl;
#endif
                    toReturn = false;
                }
                else {
                    haveUsedLists(singleLeaf.listNumber)++;
                }
                break;
            case DTFunctionLeaf::Constant:
                if (singleLeaf.listNumber!=-1 || singleLeaf.functionNumber!=-1 || singleLeaf.functionReference!=DTFunctionLeaf::Identity
                    || singleLeaf.leftReference!=0 || singleLeaf.rightReference!=0) {
#ifndef DG_NOSTDErrOut
                    std::cerr << "Leaf " << leafN << " not properly initialized for a constant" << std::endl;
#endif
                    toReturn = false;
                }
                if (singleLeaf.constantNumber<0 || singleLeaf.constantNumber>=howManyConstants) {
#ifndef DG_NOSTDErrOut
                    std::cerr << "Leaf " << leafN << " points to a constant out of bounds" << std::endl;
#endif
                    toReturn = false;
                }
                break;
            case DTFunctionLeaf::Plus: 
            case DTFunctionLeaf::Minus:
            case DTFunctionLeaf::Times:
            case DTFunctionLeaf::Divide:
            case DTFunctionLeaf::Power:
            case DTFunctionLeaf::Or:
            case DTFunctionLeaf::And:
            case DTFunctionLeaf::NotEqual:
            case DTFunctionLeaf::Equal:
            case DTFunctionLeaf::Less:
            case DTFunctionLeaf::Greater:
            case DTFunctionLeaf::LessEqual:
            case DTFunctionLeaf::GreaterEqual:
                if (singleLeaf.listNumber!=-1 || singleLeaf.constantNumber!=-1 || singleLeaf.functionNumber!=-1 || singleLeaf.functionReference!=DTFunctionLeaf::Identity
                    || singleLeaf.leftReference<=0 || singleLeaf.rightReference<=0) {
                    toReturn = false;
#ifndef DG_NOSTDErrOut
                    std::cerr << "Leaf " << leafN << " not properly initialized for a binary operator" << std::endl;
#endif
                }
                if (singleLeaf.leftReference<=leafN || singleLeaf.rightReference<=leafN || singleLeaf.leftReference>=howManyLeafs || singleLeaf.rightReference>=howManyLeafs) {
#ifndef DG_NOSTDErrOut
                    std::cerr << "Leaf " << leafN << " is a binary operator that points to leafs out of bounds." << std::endl;
#endif
                    toReturn = false;
                }
                else {
                    haveUsedLeafs(singleLeaf.leftReference)++;
                    haveUsedLeafs(singleLeaf.rightReference)++;
                }
                break;
            case DTFunctionLeaf::PredefinedFunction:
                if (singleLeaf.listNumber!=-1 || singleLeaf.constantNumber!=-1 || singleLeaf.functionNumber!=-1) {
#ifndef DG_NOSTDErrOut
                    std::cerr << "Leaf " << leafN << " not properly initialized for a predefined function" << std::endl;
#endif
                    toReturn = false;
                }
                if (singleLeaf.leftReference<=0 || singleLeaf.leftReference>=howManyLeafs) {
#ifndef DG_NOSTDErrOut
                    std::cerr << "Leaf " << leafN << " has a left reference out of bounds" << std::endl;
#endif
                    toReturn = false;
                }
                else {
                    // Check to make sure that the reference is valid
                    haveUsedLeafs(singleLeaf.leftReference)++;
                    argumentLeaf = leafs(singleLeaf.leftReference);
                    switch (singleLeaf.functionReference) {
                        case DTFunctionLeaf::Min:
                        case DTFunctionLeaf::Max:
                        case DTFunctionLeaf::Norm:
                            // Can take any number of arguments
                            if (argumentLeaf.type!=DTFunctionLeaf::ListOfValues) {
#ifndef DG_NOSTDErrOut
                                std::cerr << "Leaf " << leafN << " is a function that requires multiple arguments, and is not handed a list." << std::endl;
#endif
                                toReturn = false;
                            }
                            else {
                                listNumber = argumentLeaf.listNumber;
                                if (listNumber<0 || listNumber>=howManyLists) {
#ifndef DG_NOSTDErrOut
                                    std::cerr << "Leaf " << leafN << " refers to an entry that is an invalid list" << std::endl;
#endif
                                    toReturn = false;
                                }
                            }
                            break;
                        case DTFunctionLeaf::SignSwitch:
                            // Take 3 or 4 arguments
                            if (argumentLeaf.type!=DTFunctionLeaf::ListOfValues) {
#ifndef DG_NOSTDErrOut
                                std::cerr << "Leaf " << leafN << " is a function that requires multiple arguments, and is not handed a list." << std::endl;
#endif
                                toReturn = false;
                            }
                            else {
                                listNumber = argumentLeaf.listNumber;
                                if (listNumber<0 || listNumber>=howManyLists) {
#ifndef DG_NOSTDErrOut
                                    std::cerr << "Leaf " << leafN << " refers to an entry that is an invalid list" << std::endl;
#endif
                                    toReturn = false;
                                }
                                else {
                                    singleList = argumentLists(listNumber);
                                    if (singleList.Length()!=3 && singleList.Length()!=4) {
#ifndef DG_NOSTDErrOut
                                        std::cerr << "Leaf " << leafN << " refers to an argument list that has an invalid length." << std::endl;
#endif
                                        toReturn = false;
                                    }
                                }
                            }
                            break;
                        case DTFunctionLeaf::If:
                            // Take 3
                            if (argumentLeaf.type!=DTFunctionLeaf::ListOfValues) {
#ifndef DG_NOSTDErrOut
                                std::cerr << "Leaf " << leafN << " is a function that requires multiple arguments, and is not handed a list." << std::endl;
#endif
                                toReturn = false;
                            }
                            else {
                                listNumber = argumentLeaf.listNumber;
                                if (listNumber<0 || listNumber>=howManyLists) {
#ifndef DG_NOSTDErrOut
                                    std::cerr << "Leaf " << leafN << " refers to an entry that is an invalid list" << std::endl;
#endif
                                    toReturn = false;
                                }
                                else {
                                    singleList = argumentLists(listNumber);
                                    if (singleList.Length()!=3) {
#ifndef DG_NOSTDErrOut
                                        std::cerr << "Leaf " << leafN << " refers to an argument list that has an invalid length." << std::endl;
#endif
                                        toReturn = false;
                                    }
                                }
                            }
                            break;
                        case DTFunctionLeaf::Jn:
                        case DTFunctionLeaf::Yn:
                        case DTFunctionLeaf::Angle:
                        case DTFunctionLeaf::Mod:
                            // Need 2 arguments
                            if (argumentLeaf.type!=DTFunctionLeaf::ListOfValues) {
#ifndef DG_NOSTDErrOut
                                std::cerr << "Leaf " << leafN << " is a function that requires multiple arguments, and is not handed a list." << std::endl;
#endif
                                toReturn = false;
                            }
                            else {
                                listNumber = argumentLeaf.listNumber;
                                if (listNumber<0 || listNumber>=howManyLists) {
#ifndef DG_NOSTDErrOut
                                    std::cerr << "Leaf " << leafN << " refers to an entry that is an invalid list" << std::endl;
#endif
                                    toReturn = false;
                                }
                                else {
                                    singleList = argumentLists(listNumber);
                                    if (singleList.Length()!=2) {
#ifndef DG_NOSTDErrOut
                                        std::cerr << "Leaf " << leafN << " refers to an argument list that has an invalid length." << std::endl;
#endif
                                        toReturn = false;
                                    }
                                }
                            }
                            break;
                        default:
                            // Only take in 1 argument, can not be a list
                            if (argumentLeaf.type==DTFunctionLeaf::ListOfValues) {
#ifndef DG_NOSTDErrOut
                                std::cerr << "Leaf " << leafN << " is a function of one argument that takes in a list of arguments" << std::endl;
#endif
                                toReturn = false;
                            }
                            break;
                    }
                }
                break;
            case DTFunctionLeaf::Function:
                if (singleLeaf.listNumber!=-1 || singleLeaf.constantNumber!=-1 || singleLeaf.functionReference!=DTFunctionLeaf::Identity || singleLeaf.rightReference!=0) {
#ifndef DG_NOSTDErrOut
                    std::cerr << "Leaf " << leafN << " not properly initialized for a function object" << std::endl;
#endif
                    toReturn = false;
                }
                if (singleLeaf.leftReference<=0 || singleLeaf.leftReference>=howManyLeafs) {
#ifndef DG_NOSTDErrOut
                    std::cerr << "Leaf " << leafN << " has a function argument out of bounds" << std::endl;
#endif
                    toReturn = false;
                }
                else {
                    haveUsedLeafs(singleLeaf.leftReference)++;
                }
                if (singleLeaf.functionNumber<0 || singleLeaf.functionNumber>=howManyFunctions) {
#ifndef DG_NOSTDErrOut
                    std::cerr << "Leaf " << leafN << " refers to a function out of bounds" << std::endl;
#endif
                    toReturn = false;
                }
                else if (singleLeaf.leftReference>0 || singleLeaf.leftReference<howManyLeafs) {
                    // Check to see if the number of arguments matches.
                    DTFunction fcn = SubFunction(singleLeaf.functionNumber);
                    ssize_t howManyArgs = fcn.ConstantNames().Length();
                    argumentLeaf = leafs(singleLeaf.leftReference);
                    if (howManyArgs==1) {
                        // Needs to be a single argument, and not a list
                        if (argumentLeaf.type==DTFunctionLeaf::ListOfValues) {
#ifndef DG_NOSTDErrOut
                            std::cerr << "Leaf " << leafN << " is a function of one argument that takes in a list of arguments" << std::endl;
#endif
                            toReturn = false;
                        }
                        else {
                            
                        }
                    }
                    else {
                        // The list has to have the same number
                        if (argumentLeaf.type==DTFunctionLeaf::ListOfValues) {
                            listNumber = argumentLeaf.listNumber;
                            if (listNumber<0 || listNumber>=howManyLists) {
#ifndef DG_NOSTDErrOut
                                std::cerr << "Leaf " << leafN << " refers to an entry that is an invalid list" << std::endl;
#endif
                                toReturn = false;
                            }
                            else {
                                singleList = argumentLists(listNumber);
                                if (singleList.Length()!=howManyArgs) {
#ifndef DG_NOSTDErrOut
                                    std::cerr << "Leaf " << leafN << " refers to an argument list that has an invalid length." << std::endl;
#endif
                                    toReturn = false;
                                }
                            }
                        }
                        else {
#ifndef DG_NOSTDErrOut
                            std::cerr << "Leaf " << leafN << " needs to refer to an argument list" << std::endl;
#endif
                            toReturn = false;
                        }
                    }
                }
                
                break;
        }
    }
    
    // Check the argument list.  Refers to entries in the leaf array
    ssize_t listN,i,howMany;
    for (listN=0;listN<howManyLists;listN++) {
        singleList = argumentLists(listN);
        howMany = singleList.Length();
        for (i=0;i<howMany;i++) {
            if (singleList(i)<=0 || singleList(i)>=howManyLeafs) {
#ifndef DG_NOSTDErrOut
                std::cerr << "Entry " << i <<  " << in list " << listN << " points to an entry out of bounds" << std::endl;
#endif
                toReturn = false;
            }
            else {
                haveUsedLeafs(singleList(i))++;
            }
        }
    }
    
    if (Minimum(haveUsedLeafs)<1) {
#ifndef DG_NOSTDErrOut
        std::cerr << "One of the leafs is not referenced." << std::endl;
#endif
        toReturn = false;
    }
    if (Maximum(haveUsedLeafs)>1) {
#ifndef DG_NOSTDErrOut
        std::cerr << "One of the leafs is referenced more than once." << std::endl;
#endif
        toReturn = false;
    }
    
    return toReturn;
}

std::ostream &operator<<(std::ostream &os,const DTFunctionStorage &stor)
{
    if (stor.leafs.Length()==0)
        return os; // Empty function
    
    stor.leafs(0).outputIntoStream(os,stor);
    
    if (stor.HowManySubFunctions()) {
        ssize_t howManyFunctions = stor.HowManySubFunctions();
        ssize_t i,j,howManyConstants;
        DTFunction foo;
        DTList<std::string> constants;
        os << " {";
        for (i=0;i<howManyFunctions;i++) {
            foo = stor.SubFunction(i);
            constants = foo.ConstantNames();
            howManyConstants = constants.Length();
            os << "f" << (i+1) << "(";
            for (j=0;j<howManyConstants;j++) {
                os << constants(j);
                if (j<howManyConstants-1) os << ",";
            }
            os << ") = ";
            os << foo;
            if (i<howManyFunctions-1)
                os << " & ";
        }
        os << "} ";
    }
    return os;
}

DTFunction::DTFunction(const DTPointer<DTFunctionStorage> &C)
: content(C) 
{
}

DTFunction::DTFunction(std::string name)
{
    DTMutableList<DTFunctionLeaf> leafs(1);
    
    leafs(0).type = DTFunctionLeaf::Constant;
    leafs(0).constantNumber = 0;
    
    DTMutablePointer<DTFunctionStorage> stor(new DTFunctionStorage(leafs));
    DTMutableList<std::string> constNames(1);
    constNames(0) = name;
    stor->constantNames = constNames;
    
    content = stor;
}

DTFunction operator+(const DTFunction &A,const DTFunction &B)
{
    return BinaryOperator(DTFunctionLeaf::Plus,*A.Content(),*B.Content());
}

DTFunction operator-(const DTFunction &A,const DTFunction &B)
{
    return BinaryOperator(DTFunctionLeaf::Minus,*A.Content(),*B.Content());
}

DTFunction operator*(const DTFunction &A,const DTFunction &B)
{
    return BinaryOperator(DTFunctionLeaf::Times,*A.Content(),*B.Content());
}

DTFunction operator/(const DTFunction &A,const DTFunction &B)
{
    return BinaryOperator(DTFunctionLeaf::Divide,*A.Content(),*B.Content());
}

DTFunction DTFunction::power(const DTFunction &B) const
{
    return BinaryOperator(DTFunctionLeaf::Power,*content,*B.content);
}

string DTFunction::StringVersion(void) const
{
    stringstream output(stringstream::in | stringstream::out);
    output << *content;
    return output.str();
}

void DTFunction::pinfo(void) const
{
#ifndef DG_NOSTDErrOut
    std::cerr << *content << std::endl << std::flush;
#endif
}

std::ostream &operator<<(std::ostream &os,const DTFunction &F)
{
    os << *F.Content();
    return os;
}

DTFunction DTFunction::Value(double val)
{
    DTMutableList<DTFunctionLeaf> leafs(1);

    leafs(0).value = val;

    DTMutablePointer<DTFunctionStorage> stor(new DTFunctionStorage(leafs));

    return DTFunction(stor);
}

DTFunction DTFunction::Constant(string name)
{
    DTMutableList<DTFunctionLeaf> leafs(1);

    leafs(0).type = DTFunctionLeaf::Constant;
    leafs(0).constantNumber = 0;

    DTMutablePointer<DTFunctionStorage> stor(new DTFunctionStorage(leafs));
    DTMutableList<std::string> constNames(1);
    constNames(0) = name;
    stor->constantNames = constNames;

    return DTFunction(stor);
}

DTFunction DTFunction::ChangeConstantOrder(const DTList<std::string> &newList) const
{
    DTList<std::string> currentList = ConstantNames();

    // Need to make sure that all of the current names exist in the new list.
    ssize_t lenCurrent = currentList.Length();
    ssize_t lenNew = newList.Length();

    DTMutableIntArray currentIntoNew(lenCurrent);
    ssize_t posCurrent, posNew;
    for (posCurrent=0;posCurrent<lenCurrent;posCurrent++) {
        for (posNew=0;posNew<lenNew;posNew++) {
            if (currentList(posCurrent)==newList(posNew)) {
                currentIntoNew(posCurrent) = int(posNew);
                break;
            }
        }
        if (posNew==lenNew)
            break; // Did not find one of the current entry in the new list.  This is an error
    }
    if (posCurrent<lenCurrent) {
        DTErrorMessage("DTFunction::ChangeConstantOrder","New list needs to include existing unknowns)");
        return DTFunction();
    }
    
    for (posCurrent=0;posCurrent<lenCurrent;posCurrent++) {
        if (currentIntoNew(posCurrent)!=posCurrent) break;
    }
    if (posCurrent==lenCurrent && lenNew==lenCurrent) {
        // Nothing changed.
        return *this;
    }
    
    // Now go over the leafs, and change all the constant references.
    DTList<DTFunctionLeaf> currentLeafs = content->leafs;
    ssize_t howManyLeafs = currentLeafs.Length();
    DTMutableList<DTFunctionLeaf> newLeafs(howManyLeafs);
    DTFunctionLeaf singleLeaf;
    ssize_t pos;
    for (pos=0;pos<howManyLeafs;pos++) {
        singleLeaf = currentLeafs(pos);
        if (singleLeaf.type==DTFunctionLeaf::Constant) {
            singleLeaf.constantNumber = currentIntoNew(singleLeaf.constantNumber);
        }
        newLeafs(pos) = singleLeaf;
    }

    // Now copy everything.
    DTMutablePointer<DTFunctionStorage> toReturn(new DTFunctionStorage());
    
    toReturn->leafs = newLeafs;
    toReturn->leafsD = newLeafs.Pointer();
    toReturn->constantNames = newList;
    toReturn->argumentLists = content->argumentLists;
    if (content->HowManySubFunctions()) {
        ssize_t howMany = content->HowManySubFunctions();
        DTMutableList<DTFunction> subFunctions(howMany);
        for (pos=0;pos<howMany;pos++)
            subFunctions(pos) = content->SubFunction(pos);
        toReturn->SetSubFunctions(subFunctions.Pointer(),subFunctions.Length());
    }
    // Precompute the leaf information
    newLeafs(0).ComputeIfThisLeafIsAList(newLeafs);

    return DTFunction(toReturn);
}

DTFunction DTFunction::InsertConstants(const DTDictionary &constants) const
{
    // Remove the constants that are defined in the dictionary.
    // Go over the leafs and change constant leafs to value leafs.
    
    DTList<std::string> currentList = ConstantNames();
    ssize_t lenCurrent = currentList.Length();
    DTMutableList<std::string> newList(lenCurrent);
    ssize_t i,posInNew = 0;
    
    DTMutableIntArray currentIntoNew(lenCurrent);
    for (i=0;i<lenCurrent;i++) {
        if (constants.TypeOf(currentList(i))==DTDictionary::Number) {
            currentIntoNew(i) = -1;
        }
        else {
            currentIntoNew(i) = int(posInNew);
            newList(posInNew) = currentList(i);
            posInNew++;
        }
    }
    if (posInNew==lenCurrent) return *this; // No change
    
    newList = TruncateSize(newList,posInNew);
    
    // Now go over the leafs, and change all the constant references.
    DTList<DTFunctionLeaf> currentLeafs = content->leafs;
    ssize_t howManyLeafs = currentLeafs.Length();
    DTMutableList<DTFunctionLeaf> newLeafs(howManyLeafs);
    DTFunctionLeaf singleLeaf;
    ssize_t pos;
    for (pos=0;pos<howManyLeafs;pos++) {
        singleLeaf = currentLeafs(pos);
        if (singleLeaf.type==DTFunctionLeaf::Constant) {
            if (currentIntoNew(singleLeaf.constantNumber)==-1) {
                // Change the type into a value
                singleLeaf.type=DTFunctionLeaf::Value;
                singleLeaf.value = constants(currentList(singleLeaf.constantNumber));
                singleLeaf.constantNumber = -1;
            }
            else {
                // Continue it, but the location changed
                singleLeaf.constantNumber = currentIntoNew(singleLeaf.constantNumber);
            }
        }
        newLeafs(pos) = singleLeaf;
    }
    
    // Now copy everything.
    DTMutablePointer<DTFunctionStorage> toReturn(new DTFunctionStorage());
    
    toReturn->leafs = newLeafs;
    toReturn->leafsD = newLeafs.Pointer();
    toReturn->constantNames = newList;
    toReturn->argumentLists = content->argumentLists;
    if (content->HowManySubFunctions()) {
        ssize_t howMany = content->HowManySubFunctions();
        DTMutableList<DTFunction> subFunctions(howMany);
        for (pos=0;pos<howMany;pos++)
            subFunctions(pos) = content->SubFunction(pos);
        toReturn->SetSubFunctions(subFunctions.Pointer(),subFunctions.Length());
    }
    // Precompute the leaf information
    newLeafs(0).ComputeIfThisLeafIsAList(newLeafs);

    return DTFunction(toReturn);
}

void Read(const DTDataStorage &input,const std::string &name,DTFunction &var)
{
    int i;
    DTDoubleArray leafArray;
    Read(input,name,leafArray);
    if (leafArray.IsEmpty()) {
        var = DTFunction();
        return;
    }
    
    // Unknown constants
    ssize_t howManyConstants = input.ReadInt(name+"_uC");
    DTMutableList<std::string> unknownNames(howManyConstants);
    for (i=0;i<howManyConstants;i++) {
        unknownNames(i) = input.ReadString(name+"_u"+DTInt2String(i+1));
    }
    
    // Arguments
    ssize_t howManyLists = input.ReadInt(name+"_lC");
    DTMutableList<DTIntArray> argumentLists(howManyLists);
    for (i=0;i<howManyLists;i++) {
        argumentLists(i) = input.ReadIntArray(name+"_l"+DTInt2String(i+1));
    }
    
    // Other functions
    ssize_t howManyFunctions = input.ReadInt(name+"_fC");
    DTMutableList<DTFunction> subFunctions(howManyFunctions);
    for (i=0;i<howManyFunctions;i++) {
        Read(input,name+"_f"+DTInt2String(i+1),subFunctions(i));
    }
    
    if (leafArray.m()!=8 || leafArray.o()!=1) {
        DTErrorMessage("Read(storage,name,function)","Invalid array size.");
        return;
    }
    ssize_t howManyLeafs = leafArray.n();
    ssize_t leafsToCheck;
    
    DTMutableList<DTFunctionLeaf> leafs(howManyLeafs);
    DTFunctionLeaf singleLeaf;
    singleLeaf.type = DTFunctionLeaf::Value;
    std::string errorMsg;
    DTFunctionLeaf::FunctionName functionName;
    for (i=0;i<howManyLeafs;i++) {
        leafsToCheck = 0;
        switch (int(leafArray(0,i))) {
            case 0:  singleLeaf.type = DTFunctionLeaf::Value;                                break;
            case 1:  singleLeaf.type = DTFunctionLeaf::ListOfValues;       leafsToCheck = 1; break;
            case 2:  singleLeaf.type = DTFunctionLeaf::Constant;                             break;
            case 3:  singleLeaf.type = DTFunctionLeaf::Plus;               leafsToCheck = 2; break;
            case 4:  singleLeaf.type = DTFunctionLeaf::Minus;              leafsToCheck = 2; break;
            case 5:  singleLeaf.type = DTFunctionLeaf::Times;              leafsToCheck = 2; break;
            case 6:  singleLeaf.type = DTFunctionLeaf::Divide;             leafsToCheck = 2; break;
            case 7:  singleLeaf.type = DTFunctionLeaf::Power;              leafsToCheck = 2; break;
            case 8:  singleLeaf.type = DTFunctionLeaf::PredefinedFunction; leafsToCheck = 1; break;
            case 9:  singleLeaf.type = DTFunctionLeaf::Function;           leafsToCheck = 1; break;
            case 10: singleLeaf.type = DTFunctionLeaf::Or;                 leafsToCheck = 2; break;
            case 11: singleLeaf.type = DTFunctionLeaf::And;                leafsToCheck = 2; break;
            case 12: singleLeaf.type = DTFunctionLeaf::NotEqual;           leafsToCheck = 2; break;
            case 13: singleLeaf.type = DTFunctionLeaf::Equal;              leafsToCheck = 2; break;
            case 14: singleLeaf.type = DTFunctionLeaf::Less;               leafsToCheck = 2; break;
            case 15: singleLeaf.type = DTFunctionLeaf::Greater;            leafsToCheck = 2; break;
            case 16: singleLeaf.type = DTFunctionLeaf::LessEqual;          leafsToCheck = 2; break;
            case 17: singleLeaf.type = DTFunctionLeaf::GreaterEqual;       leafsToCheck = 2; break;
            default: DTErrorMessage("Operator: Not implemeted");
        }
        singleLeaf.value = leafArray(1,i);
        singleLeaf.listNumber = int(leafArray(2,i));
        if (singleLeaf.listNumber<0 || (howManyLists && singleLeaf.listNumber>=howManyLists)) {
            errorMsg = "Invalid format, referencing a list that isn't saved.";
            break;
        }
        
        singleLeaf.constantNumber = int(leafArray(3,i));
        if (singleLeaf.constantNumber<0 || (howManyConstants && singleLeaf.constantNumber>=howManyConstants)) {
            errorMsg = "Invalid format, referencing a constant that isn't saved.";
            break;
        }
        
        singleLeaf.functionNumber = int(leafArray(4,i));
        if (singleLeaf.functionNumber<0 || (howManyFunctions && singleLeaf.functionNumber>=howManyFunctions)) {
            errorMsg = "Invalid format, referencing a sub function that has not been saved.";
            break;
        }
        
        singleLeaf.leftReference = int(leafArray(5,i));
        singleLeaf.rightReference = int(leafArray(6,i));
        
        switch (int(leafArray(7,i))) {
            case 0:  functionName = DTFunctionLeaf::Identity;   break;
            case 1:  functionName = DTFunctionLeaf::Abs;        break;
            case 2:  functionName = DTFunctionLeaf::Sin;        break;
            case 3:  functionName = DTFunctionLeaf::Cos;        break;
            case 4:  functionName = DTFunctionLeaf::ASin;       break;
            case 5:  functionName = DTFunctionLeaf::ACos;       break;
            case 6:  functionName = DTFunctionLeaf::Tan;        break;
            case 7:  functionName = DTFunctionLeaf::ATan;       break;
            case 8:  functionName = DTFunctionLeaf::Sinh;       break;
            case 9:  functionName = DTFunctionLeaf::Cosh;       break;
            case 10: functionName = DTFunctionLeaf::Tanh;       break;
            case 11: functionName = DTFunctionLeaf::Sqrt;       break;
            case 12: functionName = DTFunctionLeaf::Gamma;      break;
            case 13: functionName = DTFunctionLeaf::LogGamma;   break;
            case 14: functionName = DTFunctionLeaf::Exp;        break;
            case 15: functionName = DTFunctionLeaf::Log;        break;
            case 16: functionName = DTFunctionLeaf::Log10;      break;
            case 17: functionName = DTFunctionLeaf::Erfc;       break;
            case 26: functionName = DTFunctionLeaf::Erf;        break;
            case 18: functionName = DTFunctionLeaf::Floor;      break;
            case 19: functionName = DTFunctionLeaf::Ceil;       break;
            case 20: functionName = DTFunctionLeaf::Round;      break;
            case 21: functionName = DTFunctionLeaf::Fact;       break;
            case 22: functionName = DTFunctionLeaf::Min;        break;
            case 23: functionName = DTFunctionLeaf::Max;        break;
            case 24: functionName = DTFunctionLeaf::Norm;       break;
            case 25: functionName = DTFunctionLeaf::SignSwitch; break;
            case 27: functionName = DTFunctionLeaf::Angle;      break;
            case 28: functionName = DTFunctionLeaf::J0;         break;
            case 29: functionName = DTFunctionLeaf::J1;         break;
            case 30: functionName = DTFunctionLeaf::Jn;         break;
            case 31: functionName = DTFunctionLeaf::Y0;         break;
            case 32: functionName = DTFunctionLeaf::Y1;         break;
            case 33: functionName = DTFunctionLeaf::Yn;         break;
            case 34: functionName = DTFunctionLeaf::Mod;        break;
            case 35: functionName = DTFunctionLeaf::Sinc;       break;
            case 36: functionName = DTFunctionLeaf::Rect;       break;
            case 37: functionName = DTFunctionLeaf::Tri;        break;
            case 38: functionName = DTFunctionLeaf::H;          break;
            case 39: functionName = DTFunctionLeaf::Sgn;        break;
            case 40: functionName = DTFunctionLeaf::Log2;       break;
            // case 41: functionName = DTFunctionLeaf::Seconds;        break;
            case 42: functionName = DTFunctionLeaf::Year;       break;
            case 43: functionName = DTFunctionLeaf::Month;      break;
            case 44: functionName = DTFunctionLeaf::Day;        break;
            case 45: functionName = DTFunctionLeaf::Hour;       break;
            case 46: functionName = DTFunctionLeaf::Minute;     break;
            case 47: functionName = DTFunctionLeaf::IsAM;       break;
            case 48: functionName = DTFunctionLeaf::IsPM;       break;
            case 49: functionName = DTFunctionLeaf::Log1p;      break;
            case 50: functionName = DTFunctionLeaf::Rem;        break;
            case 51: functionName = DTFunctionLeaf::Cbrt;       break;
            case 52: functionName = DTFunctionLeaf::If;         break;
            case 53: functionName = DTFunctionLeaf::Isnan;      break;
            case 54: functionName = DTFunctionLeaf::Isfinite;   break;
                
            default: DTErrorMessage("Operator: Function not defined");
                functionName = DTFunctionLeaf::Identity;
        }
        singleLeaf.functionReference = functionName;
        
        if (singleLeaf.type!=DTFunctionLeaf::ListOfValues) {
            // A a leaf reference can not point outside the bound, or to an earlier entry.
            // This will guarantee that there will not be any circular references.
            if ((leafsToCheck==0 && singleLeaf.leftReference!=0 ) ||
                (leafsToCheck>0  && singleLeaf.leftReference<=i ) ||
                (leafsToCheck==1 && singleLeaf.rightReference<0 ) ||
                (leafsToCheck>1  && singleLeaf.rightReference<=i) ||
                (leafsToCheck==2 && singleLeaf.leftReference==singleLeaf.rightReference)) {
                errorMsg = "Invalid format - invalid leaf reference.";
                break;
            }
        }
        
        leafs(i) = singleLeaf;
    }
    
    if (i<howManyLeafs) {
        if (errorMsg.size()==0) errorMsg = "Invalid format";
        DTErrorMessage("Read(file,name,expression)",errorMsg);
        var = DTFunction();
        return;
    }
    
    DTMutablePointer<DTFunctionStorage> toReturn(new DTFunctionStorage());
    
    toReturn->leafs = leafs;
    toReturn->leafsD = leafs.Pointer();
    toReturn->constantNames = unknownNames;
    toReturn->argumentLists = argumentLists;

    if (subFunctions.Length()) {
        toReturn->SetSubFunctions(subFunctions.Pointer(),subFunctions.Length());
    }

    // Precompute the leaf information
    leafs(0).ComputeIfThisLeafIsAList(leafs);
    
    var = DTFunction(toReturn);
}

void Write(DTDataStorage &output,const std::string &name,const DTFunction &var)
{
    // Stack together the entries.
    // type=int, value=double, listNumber=int, constantNumber=int,leftReference=int, rightReference=int, functionReference=int
    int i,howMany;
    
    howMany = int(var.Content()->HowManySubFunctions());
    DTFunction subFunction;
    output.Save(int(howMany),name+"_fC");
    for (i=0;i<howMany;i++) {
        subFunction = var.Content()->SubFunction(i);
        Write(output,name+"_f"+DTInt2String(i+1),subFunction);
    }
    
    DTList<std::string> constants = var.Content()->constantNames;
    howMany = int(constants.Length());
    output.Save(int(howMany),name+"_uC");
    for (i=0;i<howMany;i++) {
        output.Save(constants(i),name+"_u"+DTInt2String(i+1));
    }
    
    DTList<DTIntArray> argumentLists = var.Content()->argumentLists;
    howMany = int(argumentLists.Length());
    output.Save(int(howMany),name+"_lC");
    for (i=0;i<howMany;i++) {
        output.Save(argumentLists(i),name+"_l"+DTInt2String(i+1));
    }
    
    DTList<DTFunctionLeaf> leafs = var.Content()->leafs;
    howMany = int(leafs.Length());
    DTMutableDoubleArray leafArray(8,howMany);
    int label;
    for (i=0;i<howMany;i++) {
        leafArray(0,i) = leafs(i).type;
        leafArray(1,i) = leafs(i).value;
        leafArray(2,i) = leafs(i).listNumber;
        leafArray(3,i) = leafs(i).constantNumber;
        leafArray(4,i) = leafs(i).functionNumber;
        leafArray(5,i) = leafs(i).leftReference;
        leafArray(6,i) = leafs(i).rightReference;
        switch (leafs(i).functionReference) {
            case DTFunctionLeaf::Identity:   label = 0;  break;
            case DTFunctionLeaf::Abs:        label = 1;  break;
            case DTFunctionLeaf::Sin:        label = 2;  break;
            case DTFunctionLeaf::Cos:        label = 3;  break;
            case DTFunctionLeaf::ASin:       label = 4;  break;
            case DTFunctionLeaf::ACos:       label = 5;  break;
            case DTFunctionLeaf::Tan:        label = 6;  break;
            case DTFunctionLeaf::ATan:       label = 7;  break;
            case DTFunctionLeaf::Sinh:       label = 8;  break;
            case DTFunctionLeaf::Cosh:       label = 9;  break;
            case DTFunctionLeaf::Tanh:       label = 10; break;
            case DTFunctionLeaf::Sqrt:       label = 11; break;
            case DTFunctionLeaf::Gamma:      label = 12; break;
            case DTFunctionLeaf::LogGamma:   label = 13; break;
            case DTFunctionLeaf::Exp:        label = 14; break;
            case DTFunctionLeaf::Log:        label = 15; break;
            case DTFunctionLeaf::Log10:      label = 16; break;
            case DTFunctionLeaf::Erfc:       label = 17; break;
            case DTFunctionLeaf::Erf:        label = 26; break;
            case DTFunctionLeaf::Floor:      label = 18; break;
            case DTFunctionLeaf::Ceil:       label = 19; break;
            case DTFunctionLeaf::Round:      label = 20; break;
            case DTFunctionLeaf::Fact:       label = 21; break;
            case DTFunctionLeaf::Min:        label = 22; break;
            case DTFunctionLeaf::Max:        label = 23; break;
            case DTFunctionLeaf::Norm:       label = 24; break;
            case DTFunctionLeaf::SignSwitch: label = 25; break;
            case DTFunctionLeaf::Angle:      label = 27; break;
            case DTFunctionLeaf::J0:         label = 28; break;
            case DTFunctionLeaf::J1:         label = 29; break;
            case DTFunctionLeaf::Jn:         label = 30; break;
            case DTFunctionLeaf::Y0:         label = 31; break;
            case DTFunctionLeaf::Y1:         label = 32; break;
            case DTFunctionLeaf::Yn:         label = 33; break;
            case DTFunctionLeaf::Mod:        label = 34; break;
            case DTFunctionLeaf::Sinc:       label = 35; break;
            case DTFunctionLeaf::Rect:       label = 36; break;
            case DTFunctionLeaf::Tri:        label = 37; break;
            case DTFunctionLeaf::H:          label = 38; break;
            case DTFunctionLeaf::Sgn:        label = 39; break;
            case DTFunctionLeaf::Log2:       label = 40; break;
            // case DTFunctionLeaf::Seconds;    label = 41; break;
            case DTFunctionLeaf::Year:       label = 42; break;
            case DTFunctionLeaf::Month:      label = 43; break;
            case DTFunctionLeaf::Day:        label = 44; break;
            case DTFunctionLeaf::Hour:       label = 45; break;
            case DTFunctionLeaf::Minute:     label = 46; break;
            case DTFunctionLeaf::IsAM:       label = 47; break;
            case DTFunctionLeaf::IsPM:       label = 48; break;
            case DTFunctionLeaf::Log1p:      label = 49; break;
            case DTFunctionLeaf::Rem:        label = 50; break;
            case DTFunctionLeaf::Cbrt:       label = 51; break;
            case DTFunctionLeaf::If:         label = 52; break;
            case DTFunctionLeaf::Isnan:      label = 53; break;
            case DTFunctionLeaf::Isfinite:   label = 54; break;

            default:                         label = 0;
        }
        leafArray(7,i) = double(label);
    }
    
    Write(output,name,leafArray);
}

DTFunction SingleOperator(DTFunctionLeaf::FunctionName type,const DTFunctionStorage &A)
{
    // For example cos(f) where f is a function.  The function storage is the actual data behind the f object (DTFunction is a smart pointer).
    // Need to create a new tree structure where the top node is the function you want (e.g. cos) and the tree structure of f is added as the only
    // child for that node.  Since the tree structure is saved flattened into a list and reference are by offsets into that list rather
    // than pointers all arguments have been shifted.
    ssize_t i,j,len,howMany;

    /////////////////////////////////////////////////////////////////////////////
    // The argument lists need to be updated to reflect the additional entry at the beginning.
    howMany = A.argumentLists.Length();
    DTMutableList<DTIntArray> newLists(howMany);
    DTMutableIntArray temp;
    for (i=0;i<howMany;i++) {
        // This could be done by newLists(i) = A.argumentLists(i)+1
        // but inline it here to avoid dependence on the DTIntArrayOperator.h header
        temp = A.argumentLists(i).Copy();
        len = temp.Length();
        for (j=0;j<len;j++) {
            temp(j) += 1;
        }
        newLists(i) = temp;
    }


    /////////////////////////////////////////////////////////////////////////////
    // Create a new leaf list
    DTList<DTFunctionLeaf> Atree = A.leafs;
    howMany = Atree.Length();
    DTMutableList<DTFunctionLeaf> combinedTrees(howMany+1);
    
    DTFunctionLeaf singleLeaf;
    singleLeaf.leftReference = 1;
    singleLeaf.type = DTFunctionLeaf::PredefinedFunction;
    singleLeaf.functionReference = type;
    combinedTrees(0) = singleLeaf;
    
    
    /////////////////////////////////////////////////////////////////////////////
    for (i=0;i<howMany;i++) {
        singleLeaf = Atree(i);
        // Need to shift the references.        
        if (singleLeaf.leftReference) singleLeaf.leftReference++;
        if (singleLeaf.rightReference) singleLeaf.rightReference++;
        combinedTrees(i+1) = singleLeaf;
    }
    
    
    /////////////////////////////////////////////////////////////////////////////
    DTMutablePointer<DTFunctionStorage> toReturn(new DTFunctionStorage());
    
    toReturn->leafs = combinedTrees;
    toReturn->leafsD = combinedTrees.Pointer();
    toReturn->constantNames = A.constantNames;
    toReturn->argumentLists = newLists;

    // Precompute the leaf information
    combinedTrees(0).ComputeIfThisLeafIsAList(combinedTrees);

    if (A.HowManySubFunctions()) {
        howMany = A.HowManySubFunctions();
        DTMutableList<DTFunction> subFunctions(howMany);
        for (i=0;i<howMany;i++)
            subFunctions(i) = A.SubFunction(i);
        toReturn->SetSubFunctions(subFunctions.Pointer(),subFunctions.Length());
    }
    
    return DTFunction(toReturn);
}

DTFunction BinaryOperator(DTFunctionLeaf::LeafType t,
                          const DTFunctionStorage &A,
                          const DTFunctionStorage &B)
{
    int pos,posA,posB;
    ssize_t lenA,lenB;
    
    /////////////////////////////////////////////////////////////////////////////
    // Combine the functions.
    lenA = A.HowManySubFunctions();
    lenB = B.HowManySubFunctions();
    DTMutableList<DTFunction> combinedFunctions(lenA+lenB);
    ssize_t offsetBFunctions = lenA;
    for (posA=0;posA<lenA;posA++)
        combinedFunctions(posA) = A.SubFunction(posA);
    for (posB=0;posB<lenB;posB++)
        combinedFunctions(offsetBFunctions+posB) = B.SubFunction(posB);
    
    
    /////////////////////////////////////////////////////////////////////////////
    // Combine the constants.  Should put them in alphabetical order, and remove duplicates.
    lenA = A.constantNames.Length();
    lenB = B.constantNames.Length();
    DTMutableIntArray AConstantsToNewConstants(lenA);
    DTMutableIntArray BConstantsToNewConstants(lenB);
    pos = posA = posB = 0;
    // Merge the lists.
    while (posA<lenA && posB<lenB) {
        if (A.constantNames(posA)==B.constantNames(posB)) {
            AConstantsToNewConstants(posA++) = pos;
            BConstantsToNewConstants(posB++) = pos;
        }
        else if (A.constantNames(posA)<B.constantNames(posB)) {
            AConstantsToNewConstants(posA++) = pos;
        }
        else {
            BConstantsToNewConstants(posB++) = pos;
        }
        pos++;
    }
    while (posA<lenA) {
        AConstantsToNewConstants(posA++) = pos++;
    }
    while (posB<lenB) {
        BConstantsToNewConstants(posB++) = pos++;
    }
    DTMutableList<std::string> combinedConstants(pos);
    for (posA=0;posA<lenA;posA++)
        combinedConstants(AConstantsToNewConstants(posA)) = A.constantNames(posA);
    for (posB=0;posB<lenB;posB++)
        combinedConstants(BConstantsToNewConstants(posB)) = B.constantNames(posB);
    
    
    /////////////////////////////////////////////////////////////////////////////
    // Combine the lists
    lenA = A.argumentLists.Length();
    lenB = B.argumentLists.Length();
    DTMutableList<DTIntArray> combinedLists(lenA+lenB);
    ssize_t offsetBLists = lenA;
    
    DTMutableIntArray temp;
    ssize_t j,len;
    
    // This could be done by newLists(i) = A.argumentLists(i)+1
    // but inline it here to avoid dependence on the DTIntArrayOperator.h header
    for (posA=0;posA<lenA;posA++) {
        temp = A.argumentLists(posA).Copy();
        len = temp.Length();
        for (j=0;j<len;j++) {
            temp(j) += 1;
        }
        combinedLists(posA) = temp;
    }
    for (posB=0;posB<lenB;posB++) {
        temp = B.argumentLists(posB).Copy();
        len = temp.Length();
        ssize_t addThis = A.leafs.Length()+1;
        for (j=0;j<len;j++) {
            temp(j) += addThis;
        }
        combinedLists(offsetBLists+posB) = temp;
    }
        
    /////////////////////////////////////////////////////////////////////////////
    // Combine the leafs.
    DTList<DTFunctionLeaf> Atree = A.leafs;
    DTList<DTFunctionLeaf> Btree = B.leafs;
                                            
    lenA = Atree.Length();
    lenB = Btree.Length();
    DTMutableList<DTFunctionLeaf> combinedTrees(lenA+lenB+1);
    
    DTFunctionLeaf singleLeaf;
    pos = 1;
    for (posA=0;posA<lenA;posA++) {
        singleLeaf = Atree(posA);
        
        if (singleLeaf.type==DTFunctionLeaf::Constant)
            singleLeaf.constantNumber = AConstantsToNewConstants(singleLeaf.constantNumber);
        
        // Need to shift the references.
        if (singleLeaf.leftReference) singleLeaf.leftReference++;
        if (singleLeaf.rightReference) singleLeaf.rightReference++;
        combinedTrees(pos++) = singleLeaf;
    }
    
    int offsetBLeafs = int(lenA+1);
    for (posB=0;posB<lenB;posB++) {
        singleLeaf = Btree(posB);
        
        if (singleLeaf.type==DTFunctionLeaf::Constant)
            singleLeaf.constantNumber = BConstantsToNewConstants(singleLeaf.constantNumber);
        
        if (singleLeaf.type==DTFunctionLeaf::ListOfValues)
            singleLeaf.listNumber += offsetBLists;
        
        if (singleLeaf.type==DTFunctionLeaf::Function)
            singleLeaf.functionNumber += offsetBFunctions;
        
        // Need to shift the leaf references.
        if (singleLeaf.leftReference) singleLeaf.leftReference+=offsetBLeafs;
        if (singleLeaf.rightReference) singleLeaf.rightReference+=offsetBLeafs;
        combinedTrees(pos++) = singleLeaf;
    }


    /////////////////////////////////////////////////////////////////////////////
    singleLeaf = DTFunctionLeaf();
    singleLeaf.type = t;
    singleLeaf.leftReference = 1;
    singleLeaf.rightReference = offsetBLeafs;
    combinedTrees(0) = singleLeaf;
    

    /////////////////////////////////////////////////////////////////////////////
    DTMutablePointer<DTFunctionStorage> toReturn(new DTFunctionStorage());
    
    toReturn->leafs = combinedTrees;
    toReturn->leafsD = combinedTrees.Pointer();
    toReturn->argumentLists = combinedLists;
    toReturn->constantNames = combinedConstants;
    toReturn->SetSubFunctions(combinedFunctions.Pointer(),combinedFunctions.Length());
    
    // Precompute the leaf information
    combinedTrees(0).ComputeIfThisLeafIsAList(combinedTrees);

    return DTFunction(toReturn);
}

DTFunction operator-(const DTFunction &A)
{
    return DTFunction::Value(-1)*A;
}

DTFunction operator*(const DTFunction &A,double b)
{
    return A*DTFunction::Value(b);
}

DTFunction operator*(double a,const DTFunction &B)
{
    return DTFunction::Value(a)*B;
}

DTFunction operator*(const char *A,const DTFunction &B)
{
    return DTFunction(A)*B;
}

DTFunction operator/(const DTFunction &A,double b)
{
    return A/DTFunction::Value(b);
}

DTFunction operator/(double a,const DTFunction &B)
{
    return DTFunction::Value(a)/B;
}

DTFunction operator+(const DTFunction &A,double b)
{
    return A+DTFunction::Value(b);
}

DTFunction operator+(double a,const DTFunction &B)
{
    return DTFunction::Value(a)+B;
}

DTFunction operator+(const DTFunction &A,const char *b)
{
    return A+DTFunction(b);
}

DTFunction operator+(const char *a,const DTFunction &B)
{
    return DTFunction(a)+B;
}

DTFunction operator-(const DTFunction &A,double b)
{
    return A-DTFunction::Value(b);
}

DTFunction operator-(double a,const DTFunction &B)
{
    return DTFunction::Value(a)-B;
}

DTFunction operator<(const DTFunction &A,const DTFunction &B)
{
    return BinaryOperator(DTFunctionLeaf::Less,*A.Content(),*B.Content());
}

DTFunction operator>(const DTFunction &A,const DTFunction &B)
{
    return BinaryOperator(DTFunctionLeaf::Greater,*A.Content(),*B.Content());
}

DTFunction operator<=(const DTFunction &A,const DTFunction &B)
{
    return BinaryOperator(DTFunctionLeaf::LessEqual,*A.Content(),*B.Content());
}

DTFunction operator>=(const DTFunction &A,const DTFunction &B)
{
    return BinaryOperator(DTFunctionLeaf::GreaterEqual,*A.Content(),*B.Content());
}

DTFunction operator||(const DTFunction &A,const DTFunction &B)
{
    return BinaryOperator(DTFunctionLeaf::Or,*A.Content(),*B.Content());
}

DTFunction operator&&(const DTFunction &A,const DTFunction &B)
{
    return BinaryOperator(DTFunctionLeaf::And,*A.Content(),*B.Content());
}

DTFunction abs(const DTFunction &A)
{
    return SingleOperator(DTFunctionLeaf::Abs,*A.Content());
}

DTFunction sin(const DTFunction &A)
{
    return SingleOperator(DTFunctionLeaf::Sin,*A.Content());
}

DTFunction cos(const DTFunction &A)
{
    return SingleOperator(DTFunctionLeaf::Cos,*A.Content());
}

DTFunction asin(const DTFunction &A)
{
    return SingleOperator(DTFunctionLeaf::ASin,*A.Content());
}

DTFunction acos(const DTFunction &A)
{
    return SingleOperator(DTFunctionLeaf::ACos,*A.Content());
}

DTFunction tan(const DTFunction &A)
{
    return SingleOperator(DTFunctionLeaf::Tan,*A.Content());
}

DTFunction atan(const DTFunction &A)
{
    return SingleOperator(DTFunctionLeaf::ATan,*A.Content());
}

DTFunction sinh(const DTFunction &A)
{
    return SingleOperator(DTFunctionLeaf::Sinh,*A.Content());
}

DTFunction cosh(const DTFunction &A)
{
    return SingleOperator(DTFunctionLeaf::Cosh,*A.Content());
}

DTFunction tanh(const DTFunction &A)
{
    return SingleOperator(DTFunctionLeaf::Tanh,*A.Content());
}

DTFunction sqrt(const DTFunction &A)
{
    return SingleOperator(DTFunctionLeaf::Sqrt,*A.Content());
}

DTFunction gamma(const DTFunction &A)
{
    return SingleOperator(DTFunctionLeaf::Gamma,*A.Content());
}

DTFunction loggamma(const DTFunction &A)
{
    return SingleOperator(DTFunctionLeaf::LogGamma,*A.Content());
}

DTFunction exp(const DTFunction &A)
{
    return SingleOperator(DTFunctionLeaf::Exp,*A.Content());
}

DTFunction log(const DTFunction &A)
{
    return SingleOperator(DTFunctionLeaf::Log,*A.Content());
}

DTFunction log10(const DTFunction &A)
{
    return SingleOperator(DTFunctionLeaf::Log10,*A.Content());
}

DTFunction erfc(const DTFunction &A)
{
    return SingleOperator(DTFunctionLeaf::Erfc,*A.Content());
}

DTFunction erf(const DTFunction &A)
{
    return SingleOperator(DTFunctionLeaf::Erf,*A.Content());
}

DTFunction floor(const DTFunction &A)
{
    return SingleOperator(DTFunctionLeaf::Floor,*A.Content());
}

DTFunction ceil(const DTFunction &A)
{
    return SingleOperator(DTFunctionLeaf::Ceil,*A.Content());
}

DTFunction round(const DTFunction &A)
{
    return SingleOperator(DTFunctionLeaf::Round,*A.Content());
}

DTFunction fact(const DTFunction &A)
{
    return SingleOperator(DTFunctionLeaf::Fact,*A.Content());
}

DTFunction pow(const DTFunction &F,double p)
{
    DTFunction pV = DTFunction::Value(p);
    return BinaryOperator(DTFunctionLeaf::Power,*F.Content(),*pV.Content());
}

DTFunction j0(const DTFunction &A)
{
    return SingleOperator(DTFunctionLeaf::J0,*A.Content());
}

DTFunction j1(const DTFunction &A)
{
    return SingleOperator(DTFunctionLeaf::J1,*A.Content());
}

DTFunction y0(const DTFunction &A)
{
    return SingleOperator(DTFunctionLeaf::Y0,*A.Content());
}

DTFunction y1(const DTFunction &A)
{
    return SingleOperator(DTFunctionLeaf::Y1,*A.Content());
}
