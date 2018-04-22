// Part of DTSource. Copyright 2004-2017. David Adalsteinsson.
// see http://www.visualdatatools.com/DTSource/license.html for more information.

#include "DTSeriesMesh2D.h"

#include "DTMesh2D.h"
#include "DTSeriesTemplates.h"

map<string,DTSeriesMesh2D> DTGlobalSeriesMesh2D;

void DTSeriesMesh2D::Register() const
{
    Register(Name());
}

void DTSeriesMesh2D::Register(string name) const
{
    DTRegisteredSeries<DTSeriesMesh2D,map<string,DTSeriesMesh2D>::const_iterator>(DTGlobalSeriesMesh2D,name,"DTSeriesMesh2D",this);
}

void DTSeriesMesh2D::pregistered(void)
{
    DTPrintRegisteredSeries<DTSeriesMesh2D,map<string,DTSeriesMesh2D>::const_iterator>(DTGlobalSeriesMesh2D);
}

bool DTSeriesMesh2D::HasBeenRegistered(string name)
{
    return DTHasRegisteredSeries<DTSeriesMesh2D,map<string,DTSeriesMesh2D>::const_iterator>(DTGlobalSeriesMesh2D,name);
}

DTSeriesMesh2D DTSeriesMesh2D::ByName(string name)
{
    return DTFindRegisteredSeries<DTSeriesMesh2D,map<string,DTSeriesMesh2D>::const_iterator>(DTGlobalSeriesMesh2D,name,"DTSeriesMesh2D");
}

void DTSeriesMesh2D::Add(const DTDoubleArray &v,double time)
{
    if (SharedSave(time)==false) return;
    Write(DataFile(),baseName(),v);

    if (v.NotEmpty())
        AddToBBox(DTRegion2D(DTPoint2D(0,0),DTPoint2D(v.m()-1,v.n()-1)));
    AddToValueRange(ValueRange(v));
    DataFile().Flush();
}

void DTSeriesMesh2D::Add(const DTDoubleArray &v,const DTMesh2DGrid &grid,double time)
{
    if (SharedSave(time)==false) return;
    WriteNoSize(DataFile(),baseName()+"_loc",grid);
    Write(DataFile(),baseName(),v);

    AddToBBox(BoundingBox(grid));
    AddToValueRange(ValueRange(v));
    DataFile().Flush();
}

void DTSeriesMesh2D::Add(DTMesh2D v,double time)
{
    if (SharedSave(time)==false) return;
    Write(DataFile(),baseName(),v);

    AddToBBox(BoundingBox(v));
    AddToValueRange(ValueRange(v));
    DataFile().Flush();
}

DTMesh2D DTSeriesMesh2D::Get(double time) const
{
    int k = IndexForTime(time);
    if (k<0) return DTMesh2D();
    DTMesh2D toReturn;
    Read(DataFile(),baseName(k),toReturn);
    return toReturn;
}

void Read(const DTDataStorage &input,const std::string &name,DTSeriesMesh2D &toReturn)
{
    string variableName;
    DTMutablePointer<DTDataStorage> storage = DTSeries::ReferencedDataFile(input,name,variableName);
    toReturn = DTSeriesMesh2D(*storage,variableName);
}
