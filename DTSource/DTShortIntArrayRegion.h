// Part of DTSource. Copyright 2004-2017. David Adalsteinsson.
// see http://www.visualdatatools.com/DTSource/license.html for more information.

#ifndef DTShortIntArrayRegion_Header
#define DTShortIntArrayRegion_Header

// Class to support array regions.  You don't explicitly create this class,
// it only exists as a temporary object. 

#include "DTShortIntArray.h"
#include "DTIndex.h"

class DTShortIntArrayRegion {
public:
    DTShortIntArrayRegion(DTShortIntArrayStorage *,DTIndex index);
    DTShortIntArrayRegion(DTShortIntArrayStorage *,DTIndex indexX,DTIndex indexY);
    DTShortIntArrayRegion(DTShortIntArrayStorage *,DTIndex indexX,DTIndex indexY,DTIndex indexZ);
    DTShortIntArrayRegion(const DTShortIntArrayRegion &C) : storage(C.storage), I(C.I), J(C.J), K(C.K) {}
    
    operator DTShortIntArray() const;                                // Array = region
    operator DTMutableShortIntArray() const;                         // Mutable array = region
    DTShortIntArrayRegion &operator=(const DTShortIntArray &);       // Region = Array
    DTShortIntArrayRegion &operator=(short int);                     // Region = value
    DTShortIntArrayRegion &operator=(const DTShortIntArrayRegion &); // Region = Region
    
private:
    DTShortIntArrayRegion();
    
    DTMutableShortIntArray ConvertToArray() const;
    
    DTShortIntArrayStorage *storage;
    DTIndex I,J,K;
};

#endif
