// Part of DTSource. Copyright 2004-2017. David Adalsteinsson.
// see http://www.visualdatatools.com/DTSource/license.html for more information.

#ifndef DTFloatArrayRegion_Header
#define DTFloatArrayRegion_Header

// Class to support array regions.  You don't explicitly create this class,
// it only exists as a temporary object. 

#include "DTFloatArray.h"
#include "DTIndex.h"

class DTFloatArrayRegion {
public:
    DTFloatArrayRegion(DTFloatArrayStorage *,DTIndex index);
    DTFloatArrayRegion(DTFloatArrayStorage *,DTIndex indexX,DTIndex indexY);
    DTFloatArrayRegion(DTFloatArrayStorage *,DTIndex indexX,DTIndex indexY,DTIndex indexZ);
    DTFloatArrayRegion(const DTFloatArrayRegion &C) : storage(C.storage), I(C.I), J(C.J), K(C.K) {}
    
    operator DTFloatArray() const;                             // Array = region
    operator DTMutableFloatArray() const;                      // Mutable array = region
    DTFloatArrayRegion &operator=(const DTFloatArray &);       // Region = Array
    DTFloatArrayRegion &operator=(float);                      // Region = value
    DTFloatArrayRegion &operator=(const DTFloatArrayRegion &); // Region = Region
    
private:
    DTFloatArrayRegion();
    
    DTMutableFloatArray ConvertToArray() const;
    
    DTFloatArrayStorage *storage;
    DTIndex I,J,K;
};

#endif
