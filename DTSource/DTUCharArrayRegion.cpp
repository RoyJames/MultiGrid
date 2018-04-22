// Part of DTSource. Copyright 2004-2017. David Adalsteinsson.
// see http://www.visualdatatools.com/DTSource/license.html for more information.

#include "DTUCharArrayRegion.h"
#include "DTArrayRegionTemplates.h"

#include "DTError.h"

const DTUCharArrayRegion DTUCharArray::operator()(DTIndex I) const
{
    return DTUCharArrayRegion(Storage,I);
}

const DTUCharArrayRegion DTUCharArray::operator()(DTIndex I,DTIndex J) const
{
    return DTUCharArrayRegion(Storage,I,J);
}

const DTUCharArrayRegion DTUCharArray::operator()(DTIndex I,DTIndex J,DTIndex K) const
{
    return DTUCharArrayRegion(Storage,I,J,K);
}

const DTUCharArrayRegion DTMutableUCharArray::operator()(DTIndex I) const
{
    return DTUCharArrayRegion(Storage,I);
}

const DTUCharArrayRegion DTMutableUCharArray::operator()(DTIndex I,DTIndex J) const
{
    return DTUCharArrayRegion(Storage,I,J);
}

const DTUCharArrayRegion DTMutableUCharArray::operator()(DTIndex I,DTIndex J,DTIndex K) const
{
    return DTUCharArrayRegion(Storage,I,J,K);
}

DTUCharArrayRegion DTMutableUCharArray::operator()(DTIndex I)
{
    return DTUCharArrayRegion(Storage,I);
}

DTUCharArrayRegion DTMutableUCharArray::operator()(DTIndex I,DTIndex J)
{
    return DTUCharArrayRegion(Storage,I,J);
}

DTUCharArrayRegion DTMutableUCharArray::operator()(DTIndex I,DTIndex J,DTIndex K)
{
    return DTUCharArrayRegion(Storage,I,J,K);
}

DTUCharArrayRegion::DTUCharArrayRegion(DTUCharArrayStorage *ptr,DTIndex inI)
{
    storage = ptr;
    I = inI;
    if (I.SetLength(storage->length)==false) {
        // Error, the user has been notified.
        I = DTIndex();
    }
}

DTUCharArrayRegion::DTUCharArrayRegion(DTUCharArrayStorage *ptr,DTIndex inI,DTIndex inJ)
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

DTUCharArrayRegion::DTUCharArrayRegion(DTUCharArrayStorage *ptr,DTIndex inI,DTIndex inJ,DTIndex inK)
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

DTMutableUCharArray DTUCharArrayRegion::ConvertToArray() const
{
    return DTRegionConvertToArray<DTUCharArrayStorage,DTMutableUCharArray,unsigned char>(storage,I,J,K);
}

DTUCharArrayRegion::operator DTUCharArray() const
{
    // Array = region
    return ConvertToArray();
}

DTUCharArrayRegion::operator DTMutableUCharArray() const
{
    // Mutable array = region
    return ConvertToArray();
}

DTUCharArrayRegion &DTUCharArrayRegion::operator=(const DTUCharArray &A)
{
    DTRegionAssignArray<DTUCharArrayStorage,DTUCharArray,unsigned char>(storage,I,J,K,A);
    return *this;
}

DTUCharArrayRegion &DTUCharArrayRegion::operator=(unsigned char v)
{
    DTRegionAssignValue<DTUCharArrayStorage,unsigned char>(storage,I,J,K,v);
    return *this;
}

DTUCharArrayRegion &DTUCharArrayRegion::operator=(const DTUCharArrayRegion &R)
{
    // Region = Region
    *this = DTUCharArray(R);
    return *this;
}

