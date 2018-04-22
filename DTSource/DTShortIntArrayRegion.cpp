// Part of DTSource. Copyright 2004-2017. David Adalsteinsson.
// see http://www.visualdatatools.com/DTSource/license.html for more information.

#include "DTShortIntArrayRegion.h"
#include "DTArrayRegionTemplates.h"

#include "DTError.h"

const DTShortIntArrayRegion DTShortIntArray::operator()(DTIndex I) const
{
    return DTShortIntArrayRegion(Storage,I);
}

const DTShortIntArrayRegion DTShortIntArray::operator()(DTIndex I,DTIndex J) const
{
    return DTShortIntArrayRegion(Storage,I,J);
}

const DTShortIntArrayRegion DTShortIntArray::operator()(DTIndex I,DTIndex J,DTIndex K) const
{
    return DTShortIntArrayRegion(Storage,I,J,K);
}

const DTShortIntArrayRegion DTMutableShortIntArray::operator()(DTIndex I) const
{
    return DTShortIntArrayRegion(Storage,I);
}

const DTShortIntArrayRegion DTMutableShortIntArray::operator()(DTIndex I,DTIndex J) const
{
    return DTShortIntArrayRegion(Storage,I,J);
}

const DTShortIntArrayRegion DTMutableShortIntArray::operator()(DTIndex I,DTIndex J,DTIndex K) const
{
    return DTShortIntArrayRegion(Storage,I,J,K);
}

DTShortIntArrayRegion DTMutableShortIntArray::operator()(DTIndex I)
{
    return DTShortIntArrayRegion(Storage,I);
}

DTShortIntArrayRegion DTMutableShortIntArray::operator()(DTIndex I,DTIndex J)
{
    return DTShortIntArrayRegion(Storage,I,J);
}

DTShortIntArrayRegion DTMutableShortIntArray::operator()(DTIndex I,DTIndex J,DTIndex K)
{
    return DTShortIntArrayRegion(Storage,I,J,K);
}

DTShortIntArrayRegion::DTShortIntArrayRegion(DTShortIntArrayStorage *ptr,DTIndex inI)
{
    storage = ptr;
    I = inI;
    if (I.SetLength(storage->length)==false) {
        // Error, the user has been notified.
        I = DTIndex();
    }
}

DTShortIntArrayRegion::DTShortIntArrayRegion(DTShortIntArrayStorage *ptr,DTIndex inI,DTIndex inJ)
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

DTShortIntArrayRegion::DTShortIntArrayRegion(DTShortIntArrayStorage *ptr,DTIndex inI,DTIndex inJ,DTIndex inK)
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

DTMutableShortIntArray DTShortIntArrayRegion::ConvertToArray() const
{
    return DTRegionConvertToArray<DTShortIntArrayStorage,DTMutableShortIntArray,short int>(storage,I,J,K);
}

DTShortIntArrayRegion::operator DTShortIntArray() const
{
    // Array = region
    return ConvertToArray();
}

DTShortIntArrayRegion::operator DTMutableShortIntArray() const
{
    // Mutable array = region
    return ConvertToArray();
}

DTShortIntArrayRegion &DTShortIntArrayRegion::operator=(const DTShortIntArray &A)
{
    DTRegionAssignArray<DTShortIntArrayStorage,DTShortIntArray,short int>(storage,I,J,K,A);
    return *this;
}

DTShortIntArrayRegion &DTShortIntArrayRegion::operator=(short int v)
{
    DTRegionAssignValue<DTShortIntArrayStorage,short int>(storage,I,J,K,v);
    return *this;
}

DTShortIntArrayRegion &DTShortIntArrayRegion::operator=(const DTShortIntArrayRegion &R)
{
    // Region = Region
    *this = DTShortIntArray(R);
    return *this;
}

