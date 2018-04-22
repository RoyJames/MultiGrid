// Part of DTSource. Copyright 2004-2017. David Adalsteinsson.
// see http://www.visualdatatools.com/DTSource/license.html for more information.

#include "DTPoint2D.h"

#include "DTDoubleArray.h"
#include "DTError.h"

void DTPoint2D::pinfo(void) const
{
    std::cerr << "(" << x << ", " << y << ")\n" << std::flush;
}

void Read(const DTDataStorage &input,const std::string &name,DTPoint2D &toReturn)
{
    DTDoubleArray theArr = input.ReadDoubleArray(name);
    if (theArr.Length()!=2) {
        DTErrorMessage("ReadFromArray(DTPoint2D)","Invalid length of array.");
        toReturn = DTPoint2D();
    }
    else {
        toReturn = DTPoint2D(theArr(0),theArr(1));
    }
}

void Write(DTDataStorage &output,const std::string &name,const DTPoint2D &theVar)
{
    DTMutableDoubleArray theArr(2);
    theArr(0) = theVar.x;
    theArr(1) = theVar.y;
    output.Save(theArr,name);
}

void WriteOne(DTDataStorage &output,const std::string &name,const DTPoint2D &toWrite)
{
    Write(output,name,toWrite);
    Write(output,"Seq_"+name,"2D Point");
    output.Flush();
}

