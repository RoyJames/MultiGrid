// Part of DTSource. Copyright 2004-2017. David Adalsteinsson.
// see http://www.visualdatatools.com/DTSource/license.html for more information.

#include "DTProgress.h"

#include <math.h>
#include <iostream>

DTProgress::DTProgress()
{
    file = DTFile("DTProgress",DTFile::NewReadWrite);
    currentLength = 0;
}

void DTProgress::pinfo(void) const
{
    std::cerr << currentLength << "%" << std::flush;
}

void DTProgress::UpdatePercentage(float percent)
{
    if (currentLength<int(floor(percent*100.0+0.5))) {
        // Add a few x's to the file.
        int i = currentLength;
        currentLength = int(floor(percent*100.0+0.5));
        FILE *theFile = file.FILEForWriting();
        if (theFile) {
            while (theFile && i<currentLength) {
                fprintf(theFile,"x");
                i++;
            }
            file.Flush();
        }
    }
}

