// Part of DTSource. Copyright 2004-2017. David Adalsteinsson.
// see http://www.visualdatatools.com/DTSource/license.html for more information.

#include "DTUShortIntArrayRegion.h"
#include "DTArrayRegionTemplates.h"

#include "DTError.h"

const DTUShortIntArrayRegion DTUShortIntArray::operator()(DTIndex I) const
{
    return DTUShortIntArrayRegion(Storage,I);
}

const DTUShortIntArrayRegion DTUShortIntArray::operator()(DTIndex I,DTIndex J) const
{
    return DTUShortIntArrayRegion(Storage,I,J);
}

const DTUShortIntArrayRegion DTUShortIntArray::operator()(DTIndex I,DTIndex J,DTIndex K) const
{
    return DTUShortIntArrayRegion(Storage,I,J,K);
}

const DTUShortIntArrayRegion DTMutableUShortIntArray::operator()(DTIndex I) const
{
    return DTUShortIntArrayRegion(Storage,I);
}

const DTUShortIntArrayRegion DTMutableUShortIntArray::operator()(DTIndex I,DTIndex J) const
{
    return DTUShortIntArrayRegion(Storage,I,J);
}

const DTUShortIntArrayRegion DTMutableUShortIntArray::operator()(DTIndex I,DTIndex J,DTIndex K) const
{
    return DTUShortIntArrayRegion(Storage,I,J,K);
}

DTUShortIntArrayRegion DTMutableUShortIntArray::operator()(DTIndex I)
{
    return DTUShortIntArrayRegion(Storage,I);
}

DTUShortIntArrayRegion DTMutableUShortIntArray::operator()(DTIndex I,DTIndex J)
{
    return DTUShortIntArrayRegion(Storage,I,J);
}

DTUShortIntArrayRegion DTMutableUShortIntArray::operator()(DTIndex I,DTIndex J,DTIndex K)
{
    return DTUShortIntArrayRegion(Storage,I,J,K);
}

DTUShortIntArrayRegion::DTUShortIntArrayRegion(DTUShortIntArrayStorage *ptr,DTIndex inI)
{
    storage = ptr;
    I = inI;
    if (I.SetLength(storage->length)==false) {
        // Error, the user has been notified.
        I = DTIndex();
    }
}

DTUShortIntArrayRegion::DTUShortIntArrayRegion(DTUShortIntArrayStorage *ptr,DTIndex inI,DTIndex inJ)
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

DTUShortIntArrayRegion::DTUShortIntArrayRegion(DTUShortIntArrayStorage *ptr,DTIndex inI,DTIndex inJ,DTIndex inK)
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

DTMutableUShortIntArray DTUShortIntArrayRegion::ConvertToArray() const
{
    return DTRegionConvertToArray<DTUShortIntArrayStorage,DTMutableUShortIntArray,unsigned short int>(storage,I,J,K);
}

DTUShortIntArrayRegion::operator DTUShortIntArray() const
{
    // Array = region
    return ConvertToArray();
}

DTUShortIntArrayRegion::operator DTMutableUShortIntArray() const
{
    // Mutable array = region
    return ConvertToArray();
}

DTUShortIntArrayRegion &DTUShortIntArrayRegion::operator=(const DTUShortIntArray &A)
{
    DTRegionAssignArray<DTUShortIntArrayStorage,DTUShortIntArray,unsigned short int>(storage,I,J,K,A);
    return *this;
}

DTUShortIntArrayRegion &DTUShortIntArrayRegion::operator=(unsigned short int v)
{
    DTRegionAssignValue<DTUShortIntArrayStorage,unsigned short int>(storage,I,J,K,v);
    return *this;
}

DTUShortIntArrayRegion &DTUShortIntArrayRegion::operator=(const DTUShortIntArrayRegion &R)
{
    // Region = Region
    *this = DTUShortIntArray(R);
    return *this;
}

