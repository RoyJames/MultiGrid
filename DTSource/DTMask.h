// Part of DTSource. Copyright 2004-2017. David Adalsteinsson.
// see http://www.visualdatatools.com/DTSource/license.html for more information.

#ifndef DTMask_H
#define DTMask_H

// These masked are used inside the grid definition for 2D and 3D meshes and vector fields.
// This can be used to mask off a certain portion of the computational domain.

#include "DTIntArray.h"
#include "DTShortIntArray.h"
#include "DTCharArray.h"
#include "DTDataStorage.h"
#include "DTPointer.h"

struct DTRegion1D;

struct DTMaskOptionalData {
	DTMaskOptionalData();
	
	DTCharArray mask;
	DTIntArray offsets;
	
	bool intRangeComputed;
    ssize_t numberOfPoints;
	ssize_t minI,maxI,minJ,maxJ,minK,maxK;
};

class DTMask {
public:
    DTMask(const DTIntArray &arr,ssize_t mv,ssize_t nv,ssize_t ov=1); // 2xM array of intervals.
    DTMask(const DTCharArray &zerosAndOnes);
    
    DTMask();
    DTMask(ssize_t mV,ssize_t nV,ssize_t oV=1);

	bool IsEmpty(void) const {return _m==0;}
	bool NotEmpty(void) const {return _m!=0;}
    int m(void) const {return _m;}
    int n(void) const {return _n;}
    int o(void) const {return _o;}

    DTIntArray Intervals(void) const {return intervals;}
    ssize_t NumberOfPoints(void) const;
    
	// Optional data. Will be computed the first time, but then cached.
    DTCharArray MaskArray(void) const;
	DTIntArray Offsets(void) const;
	
	bool ContainsPointOnEdge(ssize_t) const; // Check if the mask stays away from an edge.

    // Debugging info.
    void pinfo(void) const;
	void pstars(void) const; // Works for 2D arrays, but make sure that you have enough columns.

    // Convert from offset in a band to offset in the full list
    ssize_t FromBandOffsetToBigOffset(int);
    int FromBigOffsetToBandOffset(ssize_t off);

private:
    DTIntArray intervals;
    int _m,_n,_o;
	
	DTPointer<DTMaskOptionalData> optional;
};

extern DTMask RemoveEdgeFromMask(const DTMask &,ssize_t); // Makes sure that the mask does not contain a boundary point.

extern DTMask operator-(const DTMask &,const DTMask &); // Set minus

extern bool operator==(const DTMask &,const DTMask &);
extern bool operator!=(const DTMask &,const DTMask &);

extern void Read(const DTDataStorage &input,const std::string &name,DTMask &toReturn);
extern void Write(DTDataStorage &output,const std::string &name,const DTMask &theVar);


class DTMask3D {
public:
    DTMask3D();
    DTMask3D(const DTIntArray &starts,const DTShortIntArray &intervals,ssize_t m,ssize_t n,ssize_t o);
    
    bool IsEmpty(void) const {return _m==0;}
    bool NotEmpty(void) const {return _m!=0;}
    ssize_t m(void) const {return _m;}
    ssize_t n(void) const {return _n;}
    ssize_t o(void) const {return _o;}
    ssize_t NumberOfPoints(void) const {return _numberOfPoints;}

    void pinfo(void) const;

    // The packing is as follows:
    // Starts has length n*o+1, for the line (:,j,k) the start of the intervals in that ray
    // is at Starts(j+k*n) and the end at Starts(j*k*n+1) (not included)
    const DTIntArray &Starts(void) const {return _starts;}
    const DTShortIntArray &Intervals(void) const {return _intervals;}
    const DTIntArray &StartOfData(void) const {return _startOfData;}
    
private:
    DTIntArray _starts; // Length is n*o+1.  For each line (:,j,k) j+k*n, where that portion starts in the intervals
    DTIntArray _startOfData; // for each j+k*n - where that portion starts in the value list.
    DTShortIntArray _intervals; // 2xN array (0,index) = start of interval index, (1,index) is the upper bound.  [a,b-1] between 0 and m.
    ssize_t _m,_n,_o;
    ssize_t _numberOfPoints;
    
    // So to look up the intervals
};

extern bool operator==(const DTMask3D &,const DTMask3D &);
extern bool operator!=(const DTMask3D &,const DTMask3D &);

extern void Read(const DTDataStorage &input,const std::string &name,DTMask3D &toReturn);
extern void Write(DTDataStorage &output,const std::string &name,const DTMask3D &theVar);

#endif
