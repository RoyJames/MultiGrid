// Part of DTSource. Copyright 2004-2017. David Adalsteinsson.
// see http://www.visualdatatools.com/DTSource/license.html for more information.

#ifndef DTDoubleArrayRegion_Header
#define DTDoubleArrayRegion_Header

// Class to support array regions.  You don't explicitly create this class,
// it only exists as a temporary object. 

#include "DTDoubleArray.h"
#include "DTIndex.h"

class DTDoubleArrayRegion {
public:
    DTDoubleArrayRegion(DTDoubleArrayStorage *,DTIndex index);
    DTDoubleArrayRegion(DTDoubleArrayStorage *,DTIndex indexX,DTIndex indexY);
    DTDoubleArrayRegion(DTDoubleArrayStorage *,DTIndex indexX,DTIndex indexY,DTIndex indexZ);
    DTDoubleArrayRegion(const DTDoubleArrayRegion &C) : storage(C.storage), I(C.I), J(C.J), K(C.K) {}
    
    operator DTDoubleArray() const;                              // Array = region
    operator DTMutableDoubleArray() const;                       // Mutable array = region
    DTDoubleArrayRegion &operator=(const DTDoubleArray &);       // Region = Array
    DTDoubleArrayRegion &operator=(double);                      // Region = value
    DTDoubleArrayRegion &operator=(const DTDoubleArrayRegion &); // Region = Region
    
private:
    DTDoubleArrayRegion();

    DTMutableDoubleArray ConvertToArray() const;
    
    DTDoubleArrayStorage *storage;
    DTIndex I,J,K;
};

#endif
