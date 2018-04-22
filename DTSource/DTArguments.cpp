// Part of DTSource. Copyright 2004-2017. David Adalsteinsson.
// see https://www.visualdatatools.com/DTSource/license.html for more information.

#include "DTArguments.h"

#include "DTError.h"

#include <vector>
#include <algorithm>
#include <stdlib.h>

static std::vector<std::string> DTInputArgumentList;
static std::string DTApplicationName;

void DTSetArguments(int argc,const char *argv[])
{
    // Just copy the arguments into a list of strings.
    DTInputArgumentList.clear();

    DTApplicationName = argv[0];

    for (int i=1;i<argc;i++)
        DTInputArgumentList.push_back((std::string)(argv[i]));
}

bool DTArgumentIncludesFlag(std::string flagName)
{
    std::string checkFor = "-"+flagName;

    std::vector<std::string>::const_iterator result;
    result = find(DTInputArgumentList.begin(),
                  DTInputArgumentList.end(),
                  checkFor);

    return (result!=DTInputArgumentList.end());
}

std::string DTArgumentFlagEntry(std::string flagName)
{
    std::string checkFor = "-"+flagName;

    std::vector<std::string>::const_iterator result;
    result = find(DTInputArgumentList.begin(),
                  DTInputArgumentList.end(),
                  checkFor);

    if (result==DTInputArgumentList.end())
        return std::string();

    ++result;
    std::string toReturn = *result;
    if (toReturn.length()==0 || toReturn[0]=='-')
        return std::string();

    return toReturn;
}

int DTArgumentIntegerEntry(std::string flagName)
{
    std::string strVal = DTArgumentFlagEntry(flagName);
    return atoi(strVal.c_str());
}

int DTHowManyArguments(void)
{
    return int(DTInputArgumentList.size());
}

std::string DTGetArgumentNumber(int num)
{
    if (num<0 || num>=int(DTInputArgumentList.size())) {
        DTErrorMessage("DTGetArgumentNumber(number)","number out of bounds.");
        return std::string();
    }

    return DTInputArgumentList[num];
}

