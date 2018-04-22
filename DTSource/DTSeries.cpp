// Part of DTSource. Copyright 2004-2017. David Adalsteinsson.
// see http://www.visualdatatools.com/DTSource/license.html for more information.

// See DTSeries.h for further information.
#include "DTSeries.h"

#include "DTError.h"
#include "DTUtilities.h"
#include "DTDataFile.h"
#include "DTMatlabDataFile.h"


DTSeriesStorage::~DTSeriesStorage()
{
    // Save the total bounding box, if it has been accumulated.
    if (bbox2D.isSet)
        Write(*theDataFile,"SeqInfo_"+name+"_BBox",bbox2D);
    else if (bbox3D.isSet)
        Write(*theDataFile,"SeqInfo_"+name+"_BBox",bbox3D);
    if (valueRange.isSet)
        Write(*theDataFile,"SeqInfo_"+name+"_VRange",valueRange);
    
    if (howManySaved>2 && !readOnly)
        SaveTimeValues();
}

void DTSeriesStorage::SaveTimeValues()
{
    combinedTimeValues = TruncateSize(combinedTimeValues,howManySaved);
    theDataFile->Save(combinedTimeValues,"SeqInfo_"+name+"_TimeValues");    
}

DTSeriesStorage::DTSeriesStorage()
{
    howManySaved = 0;
    lastTimeValue = -1.0;
    saveEachTime = false;
    readOnly = true;
    valid = false;
    onlyOneTimeValue = false;
}

DTSeries::DTSeries(string tp) : theData(new DTSeriesStorage())
{
    theData->type = tp;
}

DTSeries::DTSeries(DTDataStorage &saveInto,string nm,string tp)
: theData(new DTSeriesStorage())
{
    theData->theDataFile = saveInto.AsPointer();
    theData->readOnly = saveInto.IsReadOnly();
    theData->name = nm;
    theData->type = tp;
    theData->valid = true;
    theData->saveEachTime = true;

    DTMutableDoubleArray combinedTimeValues;
    
    // If the variable exists already, set this series up to allow appending and
    // retrieval of entries from file.
	string theType;
	string entryName;
	bool readTimeValue = true;
		
    if (saveInto.Contains("Seq_"+Name()) || saveInto.Contains("SeqInfo_"+Name())) {
        if (saveInto.Contains("SeqInfo_"+Name()))
            theType = saveInto.ReadString("SeqInfo_"+Name());
        else
            theType = saveInto.ReadString("Seq_"+Name());
        if (theType!=tp) {
            std::string fcn = string("DTSeries") + Type() + "(file,name)";
            DTErrorMessage(fcn,"The file already contains a variable with the same name but a different type.");
            theData->valid = false;
            return;
        }

        // Read in the time values that exist.
        if (saveInto.Contains("SeqInfo_"+Name()+"_TimeValues")) {
            DTDoubleArray theTimes = saveInto.ReadDoubleArray("SeqInfo_"+Name()+"_TimeValues");
            combinedTimeValues = theTimes.Copy();
            theData->howManySaved = int(combinedTimeValues.Length());
        }
        else {
            // See how many exist, and read the time values.
            combinedTimeValues = DTMutableDoubleArray(10);
            while (1) {
                entryName = nm + "_" + DTInt2String(HowManySaved());
                if (!saveInto.Contains(entryName))
                    break;

                if (HowManySaved()==combinedTimeValues.Length())
                    combinedTimeValues = IncreaseSize(combinedTimeValues,HowManySaved());
                if (readTimeValue) {
                    // Keep on reading in time values until it fails.
                    entryName = entryName + "_time";
                    if ((readTimeValue = saveInto.Contains(entryName)))
                        combinedTimeValues(theData->howManySaved++) = saveInto.ReadNumber(entryName);
                }
                if (readTimeValue==false) {
                    // If a time is not saved, use a uniform stride from there on.
                    double dt = 1.0;
                    if (HowManySaved()>1)
                        dt = combinedTimeValues(HowManySaved()-1)-combinedTimeValues(HowManySaved()-2);
                    if (HowManySaved()>0)
                        combinedTimeValues(HowManySaved()) = combinedTimeValues(HowManySaved()-1)+dt;
                    else
                        combinedTimeValues(HowManySaved()) = 0.0;
                    theData->howManySaved++;
                }
            }
            if (HowManySaved()==0) {
                if (saveInto.Contains(nm)) {
                    // Saved as a single number, not a time series.  Can't add to it.
                    theData->howManySaved = 1;
                    combinedTimeValues = DTMutableDoubleArray(1);
                    combinedTimeValues = 0.0;
                    theData->onlyOneTimeValue = true;
                }
            }
            else {
                theData->lastTimeValue = combinedTimeValues(HowManySaved()-1);
            }
        }
    }
	else if (saveInto.Contains(Name()) && saveInto.ReadString(Name())=="Series") {
		// Might be part of a group inside the data file.
		theType = saveInto.ReadString(Name()+"_type");
		if (theType!=tp) {
            std::string fcn = string("DTSeries") + Type() + "(file,name)";
            DTErrorMessage(fcn,"The file already contains a variable with the same name but a different type.");
            theData->valid = false;
            return;
        }
		
		// See how many exist, and read the time values.
		combinedTimeValues = DTMutableDoubleArray(10);
		while (1) {
			entryName = nm + "_" + DTInt2String(HowManySaved());
			if (!saveInto.Contains(entryName))
				break;
			
			if (HowManySaved()==combinedTimeValues.Length())
				combinedTimeValues = IncreaseSize(combinedTimeValues,HowManySaved());
			if (readTimeValue) {
				// Keep on reading in time values until it fails.
				entryName = entryName + "_time";
				if ((readTimeValue = saveInto.Contains(entryName)))
					combinedTimeValues(theData->howManySaved++) = saveInto.ReadNumber(entryName);
			}
			if (readTimeValue==false) {
				// If a time is not saved, use a uniform stride from there on.
				double dt = 1.0;
				if (HowManySaved()>1)
					dt = combinedTimeValues(HowManySaved()-1)-combinedTimeValues(HowManySaved()-2);
				if (HowManySaved()>0)
					combinedTimeValues(HowManySaved()) = combinedTimeValues(HowManySaved()-1)+dt;
				else
					combinedTimeValues(HowManySaved()) = 0.0;
				theData->howManySaved++;
			}
		}
		if (HowManySaved()==0) {
			if (saveInto.Contains(nm)) {
				// Saved as a single number, not a time series.  Can't add to it.
				theData->howManySaved = 1;
				combinedTimeValues = DTMutableDoubleArray(1);
				combinedTimeValues = 0.0;
				theData->onlyOneTimeValue = true;
			}
		}
		else {
			theData->lastTimeValue = combinedTimeValues(HowManySaved()-1);
		}
	}
    else if (theData->readOnly) {
        // set as read only, but no entry with this name.
        std::string fcn = string("DTSeries") + Type() + "(file,name)";
        std::string msg = string("The file doesn't contain a series with the name \"") + Name() + "\"";
		DTErrorMessage(fcn,msg);
        theData->valid = false;
        return;
    }
    else {
        // Add a series type desriptor.
        theData->theDataFile->Save(Type(),"Seq_"+Name());
    }
    
    theData->combinedTimeValues = combinedTimeValues;
}

DTSeries::~DTSeries()
{
}

void DTSeries::pinfo(void) const
{
    if (theData->valid) {
        std::cerr << "Type = " << Type() << ", Name = " << Name();
        if (HowManySaved()==0)
            std::cerr << ", no entries.\n";
        else if (HowManySaved()==1)
            std::cerr << ", one value.\n";
        else
            std::cerr << ", " << HowManySaved() << " time values.\n" << flush;
    }
    else {
        std::cerr << "Invalid, no entries.  Type = " << Type() << "\n" << flush;
    }
}

bool DTSeries::TimeConsideredTheSame(double t1,double t2)
{
    // Time value should be considered the same if they are too close.
    // This is to handle correctly the case where the time values should
    // be the same but they were either computed with a slightly different
    // order of operation (and therefore rounded differently) or one was
    // saved as a float the other as a double.
    return (fabs(t1-t2)<=0.0000001*(t1+t2));
}

void DTSeries::SetSaveEachTime(bool yOrN)
{
    if (HowManySaved()>0 && theData->saveEachTime!=yOrN) {
        DTErrorMessage("DTSeries::SetSaveEachTime","Can't switch in the middle.");
        return;
    }
    
    theData->saveEachTime = yOrN;
}

bool DTSeries::SharedSave(double time)
{
    // Might have been read as input with only one value (no time series).
    if (theData->valid==false) {
        std::string fcn = string("DTSeries") + Type();
        if (theData->theDataFile.Data()==NULL) {
            DTErrorMessage(fcn,"Series is invalid, no file specified.");
        }
        else if (Name().length()==0) {
            DTErrorMessage(fcn,"Series is invalid, the name is not specified.");
        }
        else {
            DTErrorMessage(fcn,string("Series is invalid - name = \"")+Name()+"\".");
        }
        return false;
    }
    if (theData->onlyOneTimeValue) {
        std::string fcn = string("DTSeries") + Type();
        DTErrorMessage(fcn,string("Can not add a time value to the variable \"")+Name()+"\".");
        return false;
    }
    
    // Time needs to be increasing.
    if (isfinite(time)==0 || time<0.0 || time<=theData->lastTimeValue) {
        std::string fcn = string("DTSeries") + Type();
        if (isfinite(time)==0) {
            DTErrorMessage(fcn,"Invalid time value.  Needs to be finite.");
        }
        else if (time>=0.0) {
            DTErrorMessage(fcn,"Invalid time value.  Needs to be strictly increasing.");
        }
        else {
            DTErrorMessage(fcn,"Invalid time value.  Can't be negative.");
        }
        return false;
    }

    // DataTank needs to be able to distinguish this time value from the previous one.
    if (theData->lastTimeValue>=0 && TimeConsideredTheSame(theData->lastTimeValue,time)) {
        std::string fcn = string("DTSeries") + Type();
        DTErrorMessage(fcn,"Time values are too close together.");
        return false;
    }

    // Add this to the combined time values.
    if (HowManySaved()==theData->combinedTimeValues.Length()) {
        // Increase the size of the time array.
        if (theData->combinedTimeValues.IsEmpty())
            theData->combinedTimeValues = DTMutableDoubleArray(10);
        else
            theData->combinedTimeValues = IncreaseSize(theData->combinedTimeValues,HowManySaved());
    }
    theData->combinedTimeValues(HowManySaved()) = time;
    
    // Save the time
    if (theData->saveEachTime)
        DataFile().Save(time,Name() + "_" + DTInt2String(HowManySaved()) + "_time");
    theData->howManySaved++;
    theData->lastTimeValue = time;
    
    return true;
}

void DTSeries::AddToValueRange(DTRegion1D vr)
{
    theData->valueRange = Union(theData->valueRange,vr);
}

void DTSeries::AddToBBox(DTRegion2D b)
{
    theData->bbox2D = Union(theData->bbox2D,b);
}

void DTSeries::AddToBBox(DTRegion3D b)
{
    theData->bbox3D = Union(theData->bbox3D,b);
}

void DTSeries::SaveTimeValues(void)
{
    theData->SaveTimeValues();
}

string DTSeries::baseName() const
{
    return Name() + "_" + DTInt2String(HowManySaved()-1);
}

string DTSeries::baseName(int k) const
{
    return Name() + "_" + DTInt2String(k);
}

DTDoubleArray DTSeries::TimeValues(void) const
{
    return TruncateSize(theData->combinedTimeValues,HowManySaved());
}

bool DTSeries::ChangesBetween(double t1,double t2) const
{
    return (IndexForTime(t1)!=IndexForTime(t2));
}

int DTSeries::IndexForTime(double time) const
{
    // Find where the index k in the list such that
    // combinedTimeValues(k) <= t < combinedTimeValues(k+1)
    // This is the same method as used in DataTank.

    if (time<0.0) {
        DTErrorMessage("DTSeries::Get","Time has to be >=0.");
        return -1;
    }
    if (HowManySaved()==0) {
        return -1; // Return an empty array.
    }
    
    int kGreaterOrEqual = 0;
    int kStrictlyLessThan = HowManySaved();
    int kCheck;
    
    DTDoubleArray combinedTimeValues = theData->combinedTimeValues;
    if (time<combinedTimeValues(0)) {
        return -1;
    }
    while (kStrictlyLessThan-kGreaterOrEqual>1) {
        kCheck = (kStrictlyLessThan+kGreaterOrEqual)/2;
        if (time<combinedTimeValues(kCheck))
            kStrictlyLessThan = kCheck;
        else
            kGreaterOrEqual = kCheck;
    }

    // At this point k==kGreaterOrEqual, but it's possible that
    // combinedTimeValues(kStrictlyLessThan)==time if we consider round-off
    // even though it is slightly less in exact floating point representation.
    if (kStrictlyLessThan<HowManySaved() &&
        TimeConsideredTheSame(combinedTimeValues(kStrictlyLessThan),combinedTimeValues(kGreaterOrEqual))) {
        return kGreaterOrEqual+1;
    }
    else {
        return kGreaterOrEqual;
    }
}

double DTSeries::TimeNumber(int timeN) const
{
    return theData->combinedTimeValues(timeN);
}

DTMutablePointer<DTDataStorage> DTSeries::ReferencedDataFile(const DTDataStorage &storage,string name,string &variableName)
{
    if (storage.Contains(name+"_file")) {
        variableName = storage.ReadString(name);
        std::string fileName = storage.ReadString(name+"_file");
        if (fileName.length() >= 4) {
            if (fileName.compare (fileName.length() - 4, 5, ".mat")==0) {
                DTMatlabDataFile fromFile(fileName,DTFile::ReadOnly);
                return fromFile.AsPointer();
            }
            else {
                DTDataFile fromFile(fileName,DTFile::ReadOnly);
                return fromFile.AsPointer();
            }
        }
        else {
            DTDataFile fromFile(fileName,DTFile::ReadOnly);
            return fromFile.AsPointer();
        }
    }
    else {
        variableName = name;
        return storage.AsPointer();
    }
}

