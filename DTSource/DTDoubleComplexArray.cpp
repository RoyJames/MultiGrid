// Part of DTSource. Copyright 2008-2017. David Adalsteinsson.
// see http://www.visualdatatools.com/DTSource/license.html for more information.

#include "DTDoubleComplexArray.h"

#include "DTDoubleArray.h"
#include "DTError.h"

#include "DTArrayTemplates.h"

#include <math.h>
#include <cstring>
#include <algorithm>
#include <limits>

#if !defined(INFINITY)
#if defined(WIN32)
#define INFINITY std::numeric_limits<float>::infinity();
#else
#define INFINITY 1e50f
#endif
#endif

DTDoubleComplexArrayStorage::DTDoubleComplexArrayStorage(ssize_t mv,ssize_t nv,ssize_t ov)
{
    // Check if it's called correctly.
    m = mv>0 ? mv : 0;
    n = nv>0 ? nv : 0;
    o = ov>0 ? ov : 0;
    length = m*n*o;
    if (length==0) m = n = o = 0;
    referenceCount = 1;
    mn = m*n;
    mutableReferences = 0;
    
    Data = length==0 ? NULL : new DTDoubleComplex[length];
}

DTDoubleComplexArrayStorage::~DTDoubleComplexArrayStorage()
{
    delete Data;
}

DTDoubleComplexArray::~DTDoubleComplexArray()
{
    if (Storage) {
        accessLock.Lock();
        Storage->accessLock.Lock();
        int refCnt = (--Storage->referenceCount);
        Storage->accessLock.Unlock();
        if (refCnt==0) delete Storage;
        accessLock.Unlock();
    }
}

DTDoubleComplexArray::DTDoubleComplexArray(const DTDoubleComplexArray &A) 
{
    accessLock.Lock();
    Storage = A.Storage;
    Storage->accessLock.Lock();
    Storage->referenceCount++;
    Storage->accessLock.Unlock();
    accessLock.Unlock();
}

DTDoubleComplexArray &DTDoubleComplexArray::operator=(const DTDoubleComplexArray &A)
{
    if (accessLock==A.accessLock) {
        // A=A, safe but pointless.
        return *this;
    }
    
    // Allow A = A
    accessLock.Lock();
    A.accessLock.Lock();
    
    if (Storage!=A.Storage) {
        Storage->accessLock.Lock();
        A.Storage->accessLock.Lock();
        Storage->referenceCount--;
        int refCnt = Storage->referenceCount;
        Storage->accessLock.Unlock();
        if (refCnt==0) delete Storage;
        Storage = A.Storage;
        Storage->referenceCount++;
        Storage->accessLock.Unlock();
    }
    
    A.accessLock.Unlock();
    accessLock.Unlock();
    
    return *this;
}

int DTDoubleComplexArray::ReferenceCount() const
{
    accessLock.Lock();
    Storage->accessLock.Lock(); 
    int refCnt = Storage->referenceCount;
    Storage->accessLock.Unlock();
    accessLock.Unlock();
    return refCnt;
}

int DTDoubleComplexArray::MutableReferences() const
{
    accessLock.Lock();
    Storage->accessLock.Lock(); 
    int toReturn = Storage->mutableReferences;
    Storage->accessLock.Unlock();
    accessLock.Unlock();
    return toReturn;
}

ssize_t DTDoubleComplexArray::m() const
{
    return Storage->m;
}

ssize_t DTDoubleComplexArray::n() const
{
    return Storage->n;
}

ssize_t DTDoubleComplexArray::o() const
{
    return Storage->o;
}

ssize_t DTDoubleComplexArray::Length() const
{
    return Storage->length;
}

bool DTDoubleComplexArray::IsEmpty() const
{
    return (Storage->length==0);
}

bool DTDoubleComplexArray::NotEmpty() const
{
    return (Storage->length!=0);
}

void DTDoubleComplexArray::pinfo(void) const
{
    if (o()==0)
        std::cerr << "Empty" << std::endl;
    else if (o()==1) {
        if (n()==1) {
            if (m()==1) {
                std::cerr << "complex array with one entry" << std::endl;
            }
            else {
                std::cerr << "complex vector with " << m() << " entries" << std::endl;
            }
        }
        else
            std::cerr << m() << " x " << n() << " complex array" << std::endl;
    }
    else
        std::cerr << m() << " x " << n() << " x " << o() << " complex array" << std::endl;
    
    std::cerr << flush;
}

void DTDoubleComplexArray::pall(void) const
{
    size_t mv = m();
    size_t nv = n();
    size_t i,j;
    if (mv==0) {
        std::cerr << "Empty" << std::endl;
    }
    else {
        DTDoubleComplex v;
        for (j=0;j<nv;j++) {
            for (i=0;i<mv-1;i++) {
                v = operator()(i,j);
                std::cerr << v << ", ";
            }
            v = operator()(i,j);
            std::cerr << v << std::endl;
        }
    }
}

void DTDoubleComplexArray::pe(int i,int j) const
{
    DTDoubleComplex v = operator()(i,j);
    std::cerr << v << std::endl;
}

ssize_t DTDoubleComplexArray::Find(DTDoubleComplex v) const
{
    const DTDoubleComplex *D = Pointer();
    size_t len = Length();
    size_t i;
    for (i=0;i<len;i++) {
        if (D[i]==v) break;
    }

    return (i<len ? i : -1);
}

DTMutableDoubleComplexArray DTDoubleComplexArray::Copy() const
{
    DTMutableDoubleComplexArray CopyInto(m(),n(),o());
    // Check that the allocation worked.
    if (CopyInto.Length()!=Length()) return CopyInto; // Failed.  Already printed an error message.
    std::memcpy(CopyInto.Pointer(),Pointer(),Length()*sizeof(DTDoubleComplex));
    return CopyInto;
}

void DTDoubleComplexArray::PrintErrorMessage(ssize_t i) const
{
    DTErrorOutOfRange("DTDoubleComplexArray",i,Storage->length);
}

void DTDoubleComplexArray::PrintErrorMessage(ssize_t i,ssize_t j) const
{
    DTErrorOutOfRange("DTDoubleComplexArray",i,j,Storage->m,Storage->n);
}

void DTDoubleComplexArray::PrintErrorMessage(ssize_t i,ssize_t j,ssize_t k) const
{
    DTErrorOutOfRange("DTDoubleComplexArray",i,j,k,Storage->m,Storage->n,Storage->o);
}

DTMutableDoubleComplexArray::DTMutableDoubleComplexArray(const DTMutableDoubleComplexArray &A)
{
    accessLock.Lock();
    Storage = A.Storage;
    Storage->accessLock.Lock();
    Storage->referenceCount++;
    Storage->mutableReferences++;
    Storage->accessLock.Unlock();
    accessLock.Unlock();
}

DTMutableDoubleComplexArray::~DTMutableDoubleComplexArray()
{
    accessLock.Lock();
    Storage->accessLock.Lock();
    int refCnt = (--Storage->referenceCount);
    Storage->mutableReferences--;
    Storage->accessLock.Unlock();
    if (refCnt==0) delete Storage;
    Storage = NULL;
    accessLock.Unlock();
}

DTMutableDoubleComplexArray &DTMutableDoubleComplexArray::operator=(const DTMutableDoubleComplexArray &A)
{
    if (accessLock==A.accessLock) {
        // A=A, safe but pointless.
        return *this;
    }
    
    // Allow A = A
    accessLock.Lock();
    A.accessLock.Lock();
    
    if (Storage!=A.Storage) {
        Storage->accessLock.Lock();
        A.Storage->accessLock.Lock();
        Storage->referenceCount--;
        int refCnt = Storage->referenceCount;
        Storage->mutableReferences--;
        Storage->accessLock.Unlock();
        if (refCnt==0) delete Storage;
        Storage = A.Storage;
        Storage->referenceCount++;
        Storage->mutableReferences++;
        Storage->accessLock.Unlock();
    }
    
    A.accessLock.Unlock();
    accessLock.Unlock();
    
    return *this;
}

DTMutableDoubleComplexArray &DTMutableDoubleComplexArray::operator=(DTDoubleComplex a)
{
    const size_t howManyNumbers = Length();
    if (a==0.0) {
        memset(Pointer(),0,sizeof(DTDoubleComplex)*howManyNumbers);
    }
    else {
        size_t i;
        DTDoubleComplex *Data = Pointer();
        for (i=0;i<howManyNumbers;i++)
            Data[i] = a;
    }
    
    return *this;
}

void DTMutableDoubleComplexArray::operator*=(double v)
{
    double *D = (double *)Storage->Data;
    size_t i,howMany = Length()*2;
    for (i=0;i<howMany;i++) {
        D[i] *= v;
    }
}

DTMutableDoubleComplexArray operator*(const DTDoubleComplexArray &A,double b)
{
    DTMutableDoubleComplexArray toReturn = A.Copy();
    size_t i,len = toReturn.Length();
    DTDoubleComplex *D = toReturn.Pointer();
    for (i=0;i<len;i++) {
        D[i] *= b;
    }
    return toReturn;
}

DTMutableDoubleComplexArray operator+(const DTDoubleComplexArray &A,const DTDoubleComplexArray &B)
{
    return DTAddArrays<DTDoubleComplexArray,DTMutableDoubleComplexArray,DTDoubleComplex>("ComplexDoubleArray+ComplexDoubleArray",A,B);
}

bool operator==(const DTDoubleComplexArray &A,const DTDoubleComplexArray &B)
{
    return DTOperatorArrayEqualsArray<DTDoubleComplexArray,DTDoubleComplex>(A,B);
}

bool operator!=(const DTDoubleComplexArray &A,const DTDoubleComplexArray &B)
{
    return !(A==B);
}

DTMutableDoubleComplexArray TruncateSize(const DTDoubleComplexArray &A,ssize_t length)
{
    return DTTruncateArraySize<DTDoubleComplexArray,DTMutableDoubleComplexArray,DTDoubleComplex>(A,length);
}

DTMutableDoubleComplexArray IncreaseSize(const DTDoubleComplexArray &A,ssize_t addLength)
{
    return DTIncreaseArraySize<DTDoubleComplexArray,DTMutableDoubleComplexArray,DTDoubleComplex>(A,addLength);
}

DTMutableDoubleComplexArray Transpose(const DTDoubleComplexArray &A)
{
    return DTTransposeArray<DTDoubleComplexArray,DTMutableDoubleComplexArray,DTDoubleComplex>(A);
}

DTMutableDoubleComplexArray Reshape(const DTDoubleComplexArray &A,ssize_t m,ssize_t n,ssize_t o)
{
    if (m<0 || n<0 || o<0) {
        DTErrorMessage("Reshape(DTDoubleComplexArray,...)","One of the new dimensions is negative.");
        return DTMutableDoubleComplexArray();
    }
    if (m*n*o!=A.Length()) {
        DTErrorMessage("Reshape(DTDoubleComplexArray,...)","Size before and after need to be the same.");
        return DTMutableDoubleComplexArray();
    }
    
    DTMutableDoubleComplexArray toReturn(m,n,o);
    if (toReturn.Length()) {
        std::memcpy(toReturn.Pointer(),A.Pointer(),A.Length()*sizeof(DTDoubleComplex));
    }
    
    return toReturn;
}

DTMutableDoubleComplexArray FlipJ(const DTDoubleComplexArray &A)
{
    return DTArrayFlipJ<DTDoubleComplexArray,DTMutableDoubleComplexArray,DTDoubleComplex>(A);
}

DTMutableDoubleComplexArray CombineColumns(const DTDoubleComplexArray &First,const DTDoubleComplexArray &Second)
{
    if (First.m()!=Second.m()) {
        DTErrorMessage("CombineColumns(A,B)","A and B have to have the same number of rows.");
        return DTMutableDoubleComplexArray();
    }
    if (First.IsEmpty())
        return DTMutableDoubleComplexArray();
    if (First.o()!=1 || Second.o()!=1) {
        DTErrorMessage("CombineColumns(A,B)","A and B have to be two dimensional.");
        return DTMutableDoubleComplexArray();
    }
    
    DTMutableDoubleComplexArray toReturn(First.m(),First.n()+Second.n());
    std::memcpy(toReturn.Pointer(),First.Pointer(),First.Length()*sizeof(DTDoubleComplex));
    std::memcpy(toReturn.Pointer()+First.Length(),Second.Pointer(),Second.Length()*sizeof(DTDoubleComplex));
    
    return toReturn;
}

DTMutableDoubleComplexArray ConvertToDoubleComplex(const DTDoubleArray &A)
{
    DTMutableDoubleComplexArray toReturn(A.m(),A.n(),A.o());
    int i=0;
    ssize_t len = A.Length();
    const double *D = A.Pointer();
    DTDoubleComplex *retD = toReturn.Pointer();
    for (i=0;i<len;i++) {
        retD[i] = D[i];
    }
    return toReturn;
}

DTMutableDoubleArray RealPart(const DTDoubleComplexArray &A)
{
    DTMutableDoubleArray toReturn(A.m(),A.n(),A.o());
    const DTDoubleComplex *D = A.Pointer();
    size_t i,len = toReturn.Length();
    double *retD = toReturn.Pointer();
    for (i=0;i<len;i++)
        retD[i] = real(D[i]);
    
    return toReturn;
}

DTMutableDoubleArray ImaginaryPart(const DTDoubleComplexArray &A)
{
    DTMutableDoubleArray toReturn(A.m(),A.n(),A.o());
    const DTDoubleComplex *D = A.Pointer();
    size_t i,len = toReturn.Length();
    double *retD = toReturn.Pointer();
    for (i=0;i<len;i++)
        retD[i] = imag(D[i]);
    
    return toReturn;
}

