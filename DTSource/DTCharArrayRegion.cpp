// Part of DTSource. Copyright 2004-2017. David Adalsteinsson.
// see http://www.visualdatatools.com/DTSource/license.html for more information.

#include "DTCharArrayRegion.h"
#include "DTArrayRegionTemplates.h"

#include "DTError.h"

const DTCharArrayRegion DTCharArray::operator()(DTIndex I) const
{
    return DTCharArrayRegion(Storage,I);
}

const DTCharArrayRegion DTCharArray::operator()(DTIndex I,DTIndex J) const
{
    return DTCharArrayRegion(Storage,I,J);
}

const DTCharArrayRegion DTCharArray::operator()(DTIndex I,DTIndex J,DTIndex K) const
{
    return DTCharArrayRegion(Storage,I,J,K);
}

const DTCharArrayRegion DTMutableCharArray::operator()(DTIndex I) const
{
    return DTCharArrayRegion(Storage,I);
}

const DTCharArrayRegion DTMutableCharArray::operator()(DTIndex I,DTIndex J) const
{
    return DTCharArrayRegion(Storage,I,J);
}

const DTCharArrayRegion DTMutableCharArray::operator()(DTIndex I,DTIndex J,DTIndex K) const
{
    return DTCharArrayRegion(Storage,I,J,K);
}

DTCharArrayRegion DTMutableCharArray::operator()(DTIndex I)
{
    return DTCharArrayRegion(Storage,I);
}

DTCharArrayRegion DTMutableCharArray::operator()(DTIndex I,DTIndex J)
{
    return DTCharArrayRegion(Storage,I,J);
}

DTCharArrayRegion DTMutableCharArray::operator()(DTIndex I,DTIndex J,DTIndex K)
{
    return DTCharArrayRegion(Storage,I,J,K);
}

DTCharArrayRegion::DTCharArrayRegion(DTCharArrayStorage *ptr,DTIndex inI)
: storage(ptr), I(inI), J(), K()
{
    if (I.SetLength(storage->length)==false) {
        // Error, the user has been notified.
        I = DTIndex();
    }
}

DTCharArrayRegion::DTCharArrayRegion(DTCharArrayStorage *ptr,DTIndex inI,DTIndex inJ)
: storage(ptr), I(inI), J(inJ), K()
{
    // Can only do this for a 2D array
    if (storage->o>1) {
        DTErrorMessage("Array(Region,Region)","The array is three dimensional");
        I = DTIndex();
        J = DTIndex();
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

DTCharArrayRegion::DTCharArrayRegion(DTCharArrayStorage *ptr,DTIndex inI,DTIndex inJ,DTIndex inK)
: storage(ptr), I(inI), J(inJ), K(inK)
{
    if (I.SetLength(storage->m)==false || J.SetLength(storage->n)==false || K.SetLength(storage->o)==false) {
        I = DTIndex();
        J = DTIndex();
        K = DTIndex();
    }
}

DTMutableCharArray DTCharArrayRegion::ConvertToArray() const
{
    return DTRegionConvertToArray<DTCharArrayStorage,DTMutableCharArray,char>(storage,I,J,K);
}

DTCharArrayRegion::operator DTCharArray() const
{
    // Array = region
    return ConvertToArray();
}

DTCharArrayRegion::operator DTMutableCharArray() const
{
    // Mutable array = region
    return ConvertToArray();
}

DTCharArrayRegion &DTCharArrayRegion::operator=(const DTCharArray &A)
{
    DTRegionAssignArray<DTCharArrayStorage,DTCharArray,char>(storage,I,J,K,A);
    return *this;
}

DTCharArrayRegion &DTCharArrayRegion::operator=(char v)
{
    DTRegionAssignValue<DTCharArrayStorage,char>(storage,I,J,K,v);
    return *this;
}

DTCharArrayRegion &DTCharArrayRegion::operator=(const DTCharArrayRegion &R)
{
    // Region = Region
    *this = DTCharArray(R);
    return *this;
}

