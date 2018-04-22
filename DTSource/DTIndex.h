// Part of DTSource. Copyright 2004-2017. David Adalsteinsson.
// see http://www.visualdatatools.com/DTSource/license.html for more information.

#ifndef DTIndex_Header
#define DTIndex_Header

// Index class to support array subscripts.

// Usage:
// A(3,DTIndex::All) = 3;   - Overwrites row 3.
// A(DTIndex::Last,DTIndex(0,2,DTIndex::Last)) = 5; - Overwrites even columns in last row.


#include "DTIntArray.h"

class DTIndex {
public:
    DTIndex();
    DTIndex(ssize_t number);
    DTIndex(ssize_t start,ssize_t end);
    DTIndex(ssize_t start,ssize_t stride,ssize_t end);
    DTIndex(const DTIntArray &specified);

    static ssize_t Last;
    static DTIndex All;

    // Need to supply the length before calling start,stride,end.
    bool SetLength(ssize_t howLong); // Returns false if invalid.

    bool IsEverything(void) const {return (stride==1 && start==0 && end==knownLength-1);}
    ssize_t Length(void) const;
    
    bool IsEmpty(void) const;
    bool ArraySpecified(void) const {return arraySpecified;}
    DTIntArray IndicesAsArray(void) const; // Will create it if necessary.
    DTIntArray SpecifiedArray(void) const {return specified;}
    ssize_t Start(void) const;
    ssize_t Stride(void) const;
    ssize_t End(void) const;
    
private:
    bool lengthSet;
    ssize_t knownLength;
    ssize_t start,stride,end;
    bool arraySpecified;
    DTIntArray specified;
};

extern DTIndex operator-(const DTIndex &,int);
extern DTIndex operator+(const DTIndex &,int);

#endif
