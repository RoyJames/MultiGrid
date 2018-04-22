// Part of DTSource. Copyright 2004-2017. David Adalsteinsson.
// see http://www.visualdatatools.com/DTSource/license.html for more information.

// Base class for a time series of a variable.

// For a usage example, consider the class DTSeriesMesh2D
//
// Create it with

#ifndef DTSeries_Header
#define DTSeries_Header

#include "DTDataStorage.h"
#include "DTDoubleArray.h"
#include "DTRegion1D.h"
#include "DTRegion2D.h"
#include "DTRegion3D.h"
#include "DTPointer.h"

struct DTSeriesStorage
{
    DTSeriesStorage();
    ~DTSeriesStorage();
    void SaveTimeValues(void);
    
    std::string name;
    std::string type;
    int howManySaved;
    double lastTimeValue;
    bool saveEachTime;
    
    bool readOnly;
    bool valid;
    bool onlyOneTimeValue;
    
    // Bounding boxes that gets accumulated
    DTRegion1D valueRange;
    DTRegion2D bbox2D;
    DTRegion3D bbox3D;
    
    DTMutablePointer<DTDataStorage> theDataFile;
    
    DTMutableDoubleArray combinedTimeValues;
};

class DTSeries {
public:
    virtual ~DTSeries();
    std::string baseName() const; // Call only after a valid SharedSave call.
    std::string baseName(int k) const;
    std::string Name(void) const {return theData->name;}
    std::string Type(void) const {return theData->type;}
    
    bool IsValid(void) const {return theData->valid;}
    
    void SetSaveEachTime(bool); // Default is true.  Otherwise it waits until the end to save a time list.
    int HowManySaved(void) const {return theData->howManySaved;}
    DTDoubleArray TimeValues(void) const;
    double TimeNumber(int) const;
    double LastTimeValue(void) const {return theData->lastTimeValue;}

    static bool TimeConsideredTheSame(double t1,double t2);
    bool ChangesBetween(double t1,double t2) const;
    
    void pinfo(void) const;
    
    // Used by the Read functions.
    static DTMutablePointer<DTDataStorage> ReferencedDataFile(const DTDataStorage &,string,string &variableName);
    
protected:
    DTSeries(string type);
    DTSeries(DTDataStorage &saveInto,string name,string type);
    
    void SaveTimeValues(void);
    bool SharedSave(double time);
    void AddToValueRange(DTRegion1D);
    void AddToBBox(DTRegion2D);
    void AddToBBox(DTRegion3D);

    int IndexForTime(double time) const;
    
    DTDataStorage &DataFile() {return *(theData->theDataFile);}
    const DTDataStorage &DataFile() const {return *(theData->theDataFile);}

private:
    DTMutablePointer<DTSeriesStorage> theData;
};

#endif

