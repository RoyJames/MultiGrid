// Part of DTSource. Copyright 2004-2015. David A. David Adalsteinsson.
// see http://www.visualdatatools.com/DTSource/license.html for more information.

#ifndef DTProgress_H
#define DTProgress_H

#include "DTFile.h"

class DTProgress {
public:
    DTProgress();

    void UpdatePercentage(float percent);
    void pinfo(void) const;
               
private:
    // Can't copy this object.  Pass it in by reference.
    DTProgress(const DTProgress &);
    void operator=(const DTProgress &);
    
    DTFile file;
    int currentLength;
};

#endif

