// Part of DTSource. Copyright 2004-2017. David Adalsteinsson.
// see http://www.visualdatatools.com/DTSource/license.html for more information.

#ifndef DTIntArrayOperators_Header
#define DTIntArrayOperators_Header

#include "DTIntArray.h"

// Array operator Array
extern DTMutableIntArray operator+(const DTIntArray &A,const DTIntArray &B);
extern DTMutableIntArray operator-(const DTIntArray &A,const DTIntArray &B);
extern DTMutableIntArray operator*(const DTIntArray &A,const DTIntArray &B);
extern DTMutableIntArray operator/(const DTIntArray &A,const DTIntArray &B);

// Array operator Number
extern DTMutableIntArray operator+(const DTIntArray &A,int b);
extern DTMutableIntArray operator-(const DTIntArray &A,int b);
extern DTMutableIntArray operator*(const DTIntArray &A,int b);
extern DTMutableIntArray operator/(const DTIntArray &A,int b);

// Number operator Array
extern DTMutableIntArray operator+(int a,const DTIntArray &B);
extern DTMutableIntArray operator-(int a,const DTIntArray &B);
extern DTMutableIntArray operator*(int a,const DTIntArray &B);
extern DTMutableIntArray operator/(int a,const DTIntArray &B);

// Negating
extern DTMutableIntArray operator-(const DTIntArray &A);

#endif
