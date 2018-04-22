// Part of DTSource. Copyright 2004-2017. David Adalsteinsson.
// see http://www.visualdatatools.com/DTSource/license.html for more information.

#ifndef DTUShortIntArrayRegion_Header
#define DTUShortIntArrayRegion_Header

// Class to support array regions.  You don't explicitly create this class,
// it only exists as a temporary object. 

#include "DTUShortIntArray.h"
#include "DTIndex.h"

class DTUShortIntArrayRegion {
public:
    DTUShortIntArrayRegion(DTUShortIntArrayStorage *,DTIndex index);
    DTUShortIntArrayRegion(DTUShortIntArrayStorage *,DTIndex indexX,DTIndex indexY);
    DTUShortIntArrayRegion(DTUShortIntArrayStorage *,DTIndex indexX,DTIndex indexY,DTIndex indexZ);
    DTUShortIntArrayRegion(const DTUShortIntArrayRegion &C) : storage(C.storage), I(C.I), J(C.J), K(C.K) {}
    
    operator DTUShortIntArray() const;                                 // Array = region
    operator DTMutableUShortIntArray() const;                          // Mutable array = region
    DTUShortIntArrayRegion &operator=(const DTUShortIntArray &);       // Region = Array
    DTUShortIntArrayRegion &operator=(unsigned short int);             // Region = value
    DTUShortIntArrayRegion &operator=(const DTUShortIntArrayRegion &); // Region = Region
    
private:
    DTUShortIntArrayRegion();
    
    DTMutableUShortIntArray ConvertToArray() const;
    
    DTUShortIntArrayStorage *storage;
    DTIndex I,J,K;
};

#endif
