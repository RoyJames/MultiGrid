// Part of DTSource. Copyright 2004-2017. David Adalsteinsson.
// see http://www.visualdatatools.com/DTSource/license.html for more information.

#include "DTIntArrayRegion.h"
#include "DTArrayRegionTemplates.h"

#include "DTError.h"

const DTIntArrayRegion DTIntArray::operator()(DTIndex I) const
{
    return DTIntArrayRegion(Storage,I);
}

const DTIntArrayRegion DTIntArray::operator()(DTIndex I,DTIndex J) const
{
    return DTIntArrayRegion(Storage,I,J);
}

const DTIntArrayRegion DTIntArray::operator()(DTIndex I,DTIndex J,DTIndex K) const
{
    return DTIntArrayRegion(Storage,I,J,K);
}

const DTIntArrayRegion DTMutableIntArray::operator()(DTIndex I) const
{
    return DTIntArrayRegion(Storage,I);
}

const DTIntArrayRegion DTMutableIntArray::operator()(DTIndex I,DTIndex J) const
{
    return DTIntArrayRegion(Storage,I,J);
}

const DTIntArrayRegion DTMutableIntArray::operator()(DTIndex I,DTIndex J,DTIndex K) const
{
    return DTIntArrayRegion(Storage,I,J,K);
}

DTIntArrayRegion DTMutableIntArray::operator()(DTIndex I)
{
    return DTIntArrayRegion(Storage,I);
}

DTIntArrayRegion DTMutableIntArray::operator()(DTIndex I,DTIndex J)
{
    return DTIntArrayRegion(Storage,I,J);
}

DTIntArrayRegion DTMutableIntArray::operator()(DTIndex I,DTIndex J,DTIndex K)
{
    return DTIntArrayRegion(Storage,I,J,K);
}

DTIntArrayRegion::DTIntArrayRegion(DTIntArrayStorage *ptr,DTIndex inI)
{
    storage = ptr;
    I = inI;
    if (I.SetLength(storage->length)==false) {
        // Error, the user has been notified.
        I = DTIndex();
    }
}

DTIntArrayRegion::DTIntArrayRegion(DTIntArrayStorage *ptr,DTIndex inI,DTIndex inJ)
{
    storage = ptr;
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

DTIntArrayRegion::DTIntArrayRegion(DTIntArrayStorage *ptr,DTIndex inI,DTIndex inJ,DTIndex inK)
{
    storage = ptr;
    I = inI;
    J = inJ;
    K = inK;
    if (I.SetLength(storage->m)==false || J.SetLength(storage->n)==false || K.SetLength(storage->o)==false) {
        I = DTIndex();
        J = DTIndex();
        K = DTIndex();
    }
}

DTMutableIntArray DTIntArrayRegion::ConvertToArray() const
{
    return DTRegionConvertToArray<DTIntArrayStorage,DTMutableIntArray,int>(storage,I,J,K);
}

DTIntArrayRegion::operator DTIntArray() const
{
    // Array = region
    return ConvertToArray();
}

DTIntArrayRegion::operator DTMutableIntArray() const
{
    // Mutable array = region
    return ConvertToArray();
}

DTIntArrayRegion &DTIntArrayRegion::operator=(const DTIntArray &A)
{
    DTRegionAssignArray<DTIntArrayStorage,DTIntArray,int>(storage,I,J,K,A);
    return *this;
}

DTIntArrayRegion &DTIntArrayRegion::operator=(int v)
{
    DTRegionAssignValue<DTIntArrayStorage,int>(storage,I,J,K,v);
    return *this;
}

DTIntArrayRegion &DTIntArrayRegion::operator=(const DTIntArrayRegion &R)
{
    // Region = Region
    *this = DTIntArray(R);
    return *this;
}

