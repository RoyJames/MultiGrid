// Part of DTSource. Copyright 2008-2015. David A. David Adalsteinsson.
// see http://www.visualdatatools.com/DTSource/license.html for more information.

#ifndef DTDoubleComplexArray_Header
#define DTDoubleComplexArray_Header

#include <iostream>
#include <unistd.h>
#include "DTLock.h"

// By default, range check is turned on.
#ifndef DTRangeCheck
#define DTRangeCheck 1
#endif

using namespace std;

#include <complex>
typedef std::complex<double> DTDoubleComplex;
/*
struct DTDoubleComplex {
    DTDoubleComplex() {}
    DTDoubleComplex(double r) : real(r), imag(0.0) {}
    DTDoubleComplex(double r,double i) : real(r), imag(i) {}
    double real;
    double imag;

    bool operator==(const DTDoubleComplex &b) const {return (real==b.real && imag==b.imag);}
    
    void operator*=(double v) {real*=v; imag*=v;}
};

inline DTDoubleComplex operator-(const DTDoubleComplex &a) {return DTDoubleComplex(-a.real,-a.imag);}
inline DTDoubleComplex operator*(double a,const DTDoubleComplex &b) {return DTDoubleComplex(a*b.real,a*b.imag);}
inline DTDoubleComplex operator*(const DTDoubleComplex &a,const DTDoubleComplex &b) {return DTDoubleComplex(a.real*b.real - a.imag*b.imag,a.real*b.imag+a.imag*b.real);}
inline DTDoubleComplex operator+(const DTDoubleComplex &a,const DTDoubleComplex &b) {return DTDoubleComplex(a.real+b.real,a.imag+b.imag);}
inline DTDoubleComplex operator-(const DTDoubleComplex &a,const DTDoubleComplex &b) {return DTDoubleComplex(a.real-b.real,a.imag-b.imag);}
// (ar+i*ai)/(br+i*bi) = (ar+i*ai)*(br-i*bi)/|b|^2
inline DTDoubleComplex operator/(const DTDoubleComplex &a,const DTDoubleComplex &b) {double temp = 1.0/(b.real*b.real + b.imag*b.imag); return DTDoubleComplex((a.real*b.real+a.imag*b.imag)*temp,(a.imag*b.real-a.real*b.imag)*temp);}

  */

class DTDoubleComplexArrayStorage {
public:
    DTDoubleComplexArrayStorage(ssize_t mv,ssize_t nv,ssize_t ov);
    ~DTDoubleComplexArrayStorage();

    DTLock accessLock;
    ssize_t m,n,o,mn,length;
    int referenceCount;
    int mutableReferences;
    DTDoubleComplex *Data;
    
private:
    DTDoubleComplexArrayStorage(const DTDoubleComplexArrayStorage &);
    DTDoubleComplexArrayStorage &operator=(const DTDoubleComplexArrayStorage &);
};

class DTMutableDoubleComplexArray;
class DTIndex;
class DTDoubleArray;
class DTMutableDoubleArray;

class DTDoubleComplexArray {

public:
    DTDoubleComplexArray() : Storage(new DTDoubleComplexArrayStorage(0,0,0)), accessLock(), invalidEntry(0.0,0.0) {}
    virtual ~DTDoubleComplexArray();
    DTDoubleComplexArray(const DTDoubleComplexArray &A);
    DTDoubleComplexArray &operator=(const DTDoubleComplexArray &A);

protected:
    // If you get a notice that this is protected, change DTDoubleComplexArray to DTMutableDoubleComplexArray
    explicit DTDoubleComplexArray(ssize_t mv,ssize_t nv=1,ssize_t ov=1) : Storage(new DTDoubleComplexArrayStorage(mv,nv,ov)), accessLock(), invalidEntry(0.0) {}

public:
    DTMutableDoubleComplexArray Copy() const;
    
    // Size information.
    ssize_t m() const;
    ssize_t n() const;
    ssize_t o() const;
    ssize_t Length() const;
    bool IsEmpty() const;
    bool NotEmpty() const;
    
    // Low level access
    int ReferenceCount() const;
    int MutableReferences() const; // How many mutable arrays have access to the pointer.
    const DTDoubleComplex *Pointer() const {return Storage->Data;}
    
    // Allow A(i) and A(i,j), but check each access.
#if DTRangeCheck
    const DTDoubleComplex operator()(ssize_t i) const
        {if (i<0 || i>=Storage->length)
            {PrintErrorMessage(i); return invalidEntry;}
         return Storage->Data[i];}
    const DTDoubleComplex operator()(ssize_t i,ssize_t j) const
        {if (i<0 || i>=Storage->m || j<0 || j>=Storage->n)
            {PrintErrorMessage(i,j); return invalidEntry;}
         return Storage->Data[i+j*Storage->m];}
    const DTDoubleComplex operator()(ssize_t i,ssize_t j,ssize_t k) const
        {if (i<0 || i>=Storage->m || j<0 || j>=Storage->n || k<0 || k>=Storage->o)
            {PrintErrorMessage(i,j,k); return invalidEntry;}
            return Storage->Data[i+j*Storage->m+k*Storage->mn];}
#else
    // No range check.  Slightly faster, but not as safe.
    // For fastest access, extract the underlying pointer and dereference it directly.
    const DTDoubleComplex operator()(size_t i) const {return Storage->Data[i];}
    const DTDoubleComplex operator()(size_t i,size_t j) const {return Storage->Data[i+j*Storage->m];}
    const DTDoubleComplex operator()(size_t i,size_t j,size_t k) const {return Storage->Data[i+j*Storage->m+k*Storage->mn];}
#endif

    void pinfo(void) const;
    void pe(int,int) const;
    void pall(void) const;

    ssize_t Find(DTDoubleComplex) const; // Returns -1 if not found.
    
protected:
    DTDoubleComplexArrayStorage *Storage;
    DTLock accessLock;
    DTDoubleComplex invalidEntry;
    
    // Error messages for index access.
    void PrintErrorMessage(ssize_t i) const;
    void PrintErrorMessage(ssize_t i,ssize_t j) const;
    void PrintErrorMessage(ssize_t i,ssize_t j,ssize_t k) const;
};

class DTMutableDoubleComplexArray : public DTDoubleComplexArray
{
public:
    DTMutableDoubleComplexArray() : DTDoubleComplexArray() {Storage->mutableReferences = 1;}
    ~DTMutableDoubleComplexArray();
    explicit DTMutableDoubleComplexArray(ssize_t mv,ssize_t nv=1,ssize_t ov=1) : DTDoubleComplexArray(mv,nv,ov) {Storage->mutableReferences = 1;}
    DTMutableDoubleComplexArray(const DTMutableDoubleComplexArray &A);

    DTMutableDoubleComplexArray &operator=(const DTMutableDoubleComplexArray &A);

    // Assignment
    DTMutableDoubleComplexArray &operator=(DTDoubleComplex a);

    // Raw access
    DTDoubleComplex *Pointer() {return Storage->Data;}
    const DTDoubleComplex *Pointer() const {return Storage->Data;}

    // High level access
#if DTRangeCheck
    DTDoubleComplex operator()(ssize_t i) const
        {if (i<0 || i>=Storage->length)
            {PrintErrorMessage(i); return invalidEntry;}
            return Storage->Data[i];}
    DTDoubleComplex &operator()(ssize_t i)
        {if (i<0 || i>=Storage->length)
            {PrintErrorMessage(i); return invalidEntry;}
        return Storage->Data[i];}
    DTDoubleComplex operator()(ssize_t i,ssize_t j) const
        {if (i<0 || i>=Storage->m || j<0 || j>=Storage->n)
            {PrintErrorMessage(i,j); return invalidEntry;}
            return Storage->Data[i+j*Storage->m];}
    DTDoubleComplex &operator()(ssize_t i,ssize_t j)
        {if (i<0 || i>=Storage->m || j<0 || j>=Storage->n)
            {PrintErrorMessage(i,j); return invalidEntry;}
        return Storage->Data[i+j*Storage->m];}
    DTDoubleComplex operator()(ssize_t i,ssize_t j,ssize_t k) const
        {if (i<0 || i>=Storage->m || j<0 || j>=Storage->n || k<0 || k>=Storage->o)
            {PrintErrorMessage(i,j,k); return invalidEntry;}
            return Storage->Data[i+j*Storage->m+k*Storage->mn];}
    DTDoubleComplex &operator()(ssize_t i,ssize_t j,ssize_t k)
        {if (i<0 || i>=Storage->m || j<0 || j>=Storage->n || k<0 || k>=Storage->o)
            {PrintErrorMessage(i,j,k); return invalidEntry;}
        return Storage->Data[i+j*Storage->m+k*Storage->mn];}
#else
    DTDoubleComplex operator()(size_t i) const {return Storage->Data[i];}
    DTDoubleComplex &operator()(size_t i) {return Storage->Data[i];}
    DTDoubleComplex operator()(size_t i,size_t j) const {return Storage->Data[i+j*Storage->m];}
    DTDoubleComplex &operator()(size_t i,size_t j) {return Storage->Data[i+j*Storage->m];}
    DTDoubleComplex operator()(size_t i,size_t j,size_t k) const {return Storage->Data[i+j*Storage->m+k*Storage->mn];}
    DTDoubleComplex &operator()(size_t i,size_t j,size_t k) {return Storage->Data[i+j*Storage->m+k*Storage->mn];}
#endif
    
    void operator*=(double);
};

extern DTMutableDoubleComplexArray operator*(const DTDoubleComplexArray &,double v);
extern DTMutableDoubleComplexArray operator+(const DTDoubleComplexArray &,const DTDoubleComplexArray &);

bool operator==(const DTDoubleComplexArray &A,const DTDoubleComplexArray &B);
bool operator!=(const DTDoubleComplexArray &A,const DTDoubleComplexArray &B);

// Misc
extern DTMutableDoubleComplexArray Transpose(const DTDoubleComplexArray &A);
extern DTMutableDoubleComplexArray Reshape(const DTDoubleComplexArray &A,ssize_t m,ssize_t n=1,ssize_t o=1);
extern DTMutableDoubleComplexArray FlipJ(const DTDoubleComplexArray &A);

extern DTMutableDoubleComplexArray CombineColumns(const DTDoubleComplexArray &First,const DTDoubleComplexArray &Second);

// Changing the size of an array
extern DTMutableDoubleComplexArray TruncateSize(const DTDoubleComplexArray &A,ssize_t length);
extern DTMutableDoubleComplexArray IncreaseSize(const DTDoubleComplexArray &A,ssize_t addLength);

// Casting back and forth
extern DTMutableDoubleComplexArray ConvertToDoubleComplex(const DTDoubleArray &);
extern DTMutableDoubleArray RealPart(const DTDoubleComplexArray &);
extern DTMutableDoubleArray ImaginaryPart(const DTDoubleComplexArray &);

#endif
