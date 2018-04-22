// Part of DTSource. Copyright 2004-2017. David Adalsteinsson.
// see http://www.visualdatatools.com/DTSource/license.html for more information.

#include "DTShiftScale2D.h"

#include "DTDoubleArray.h"
#include "DTError.h"

void DTShiftScale2D::pinfo(void) const
{
    std::cerr << "Shift = (" << shift.x << ", " << shift.y << "), scale = (" << scaleX << ", " << scaleY << ")\n" << std::flush;
}

DTPoint2D operator*(const DTShiftScale2D &s,const DTPoint2D &p)
{
	return s.Transform(p);
}

DTShiftScale2D ShiftBy(const DTPoint2D &p)
{
	return DTShiftScale2D(p,1,1);
}

void Read(const DTDataStorage &input,const std::string &name,DTShiftScale2D &toReturn)
{
    DTDoubleArray theArr = input.ReadDoubleArray(name);
    if (theArr.Length()!=4) {
        DTErrorMessage("ReadFromArray(DTShiftScale2D)","Invalid length of array.");
        toReturn = DTShiftScale2D();
    }
    else {
        toReturn = DTShiftScale2D(DTPoint2D(theArr(0),theArr(1)),theArr(2),theArr(3));
    }
}

void Write(DTDataStorage &output,const std::string &name,const DTShiftScale2D &theVar)
{
    DTPoint2D shift = theVar.Shift();
    
    DTMutableDoubleArray theArr(4);
    theArr(0) = shift.x;
    theArr(1) = shift.y;
    theArr(2) = theVar.ScaleX();
    theArr(3) = theVar.ScaleY();
    output.Save(theArr,name);
}

void WriteOne(DTDataStorage &output,const std::string &name,const DTShiftScale2D &toWrite)
{
    Write(output,name,toWrite);
    Write(output,"Seq_"+name,"2D Shift And Scale");
    output.Flush();
}

