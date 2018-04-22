// Part of DTSource. Copyright 2004-2015. David A. David Adalsteinsson.
// see http://www.visualdatatools.com/DTSource/license.html for more information.

#include "DTError.h"
#include <unistd.h>
#include <string.h>
#include <cstring>

template <class TStorage,class TM,class Td>
TM DTRegionConvertToArray(TStorage *storage,const DTIndex &I,const DTIndex &J,const DTIndex &K)
{
    ssize_t dim = 0;
    if (!I.IsEmpty()) dim++;
    if (!J.IsEmpty()) dim++;
    if (!K.IsEmpty()) dim++;
    
    TM toReturn;
    DTIntArray Ioff,Joff,Koff;
    ssize_t retm,retn,reto,i,j,k;
    const Td *fromD = storage->Data;
    Td *toD;
    ssize_t m = storage->m;
    ssize_t n = storage->n;
    
    if (dim==1) {
        Ioff = I.IndicesAsArray();
        retm = Ioff.Length();
        toReturn = TM(retm);
        toD = toReturn.Pointer();
        for (i=0;i<retm;i++)
            toD[i] = fromD[Ioff(i)];
    }
    else if (dim==2) {
        Ioff = I.IndicesAsArray();
        Joff = J.IndicesAsArray();
        retm = Ioff.Length();
        retn = Joff.Length();
        toReturn = TM(retm,retn);
        toD = toReturn.Pointer();
        ssize_t joff;
        ssize_t ij = 0;
        for (j=0;j<retn;j++) {
            joff = Joff(j)*m;
            for (i=0;i<retm;i++) {
                toD[ij++] = fromD[Ioff(i) + joff];
            }
        }
    }
    else if (dim==3) {
        ssize_t mn = m*n;
        Ioff = I.IndicesAsArray();
        Joff = J.IndicesAsArray();
        Koff = K.IndicesAsArray();
        retm = Ioff.Length();
        retn = Joff.Length();
        reto = Koff.Length();
        toReturn = TM(retm,retn,reto);
        toD = toReturn.Pointer();
        ssize_t ijk = 0;
        ssize_t koff,jkoff;
        for (k=0;k<reto;k++) {
            koff = Koff(k)*mn;
            for (j=0;j<retn;j++) {
                jkoff = Joff(j)*m + koff;
                for (i=0;i<retm;i++) {
                    toD[ijk++] = fromD[Ioff(i) + jkoff];
                }
            }
        }
    }
    
    return toReturn;
}

template <class TStorage,class T,class Td>
void DTRegionAssignArray(TStorage *storage,const DTIndex &I,const DTIndex &J,const DTIndex &K,const T &A)
{
    // Region = Array
    
    // Is this 1, 2 or 3D array.
    ssize_t dim = 0;
    if (!I.IsEmpty()) dim++;
    if (!J.IsEmpty()) dim++;
    if (!K.IsEmpty()) dim++;
    
    DTIntArray Ioff,Joff,Koff;
    ssize_t retm,retn,reto,i,j,k;
    Td *fromD = storage->Data;
    ssize_t m = storage->m;
    ssize_t n = storage->n;
    
    if (dim==1) {
        Ioff = I.IndicesAsArray();
        retm = Ioff.Length();
        if (A.Length()!=retm) {
            DTErrorMessage("Array(Region) = ***","Incompatible array sizes.");
            return;
        }
        for (i=0;i<retm;i++)
            fromD[Ioff(i)] = A(i);
    }
    else if (dim==2) {
        Ioff = I.IndicesAsArray();
        Joff = J.IndicesAsArray();
        retm = Ioff.Length();
        retn = Joff.Length();
        if (A.m()!=retm || A.n()!=retn || A.o()!=1) {
            DTErrorMessage("Array(Region) = ***","Incompatible array sizes.");
            return;
        }
        ssize_t joff;
        for (j=0;j<retn;j++) {
            joff = Joff(j)*m;
            for (i=0;i<retm;i++) {
                fromD[Ioff(i) + joff] = A(i,j);
            }
        }
    }
    else if (dim==3) {
        ssize_t mn = m*n;
        if (A.m()!=I.Length() || A.n()!=J.Length() || A.o()!=K.Length()) {
            DTErrorMessage("Array(Region) = ***","Incompatible array sizes.");
            return;
        }
        // See if these are slabs
        if (I.IsEverything() && J.IsEverything()) {
            Koff = K.IndicesAsArray();
            reto = Koff.Length();
            for (k=0;k<reto;k++) {
                std::memcpy(fromD+mn*Koff(k),A.Pointer()+mn*k,mn*sizeof(Td));
            }
        }
        else {
            Ioff = I.IndicesAsArray();
            Joff = J.IndicesAsArray();
            Koff = K.IndicesAsArray();
            retm = Ioff.Length();
            retn = Joff.Length();
            reto = Koff.Length();
            ssize_t koff,jkoff;
            for (k=0;k<reto;k++) {
                koff = Koff(k)*mn;
                for (j=0;j<retn;j++) {
                    jkoff = Joff(j)*m + koff;
                    for (i=0;i<retm;i++) {
                        fromD[Ioff(i) + jkoff] = A(i,j,k);
                    }
                }
            }
        }
    }
}


template <class TStorage,class Td>
void DTRegionAssignValue(TStorage *storage,const DTIndex &I,const DTIndex &J,const DTIndex &K,Td v)
{
    // Region = value
    
    // Is this 1, 2 or 3D array.
    ssize_t dim = 0;
    if (!I.IsEmpty()) dim++;
    if (!J.IsEmpty()) dim++;
    if (!K.IsEmpty()) dim++;
    
    DTIntArray Ioff,Joff,Koff;
    ssize_t retm,retn,reto,i,j,k;
    Td *fromD = storage->Data;
    ssize_t m = storage->m;
    ssize_t n = storage->n;
    
    if (dim==1) {
        Ioff = I.IndicesAsArray();
        retm = Ioff.Length();
        for (i=0;i<retm;i++)
            fromD[Ioff(i)] = v;
    }
    else if (dim==2) {
        Ioff = I.IndicesAsArray();
        Joff = J.IndicesAsArray();
        retm = Ioff.Length();
        retn = Joff.Length();
        ssize_t joff;
        for (j=0;j<retn;j++) {
            joff = Joff(j)*m;
            for (i=0;i<retm;i++) {
                fromD[Ioff(i) + joff] = v;
            }
        }
    }
    else if (dim==3) {
        ssize_t mn = m*n;
        Ioff = I.IndicesAsArray();
        Joff = J.IndicesAsArray();
        Koff = K.IndicesAsArray();
        retm = Ioff.Length();
        retn = Joff.Length();
        reto = Koff.Length();
        ssize_t koff,jkoff;
        for (k=0;k<reto;k++) {
            koff = Koff(k)*mn;
            for (j=0;j<retn;j++) {
                jkoff = Joff(j)*m + koff;
                for (i=0;i<retm;i++) {
                    fromD[Ioff(i) + jkoff] = v;
                }
            }
        }
    }
}

