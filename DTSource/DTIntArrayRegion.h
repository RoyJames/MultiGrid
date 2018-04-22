// Part of DTSource. Copyright 2004-2017. David Adalsteinsson.
// see http://www.visualdatatools.com/DTSource/license.html for more information.

#ifndef DTIntArrayRegion_Header
#define DTIntArrayRegion_Header

// Class to support array regions.  You don't explicitly create this class,
// it only exists as a temporary object. 

#include "DTIntArray.h"
#include "DTIndex.h"

class DTIntArrayRegion {
public:
    DTIntArrayRegion(DTIntArrayStorage *,DTIndex index);
    DTIntArrayRegion(DTIntArrayStorage *,DTIndex indexX,DTIndex indexY);
    DTIntArrayRegion(DTIntArrayStorage *,DTIndex indexX,DTIndex indexY,DTIndex indexZ);
    DTIntArrayRegion(const DTIntArrayRegion &C) : storage(C.storage), I(C.I), J(C.J), K(C.K) {}
    
    operator DTIntArray() const;                           // Array = region
    operator DTMutableIntArray() const;                    // Mutable array = region
    DTIntArrayRegion &operator=(const DTIntArray &);       // Region = Array
    DTIntArrayRegion &operator=(int);                      // Region = value
    DTIntArrayRegion &operator=(const DTIntArrayRegion &); // Region = Region
    
private:
    DTIntArrayRegion();
    
    DTMutableIntArray ConvertToArray() const;
    
    DTIntArrayStorage *storage;
    DTIndex I,J,K;
};

#endif
