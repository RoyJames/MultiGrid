// Part of DTSource. Copyright 2004-2017. David Adalsteinsson.
// see http://www.visualdatatools.com/DTSource/license.html for more information.

#ifndef DTCharArrayRegion_Header
#define DTCharArrayRegion_Header

// Class to support array regions.  You don't explicitly create this class,
// it only exists as a temporary object. 

#include "DTCharArray.h"
#include "DTIndex.h"

class DTCharArrayRegion {
public:
    DTCharArrayRegion(DTCharArrayStorage *,DTIndex index);
    DTCharArrayRegion(DTCharArrayStorage *,DTIndex indexX,DTIndex indexY);
    DTCharArrayRegion(DTCharArrayStorage *,DTIndex indexX,DTIndex indexY,DTIndex indexZ);
    DTCharArrayRegion(const DTCharArrayRegion &C) : storage(C.storage), I(C.I), J(C.J), K(C.K) {}
    
    operator DTCharArray() const;                            // Array = region
    operator DTMutableCharArray() const;                     // Mutable array = region
    DTCharArrayRegion &operator=(const DTCharArray &);       // Region = Array
    DTCharArrayRegion &operator=(char);                      // Region = value
    DTCharArrayRegion &operator=(const DTCharArrayRegion &); // Region = Region
    
private:
    DTCharArrayRegion();
    
    DTMutableCharArray ConvertToArray() const;
    
    DTCharArrayStorage *storage;
    DTIndex I,J,K;
};

#endif
