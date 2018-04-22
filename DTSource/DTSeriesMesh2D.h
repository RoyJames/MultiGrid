// Part of DTSource. Copyright 2004-2017. David Adalsteinsson.
// see http://www.visualdatatools.com/DTSource/license.html for more information.

#ifndef DTSeriesMesh2D_Header
#define DTSeriesMesh2D_Header

// See DTSeries.h for further information.
#include "DTSeries.h"

class DTMesh2D;
class DTMesh2DGrid;
class DTDoubleArray;

class DTSeriesMesh2D : public DTSeries
{
public:
    DTSeriesMesh2D() : DTSeries("Mesh2D") {}
    DTSeriesMesh2D(DTDataStorage &saveInto,string name) : DTSeries(saveInto,name,"Mesh2D") {}

    void Register() const;
    void Register(string) const;
    static void pregistered(void); // Debug call, lists all registered series of this type.
    static bool HasBeenRegistered(string);
    static DTSeriesMesh2D ByName(string);
    
    void Add(const DTDoubleArray &v,double time);
    void Add(const DTDoubleArray &v,const DTMesh2DGrid &grid,double time);
    void Add(DTMesh2D v,double time);

    DTMesh2D Get(double time) const;

private:
};

// To read series by reference. 
extern void Read(const DTDataStorage &input,const std::string &name,DTSeriesMesh2D &toReturn);

#endif

