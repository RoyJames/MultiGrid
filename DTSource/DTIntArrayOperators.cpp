// Part of DTSource. Copyright 2004-2017. David Adalsteinsson.
// see http://www.visualdatatools.com/DTSource/license.html for more information.

#include "DTIntArrayOperators.h"

#include "DTArrayTemplates.h"

DTMutableIntArray operator+(const DTIntArray &A,const DTIntArray &B)
{
    return DTAddArrays<DTIntArray,DTMutableIntArray,int>("IntArray+IntArray",A,B);
}

DTMutableIntArray operator-(const DTIntArray &A,const DTIntArray &B)
{
    return DTSubtractArrays<DTIntArray,DTMutableIntArray,int>("IntArray-IntArray",A,B);
}

DTMutableIntArray operator*(const DTIntArray &A,const DTIntArray &B)
{
    return DTMultiplyArrays<DTIntArray,DTMutableIntArray,int>("IntArray*IntArray",A,B);
}

DTMutableIntArray operator/(const DTIntArray &A,const DTIntArray &B)
{
    return DTDivideArrays<DTIntArray,DTMutableIntArray,int>("IntArray/IntArray",A,B);
}

DTMutableIntArray operator+(const DTIntArray &A,int b)
{
    return DTArrayPlusNumber<DTIntArray,DTMutableIntArray,int>(A,b);
}

DTMutableIntArray operator-(const DTIntArray &A,int b)
{
    return DTArrayPlusNumber<DTIntArray,DTMutableIntArray,int>(A,-b);
}

DTMutableIntArray operator*(const DTIntArray &A,int b)
{
    return DTArrayTimesNumber<DTIntArray,DTMutableIntArray,int>(A,b);
}

DTMutableIntArray operator/(const DTIntArray &A,int b)
{
    return DTArrayDivideByNumber<DTIntArray,DTMutableIntArray,int>(A,b);
}

DTMutableIntArray operator+(int a,const DTIntArray &B)
{
    return DTArrayPlusNumber<DTIntArray,DTMutableIntArray,int>(B,a);
}

DTMutableIntArray operator-(int a,const DTIntArray &B)
{
    return DTNumberMinusArray<DTIntArray,DTMutableIntArray,int>(a,B);
}

DTMutableIntArray operator*(int a,const DTIntArray &B)
{
    return DTArrayTimesNumber<DTIntArray,DTMutableIntArray,int>(B,a);
}

DTMutableIntArray operator/(int a,const DTIntArray &B)
{
    return DTNumberDividedByArray<DTIntArray,DTMutableIntArray,int>(a,B);
}

DTMutableIntArray operator-(const DTIntArray &A)
{
    return DTNegateArray<DTIntArray,DTMutableIntArray,int>(A);
}

