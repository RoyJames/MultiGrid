// Part of DTSource. Copyright 2004-2017. David Adalsteinsson.
// see http://www.visualdatatools.com/DTSource/license.html for more information.

#include "DTSaveError.h"
#include "DTError.h"
#include "DTDataStorage.h"
#include "DTUCharArray.h"

#include "DTIntArray.h"
#include <cstring>

void DTSaveError(DTDataStorage &output,const std::string &name)
{
    vector<std::string> &list = DTErrorList();

    // Find total length
    int totalLength = 0;
    size_t i;
    size_t thisLength;
    for (i=0;i<list.size();i++) {
        thisLength = list[i].length();
        totalLength += thisLength+1;
    }

    // Combine the strings into a single character array.
    DTMutableUCharArray theCombinedStrings(totalLength);
    DTMutableIntArray offs((ssize_t)list.size());
    unsigned char *combP = theCombinedStrings.Pointer();
    int posInCombined = 0;
    std::string thisString;
    for (i=0;i<list.size();i++) {
        thisString = list[i];
        thisLength = list[i].length();
        std::memcpy(combP+posInCombined,thisString.c_str(),thisLength+1);
        offs((ssize_t)i) = posInCombined;
        posInCombined += thisLength+1;
    }

    output.Save(offs,name+"_offs");
    output.Save(theCombinedStrings,name);
}

