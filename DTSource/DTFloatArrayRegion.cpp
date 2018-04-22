// Part of DTSource. Copyright 2004-2015. David A. David Adalsteinsson.
// see http://www.visualdatatools.com/DTSource/license.html for more information.

#include "DTFloatArrayRegion.h"
#include "DTArrayRegionTemplates.h"

#include "DTError.h"

const DTFloatArrayRegion DTFloatArray::operator()(DTIndex I) const
{
    return DTFloatArrayRegion(Storage,I);
}

const DTFloatArrayRegion DTFloatArray::operator()(DTIndex I,DTIndex J) const
{
    return DTFloatArrayRegion(Storage,I,J);
}

const DTFloatArrayRegion DTFloatArray::operator()(DTIndex I,DTIndex J,DTIndex K) const
{
    return DTFloatArrayRegion(Storage,I,J,K);
}

const DTFloatArrayRegion DTMutableFloatArray::operator()(DTIndex I) const
{
    return DTFloatArrayRegion(Storage,I);
}

const DTFloatArrayRegion DTMutableFloatArray::operator()(DTIndex I,DTIndex J) const
{
    return DTFloatArrayRegion(Storage,I,J);
}

const DTFloatArrayRegion DTMutableFloatArray::operator()(DTIndex I,DTIndex J,DTIndex K) const
{
    return DTFloatArrayRegion(Storage,I,J,K);
}

DTFloatArrayRegion DTMutableFloatArray::operator()(DTIndex I)
{
    return DTFloatArrayRegion(Storage,I);
}

DTFloatArrayRegion DTMutableFloatArray::operator()(DTIndex I,DTIndex J)
{
    return DTFloatArrayRegion(Storage,I,J);
}

DTFloatArrayRegion DTMutableFloatArray::operator()(DTIndex I,DTIndex J,DTIndex K)
{
    return DTFloatArrayRegion(Storage,I,J,K);
}

DTFloatArrayRegion::DTFloatArrayRegion(DTFloatArrayStorage *ptr,DTIndex inI)
{
    storage = ptr;
    I = inI;
    if (I.SetLength(storage->length)==false) {
        // Error, the user has been notified.
        I = DTIndex();
    }
}

DTFloatArrayRegion::DTFloatArrayRegion(DTFloatArrayStorage *ptr,DTIndex inI,DTIndex inJ)
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

DTFloatArrayRegion::DTFloatArrayRegion(DTFloatArrayStorage *ptr,DTIndex inI,DTIndex inJ,DTIndex inK)
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

DTMutableFloatArray DTFloatArrayRegion::ConvertToArray() const
{
    return DTRegionConvertToArray<DTFloatArrayStorage,DTMutableFloatArray,float>(storage,I,J,K);
}

DTFloatArrayRegion::operator DTFloatArray() const
{
    // Array = region
    return ConvertToArray();
}

DTFloatArrayRegion::operator DTMutableFloatArray() const
{
    // Mutable array = region
    return ConvertToArray();
}

DTFloatArrayRegion &DTFloatArrayRegion::operator=(const DTFloatArray &A)
{
    DTRegionAssignArray<DTFloatArrayStorage,DTFloatArray,float>(storage,I,J,K,A);
    return *this;
}

DTFloatArrayRegion &DTFloatArrayRegion::operator=(float v)
{
    DTRegionAssignValue<DTFloatArrayStorage,float>(storage,I,J,K,v);
    return *this;
}

DTFloatArrayRegion &DTFloatArrayRegion::operator=(const DTFloatArrayRegion &R)
{
    // Region = Region
    *this = DTFloatArray(R);
    return *this;
}

