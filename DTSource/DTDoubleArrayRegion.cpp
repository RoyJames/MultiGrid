// Part of DTSource. Copyright 2004-2017. David Adalsteinsson.
// see http://www.visualdatatools.com/DTSource/license.html for more information.

#include "DTDoubleArrayRegion.h"
#include "DTArrayRegionTemplates.h"

#include "DTError.h"

const DTDoubleArrayRegion DTDoubleArray::operator()(DTIndex I) const
{
    return DTDoubleArrayRegion(Storage,I);
}

const DTDoubleArrayRegion DTDoubleArray::operator()(DTIndex I,DTIndex J) const
{
    return DTDoubleArrayRegion(Storage,I,J);
}

const DTDoubleArrayRegion DTDoubleArray::operator()(DTIndex I,DTIndex J,DTIndex K) const
{
    return DTDoubleArrayRegion(Storage,I,J,K);
}

const DTDoubleArrayRegion DTMutableDoubleArray::operator()(DTIndex I) const
{
    return DTDoubleArrayRegion(Storage,I);
}

const DTDoubleArrayRegion DTMutableDoubleArray::operator()(DTIndex I,DTIndex J) const
{
    return DTDoubleArrayRegion(Storage,I,J);
}

const DTDoubleArrayRegion DTMutableDoubleArray::operator()(DTIndex I,DTIndex J,DTIndex K) const
{
    return DTDoubleArrayRegion(Storage,I,J,K);
}

DTDoubleArrayRegion DTMutableDoubleArray::operator()(DTIndex I)
{
    return DTDoubleArrayRegion(Storage,I);
}

DTDoubleArrayRegion DTMutableDoubleArray::operator()(DTIndex I,DTIndex J)
{
    return DTDoubleArrayRegion(Storage,I,J);
}

DTDoubleArrayRegion DTMutableDoubleArray::operator()(DTIndex I,DTIndex J,DTIndex K)
{
    return DTDoubleArrayRegion(Storage,I,J,K);
}

DTDoubleArrayRegion::DTDoubleArrayRegion(DTDoubleArrayStorage *ptr,DTIndex inI)
: storage(ptr), I(inI), J(), K()
{
    if (I.SetLength(storage->length)==false) {
        // Error, the user has been notified.
        I = DTIndex();
    }
}

DTDoubleArrayRegion::DTDoubleArrayRegion(DTDoubleArrayStorage *ptr,DTIndex inI,DTIndex inJ)
: storage(ptr), I(), J(), K()
{
    // Can only do this for a 2D array
    if (storage->o>1) {
        DTErrorMessage("Array(Region,Region)","The array is three dimensional");
        return;
    }
    else {
        I = inI;
        J = inJ;
        if (I.SetLength(storage->m)==false || J.SetLength(storage->n)==false) {
            I = DTIndex();
            J = DTIndex();
        }
    }
}

DTDoubleArrayRegion::DTDoubleArrayRegion(DTDoubleArrayStorage *ptr,DTIndex inI,DTIndex inJ,DTIndex inK)
: storage(ptr), I(inI), J(inJ), K(inK)
{
    if (I.SetLength(storage->m)==false || J.SetLength(storage->n)==false || K.SetLength(storage->o)==false) {
        I = DTIndex();
        J = DTIndex();
        K = DTIndex();
    }
}

DTMutableDoubleArray DTDoubleArrayRegion::ConvertToArray() const
{
    return DTRegionConvertToArray<DTDoubleArrayStorage,DTMutableDoubleArray,double>(storage,I,J,K);
}

DTDoubleArrayRegion::operator DTDoubleArray() const
{
    // Array = region
    return ConvertToArray();
}

DTDoubleArrayRegion::operator DTMutableDoubleArray() const
{
    // Mutable array = region
    return ConvertToArray();
}

DTDoubleArrayRegion &DTDoubleArrayRegion::operator=(const DTDoubleArray &A)
{
    DTRegionAssignArray<DTDoubleArrayStorage,DTDoubleArray,double>(storage,I,J,K,A);
    return *this;
}

DTDoubleArrayRegion &DTDoubleArrayRegion::operator=(double v)
{
    DTRegionAssignValue<DTDoubleArrayStorage,double>(storage,I,J,K,v);
    return *this;
}

DTDoubleArrayRegion &DTDoubleArrayRegion::operator=(const DTDoubleArrayRegion &R)
{
    // Region = Region
    *this = DTDoubleArray(R);
    return *this;
}

