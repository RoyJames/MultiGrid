// Part of DTSource. Copyright 2004-2017. David Adalsteinsson.
// see http://www.visualdatatools.com/DTSource/license.html for more information.

#ifndef DTUCharArrayRegion_Header
#define DTUCharArrayRegion_Header

// Class to support array regions.  You don't explicitly create this class,
// it only exists as a temporary object. 

#include "DTUCharArray.h"
#include "DTIndex.h"

class DTUCharArrayRegion {
public:
    DTUCharArrayRegion(DTUCharArrayStorage *,DTIndex index);
    DTUCharArrayRegion(DTUCharArrayStorage *,DTIndex indexX,DTIndex indexY);
    DTUCharArrayRegion(DTUCharArrayStorage *,DTIndex indexX,DTIndex indexY,DTIndex indexZ);
    DTUCharArrayRegion(const DTUCharArrayRegion &C) : storage(C.storage), I(C.I), J(C.J), K(C.K) {}
    
    operator DTUCharArray() const;                             // Array = region
    operator DTMutableUCharArray() const;                      // Mutable array = region
    DTUCharArrayRegion &operator=(const DTUCharArray &);       // Region = Array
    DTUCharArrayRegion &operator=(unsigned char);              // Region = value
    DTUCharArrayRegion &operator=(const DTUCharArrayRegion &); // Region = Region
    
private:
    DTUCharArrayRegion();
    
    DTMutableUCharArray ConvertToArray() const;
    
    DTUCharArrayStorage *storage;
    DTIndex I,J,K;
};

#endif
