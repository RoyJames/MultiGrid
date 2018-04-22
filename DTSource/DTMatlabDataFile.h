// Part of DTSource. Copyright 2004-2017. David Adalsteinsson.
// see http://www.visualdatatools.com/DTSource/license.html for more information.

#ifndef DTMatlabDataFile_Header
#define DTMatlabDataFile_Header

// A class that can take care of saving arrays in a format that matlab can read.
// use "help load" and "help save" in matlab to get more information.
// On a mac (and maybe also on a pc) you can type
//    load
// in the command window, and drag and drop the file into the window to get
// the full name of the file (including path information).

// Usage is
// Create variables by specifying the file name when they are created.
//
// DTMatlabDataFile TheFile("FileNameOnDisk");
//
// If A is an array (DTDoubleArray or DTFloatArray), to save it in the file
// under the name "ArrayName", use
//     TheFile.Save(A,"ArrayName");
// to read it from the file
//     = TheFile.ReadDoubleArray("ArrayName");
// similarly for other variable types.

// If you are reading or writing variables with names test_1, test_2, ...
// name = "test+" + DTInt2String(3);

#include <stdio.h>
#include <fstream>
#include <map>

#include "DTDataStorage.h"
#include "DTFile.h"

struct DTMatlabDataEntry;
class DTMatlabDataFileContent;

class DTMatlabDataFile : public DTDataStorage {
public:
    DTMutablePointer<DTDataStorage> AsPointer() const;

    DTMatlabDataFile();
    DTMatlabDataFile(DTFile file);
    // DTFile::ReadOnly, DTFile::ExistingReadWrite, DTFile::NewReadWrite
    DTMatlabDataFile(const std::string &name,DTFile::OpenType=DTFile::ExistingReadWrite);

    // Copying and assignment treat this class as a pointer.
    DTMatlabDataFile(const DTMatlabDataFile &);
    DTMatlabDataFile &operator=(const DTMatlabDataFile &);
    ~DTMatlabDataFile();

    bool Contains(const std::string &name) const;
    DTList<std::string> AllVariableNames(void) const;
    bool IsReadOnly(void) const;
    bool IsOpen(void) const;
    void SaveIndex(void);
    void ShouldSaveIndexWhenClosing(void);
    
    // Saving data.
    void Save(int,const std::string &name);
    void Save(double,const std::string &name);
    void Save(const DTDoubleArray &A,const std::string &name);
    void Save(const DTFloatArray &A,const std::string &name);
    void Save(const DTIntArray &A,const std::string &name);
    void Save(const DTCharArray &A,const std::string &name);
    void Save(const DTUCharArray &A,const std::string &name);
    void Save(const DTShortIntArray &A,const std::string &name);
    void Save(const DTUShortIntArray &A,const std::string &name);
    void Save(const std::string &theString,const std::string &name);

    bool SavedAsCharacter(const std::string &name) const;
    bool SavedAsShort(const std::string &name) const;
    bool SavedAsInt(const std::string &name) const;
    bool SavedAsDouble(const std::string &name) const;
    bool SavedAsString(const std::string &name) const;

    void Flush(void) const;

    // Reading data.
    DTDoubleArray ReadDoubleArray(const std::string &name) const;
    DTFloatArray ReadFloatArray(const std::string &name) const;
    DTIntArray ReadIntArray(const std::string &name) const;
    DTCharArray ReadCharArray(const std::string &name) const;
    DTUCharArray ReadUCharArray(const std::string &name) const;
    DTShortIntArray ReadShortIntArray(const std::string &name) const;
    DTUShortIntArray ReadUShortIntArray(const std::string &name) const;
    double ReadNumber(const std::string &name) const;
    int ReadInt(const std::string &name) const;
    std::string ReadString(const std::string &name) const;
    
private:
    void printInfo(void) const;
    DTMatlabDataEntry FindVariable(const std::string &name) const;

    DTMatlabDataFileContent *content;
};

#endif
