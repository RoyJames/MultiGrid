// Part of DTSource. Copyright 2004-2017. David Adalsteinsson.
// see http://www.visualdatatools.com/DTSource/license.html for more information.

#include "DTIndex.h"
#include "DTError.h"

#include "DTIntArrayOperators.h"

const ssize_t DTLastIndex = -12563; // Magic number.  Should not use negative indices

// The static variables.
ssize_t DTIndex::Last = DTLastIndex;
DTIndex DTIndex::All = DTIndex(0,DTLastIndex);

DTIndex::DTIndex()
: lengthSet(false), knownLength(0), start(0), stride(0), end(0), arraySpecified(false), specified()
{
}

DTIndex::DTIndex(ssize_t number)
: lengthSet(false), knownLength(0), start(0), stride(0), end(0), arraySpecified(false), specified()
{
    // Needs to be either >=0 or <=Last
    if (number<0 && number>DTLastIndex) {
        DTErrorMessage("Subscript error","The subscript was negative.");
        return;
    }

    start = end = number;
    stride = 1;
}

DTIndex::DTIndex(ssize_t st,ssize_t en)
: lengthSet(false), knownLength(0), start(0), stride(0), end(0), arraySpecified(false), specified()
{
    //if (st>DTLastIndex) {
    //    DTErrorMessage("DTIndex(start,end)","Invalid start (negative)");
    //    return;
    //}
    stride = 1;
    start = st;
    end = en;
}

DTIndex::DTIndex(ssize_t start_in,ssize_t stride_in,ssize_t end_in)
{
    lengthSet = false;
    knownLength = 0;
    start = stride = end = 0;
    arraySpecified = false;

    if (start_in>DTLastIndex) {
        DTErrorMessage("DTIndex(start,stride,end)","Invalid start (negative)");
        return;
    }
    if (end_in>DTLastIndex) {
        DTErrorMessage("DTIndex(start,stride,end)","Invalid end (negative)");
        return;
    }
    
    start = start_in;
    stride = stride_in;
    end = end_in;
}

DTIndex::DTIndex(const DTIntArray &spec)
{
    lengthSet = false;
    knownLength = 0;
    start = stride = end = 0;
    arraySpecified = true;

    specified = spec;
}

bool DTIndex::SetLength(ssize_t howLong)
{
    // Returns false if invalid.
    if (lengthSet) {
        DTErrorMessage("Index.SetLength(Len)","Already set the length");
        return false;
    }

    if (arraySpecified) {
        // Check to make sure that the min and max are within range.
        ssize_t i;
        ssize_t len = specified.Length();
        for (i=0;i<len;i++) {
            if (specified(i)<0 || specified(i)>=howLong)
                break;
        }
        if (i<len) {
            DTErrorMessage("DTIndex(Array)","Array element out of bounds");
            return false;
        }
    }
    else {
        if (end<=DTLastIndex) {
            // passed in last-k when the index was created.
            end = howLong-1-(DTLastIndex-end);
        }
        if (start<=DTLastIndex) {
            // passed in last-k when the index was created.
            start = howLong-1-(DTLastIndex-start);
        }
        if (end>=howLong || start>=howLong) {
            // start or end outside the current range.
            DTErrorMessage("DTIndex(info)","Index out of range");
            return false;
        }
        if (stride>0 && start>end) {
            // An empty matrix, but this is ok.
            stride = 0;
        }
        else if (start>end) {
            stride = 0;
        }
    }

    knownLength = howLong;
    lengthSet = true;

    return true;
}

ssize_t DTIndex::Length(void) const
{
    if (arraySpecified)
        return specified.Length(); 
    else
        return (end-start)/stride+1;
}

bool DTIndex::IsEmpty(void) const
{
    if (arraySpecified)
        return specified.IsEmpty();
    return (stride==0);
}

DTIntArray DTIndex::IndicesAsArray(void) const
{
    // Will create it if necessary.
    if (arraySpecified)
        return specified;

    if (stride==0) {
        return DTIntArray();
    }

    ssize_t howMany;
    howMany = (end-start)/stride+1;
    ssize_t endAt = start + stride*howMany;
    ssize_t pos,i = 0;
    DTMutableIntArray toReturn(howMany);
    for (pos=start;pos!=endAt;pos+=stride) {
        toReturn(i++) = int(pos);
    }
    
    return toReturn;
}

ssize_t DTIndex::Start(void) const
{
    return start;
}

ssize_t DTIndex::Stride(void) const
{
    return stride;
}

ssize_t DTIndex::End(void) const
{
    return end;
}

DTIndex operator-(const DTIndex &A,int b)
{
    if (A.ArraySpecified()) {
        // shift each index
        return DTIndex(A.SpecifiedArray()-b);
    }
    else {
        // start,end,stride
        return DTIndex(A.Start()-b,A.Stride(),A.End()-b);
    }
}

DTIndex operator+(const DTIndex &A,int b)
{
    if (A.ArraySpecified()) {
        // shift each index
        return DTIndex(A.SpecifiedArray()+b);
    }
    else {
        // start,end,stride
        return DTIndex(A.Start()+b,A.Stride(),A.End()+b);
    }
}

