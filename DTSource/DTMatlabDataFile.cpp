// Part of DTSource. Copyright 2004-2017. David Adalsteinsson.
// see http://www.visualdatatools.com/DTSource/license.html for more information.

#include "DTMatlabDataFile.h"

#include "DTDoubleArray.h"
#include "DTIntArray.h"
#include "DTUCharArray.h"
#include "DTCharArray.h"
#include "DTFloatArray.h"
#include "DTShortIntArray.h"
#include "DTUShortIntArray.h"
#include "DTUtilities.h"
#include "DTArrayConversion.h"

#include "DTEndianSwap.h"

#include "DTError.h"
#include <string.h>
#include <algorithm>

// The format is specified in the External Interface Guide that comes with Matlab 4.0

// This class does very few checks.
// It won't check if the file opened successfully.
// It won't check to see that you specified a valid file name.

struct FMatrix {
    FMatrix() : type(0), mrows(0), ncols(0), imagf(0), namelen(0) {}
    int type;
    int mrows;
    int ncols;
    int imagf;
    int namelen;
};

struct DTMatlabDataEntry {
    DTMatlabDataEntry() : m(0), n(0), type(0), location(-1) {}
    
    int m,n,type;
    off_t location; // -1 if not found.

    // FMatrix Fmtrx;
};

class DTMatlabDataFileContent
{
public:
    DTMatlabDataFileContent(DTFile file);
    ~DTMatlabDataFileContent();
    
    void ReadInContent();
    
    bool needToSwapEndian;
    int referenceCount;
    
    // Index file is not saved after each addition, typically you save this just before you close the file.
    bool saveIndex;
    bool saveIndexWhenClosing;
    DTFile indexFile;
    
    map<string,DTMatlabDataEntry> content;
    
    DTFile file;
    
    void SaveIndex(void);
    
private:
    DTMatlabDataFileContent(const DTMatlabDataFileContent &);
    DTMatlabDataFileContent &operator=(const DTMatlabDataFileContent &);
};

DTMatlabDataFileContent::~DTMatlabDataFileContent()
{
    if (saveIndexWhenClosing) {
        SaveIndex();
    }
}

DTMatlabDataFileContent::DTMatlabDataFileContent(DTFile f)
: needToSwapEndian(false), referenceCount(1), file(f)
{
    // Read in the content of the file, so subsequent access is much faster.
    ReadInContent();
}

void DTMatlabDataFileContent::ReadInContent(void)
{
    FMatrix TheMatrix;
    DTMatlabDataEntry singleEntry;

    if (!file.IsOpen())
        return;

    off_t EndsAt = file.Length();
    
    // See if there is an index file saved.
    std::string indexName;
    std::string thisFile = file.Name();
    if (thisFile.length()>4 && thisFile.substr(thisFile.length()-4,4)==".mat") {
        indexName = thisFile.substr(0,thisFile.length()-4)+".index";
    }
    else {
        indexName = thisFile+".index";
    }
    if (DTFile::CanOpen(indexName,DTFile::ReadOnly)) {
        // Read in the entire index file
        
        // Read in the portion of the file that is valid.  The index file might
        // include entries that haven't been completely saved in the data file, and
        // those entries should be quietly skipped.
        DTFile tempIndex(indexName,DTFile::ReadOnly);
        size_t indexLength = tempIndex.Length();
        
        const char *shouldBe = "mat index file";
        if (indexLength<strlen(shouldBe)+1) {
            // Too small
            indexLength = 0;
        }
        
        FILE *indexFilePointer = tempIndex.FILEForReading();
        if (indexFilePointer==NULL) indexLength = 0;
        
        DTMutableCharArray bufferArray(indexLength ? indexLength+1 : 0);
        
        char *bufferP = bufferArray.Pointer();
        if (indexLength && bufferP) {
            bufferP[indexLength] = 0;
            if (fread(bufferP,1,indexLength,indexFilePointer)!=indexLength) {
                indexLength = 0;
                bufferArray = DTMutableCharArray();
                bufferP = 0;
            }
        }
        
        // Check if this is the right type
        if (bufferP!=NULL && memcmp(bufferP, shouldBe,strlen(shouldBe)+1)!=0) {
            bufferArray = DTMutableCharArray();
            bufferP = 0;
        }
        
        if (bufferP) {
            ssize_t m,n,nameLength,entrySize;
            size_t position = strlen(shouldBe)+1;
            bool foundAProblem = false;
            char *buffer;
            std::string name;
            
            while (position<indexLength) {
                buffer = bufferP + position;
                singleEntry.location = *((long long int *)buffer);
                singleEntry.type = buffer[8];
                m = singleEntry.m = ((int *)(buffer+9))[0];
                n = singleEntry.n = ((int *)(buffer+9))[1];
                position += 8+1+8;
                buffer = bufferP + position;
                nameLength = 0;
                while (buffer[nameLength]) nameLength++;
                
                if (nameLength==0 || m<0 || n<0) {
                    foundAProblem = true;
                    break;
                }
                
                name = std::string(buffer);
                
                switch (singleEntry.type) {
                    case 0:
                        entrySize = 8;
                        break;
                    case 10:
                    case 20:
                        entrySize = 4;
                        break;
                    case 30:
                    case 40:
                        entrySize = 2;
                        break;
                    case 50:
                    case 51:
                        entrySize = 1;
                        break;
                    case 1:
                        entrySize = 8;
                        break;
                    default:
                        foundAProblem = true;
                        entrySize = 0;
                }
                if (foundAProblem) {
                    break;
                }
                
                position += nameLength+1;
                
                if (singleEntry.location+entrySize*m*n>EndsAt) {
                    // This entry isn't completely stored in the file, ignore it.
                    DTErrorMessage("DTMatlabDataFile::ReadContent","Index has a problem");
                    foundAProblem = true;
                    break;
                }
                
                content[name] = singleEntry;
            }
            
            if (foundAProblem==false) {
                return;
            }
            else {
                // Fallback to reading the index from the file.  Clear what is here already
                content.clear();
            }
        }
    }
    
    FILE *theFile = file.FILEForReading();
    
    fseek(theFile,0,SEEK_SET);
    size_t howMuchWasRead;
    char tempString[255];
    int IsOK = 1;
    long StartsAt;
    int EntrySize = 0;
    int theSizes[10] = {8,4,4,2,2,1,-1,-1,-1,-1};
    
    int ByteOrderNumber = (DTFile::RunningOnBigEndianMachine() ? 1000 : 0);

    while (IsOK) {
        StartsAt = ftell(theFile); // Where we are currently.

        howMuchWasRead = fread(&TheMatrix,sizeof(FMatrix),1,theFile);
        
        if (howMuchWasRead==0) {
            IsOK = 0;
            break; // The file ended.
        }

        if (StartsAt==0) {
            // Check to see if the endian-ness is different.
            if (TheMatrix.type<ByteOrderNumber || TheMatrix.type>ByteOrderNumber+52) {
                // See if it helps to swap the endian-ness
                DTSwap4Bytes((unsigned char *)&TheMatrix,sizeof(FMatrix));
                needToSwapEndian = true;
            }
        }
        else if (needToSwapEndian) {
            DTSwap4Bytes((unsigned char *)&TheMatrix,sizeof(FMatrix));
        }

        if (howMuchWasRead<1 || TheMatrix.namelen<=0 || TheMatrix.type<0 || TheMatrix.type>1052 || TheMatrix.namelen>255) {
            DTErrorMessage("Reading In File Content",
                           "Invalid file format, make sure you used the -v4 in matlab.");
                if (StartsAt) needToSwapEndian = false;
                IsOK = 0; break;
        }
        
        TheMatrix.type = TheMatrix.type%1000;

        // The variable name
        howMuchWasRead = fread(tempString,1,TheMatrix.namelen,theFile);

        // Figure out the entry size to see how much to skip.
        EntrySize = theSizes[(TheMatrix.type%100)/10]*TheMatrix.mrows*TheMatrix.ncols;
        if (EntrySize<0) {
            DTErrorMessage("dataFile.ReadDoubleArray(name)",
                           "Unexpected variable type in the file.");
            IsOK = 0;
            break;
        }

        if (IsOK) {
            // Add this entry to the content list.
            singleEntry.location = StartsAt+20+TheMatrix.namelen;
            singleEntry.m = TheMatrix.mrows;
            singleEntry.n = TheMatrix.ncols;
            singleEntry.type = TheMatrix.type;
            content[string(tempString)] = singleEntry;
        }

        // Else get ready for the next entry.
        fseek(theFile,StartsAt+EntrySize+20+TheMatrix.namelen,SEEK_SET);
    }
}

void DTMatlabDataFileContent::SaveIndex(void)
{
    if (saveIndex) {
        // Already incrementally adding it.
        return;
    }
    
    saveIndex = true;
    
    // The main file needs to exist.
    if (file.IsOpen()==false) {
        return;
    }
    
    // The name should have the ending .index
    std::string theN = file.Name();
    if (theN.length()>=4 && theN.substr(theN.length()-4,4)==".mat") {
        theN = theN.substr(0,theN.length()-4)+".index";
    }
    else {
        theN = theN+".index";
    }
    DTFile newIndexFile(theN,DTFile::NewReadWrite);
    if (newIndexFile.IsOpen()==false) {
        DTErrorMessage("DTDataFile::SaveIndex","Could not create the index file");
        saveIndex = false;
        return;
    }
    
    // The format of the index file is as follows
    // First is the magic cookie, essentially the file information
    newIndexFile.WriteStringWithZero("mat index file");
    
    // Write each entry as a block of the form:
    // offset - 8 bytes
    // type - 1 byte
    // m - 4 bytes
    // n - 4 bytes
    // name - the string, 0 terminated
    
    // Going to write this as a single chunk to file, so accumulate it in memory
    int posInContent = 0;
    size_t lenOfContent = 1000;
    DTMutableCharArray bytes(lenOfContent);
    char *bytesD = bytes.Pointer();
    
    map<string,DTMatlabDataEntry>::const_iterator mapIterator;
    std::string varName;
    DTMatlabDataEntry dataEntry;
    size_t strLen;
    int m,n;
    int64_t offset;
    for (mapIterator=content.begin();mapIterator!=content.end();++mapIterator) {
        varName = mapIterator->first;
        dataEntry = mapIterator->second;
        
        strLen = varName.size();
        m = dataEntry.m;
        n = dataEntry.n;
        if (posInContent+strLen+22>lenOfContent) {
            bytes = IncreaseSize(bytes,lenOfContent+strLen+22);
            bytesD = bytes.Pointer();
            lenOfContent = bytes.Length();
        }
        offset = dataEntry.location;
        memcpy(bytesD+posInContent,(char *)(&offset),8);
        posInContent+=8;
        bytesD[posInContent++] = char(dataEntry.type);
        memcpy(bytesD+posInContent,(char *)(&m),4);
        posInContent+=4;
        memcpy(bytesD+posInContent,(char *)(&n),4);
        posInContent+=4;
        memcpy(bytesD+posInContent,varName.c_str(),strLen);
        posInContent+=strLen;
        bytesD[posInContent++] = 0;
    }
    
    newIndexFile.WriteRaw(bytesD,posInContent);
    
    indexFile = newIndexFile;
    file.Flush();
    indexFile.Flush();
}

#pragma mark DTMatlabDataFile

DTMatlabDataFile::DTMatlabDataFile()
: content(NULL)
{
    DTFile emptyFile;
    content = new DTMatlabDataFileContent(emptyFile);
}

DTMatlabDataFile::DTMatlabDataFile(DTFile file)
: content(NULL)
{
    if (file.Length()==0) {
        // Need to remove the index file if it exists
        std::string indexName;
        std::string name = file.Name();
        if (name.length()>4 && name.substr(name.length()-4,4)==".mat") {
            indexName = name.substr(0,name.length()-4)+".index";
        }
        else {
            indexName = name+".index";
        }
        unlink(indexName.c_str());
    }
    content = new DTMatlabDataFileContent(file);
}

DTMatlabDataFile::DTMatlabDataFile(const std::string &name,DTFile::OpenType oType)
: content(NULL)
{
    if (oType==DTFile::NewReadWrite) {
        // Need to remove the index file if it exists
        std::string indexName;
        if (name.length()>4 && name.substr(name.length()-4,4)==".mat") {
            indexName = name.substr(0,name.length()-4)+".index";
        }
        else {
            indexName = name+".index";
        }
        unlink(indexName.c_str());
    }
    content = new DTMatlabDataFileContent(DTFile(name,oType));
}

DTMatlabDataFile::DTMatlabDataFile(const DTMatlabDataFile &C)
: content(C.content)
{
    content->referenceCount++;
}

DTMatlabDataFile &DTMatlabDataFile::operator=(const DTMatlabDataFile &C)
{
    if (content==C.content) return *this;
    content->referenceCount--;
    if (content->referenceCount==0)
        delete content;
    content = C.content;
    content->referenceCount++;
    return *this;
}

DTMatlabDataFile::~DTMatlabDataFile()
{
    content->referenceCount--;
    if (content->referenceCount==0)
        delete content;
}

DTMutablePointer<DTDataStorage> DTMatlabDataFile::AsPointer() const
{
    return DTMutablePointer<DTDataStorage>(new DTMatlabDataFile(*this));
}

void DTMatlabDataFile::Save(int v,const std::string &name)
{
    DTMutableDoubleArray temp(1);
    temp(0) = v;
    Save(temp,name);
}

void DTMatlabDataFile::Save(double v,const std::string &name)
{
    DTMutableDoubleArray temp(1);
    temp(0) = v;
    Save(temp,name);
}

void DTMatlabDataFile::Save(const DTDoubleArray &A,const std::string &VarName)
{
    FMatrix TheMatrix;

    if (IsReadOnly()) {
        DTErrorMessage("DTMatlabDataFile::Save","File is read only.");
        return;
    }
    
    TheMatrix.type = (DTFile::RunningOnBigEndianMachine() ? 1000 : 0);
    if (A.o()<=1) {
        TheMatrix.mrows = int(A.m());
        TheMatrix.ncols = int(A.n());
    }
    else {
        Save(int(A.m()),VarName+"_3D");
        TheMatrix.mrows = int(A.m()*A.n());
        TheMatrix.ncols = int(A.o());
    }

    FILE *theFile = content->file.FILEForWriting();

    fseek(theFile,0,SEEK_END);
    TheMatrix.imagf = 0;
    TheMatrix.namelen = int(1+VarName.length());

    DTMatlabDataEntry entry;
    entry.m = TheMatrix.mrows;
    entry.n = TheMatrix.ncols;
    // entry.Fmtrx = TheMatrix;
    content->content[VarName] = entry;

    fwrite(&TheMatrix,sizeof(FMatrix), 1, theFile);
    fwrite(VarName.c_str(),sizeof(char),TheMatrix.namelen,theFile);
    entry.location = ftell(theFile);
    if (A.Length()) fwrite(A.Pointer(),sizeof(double),A.Length(),theFile);
}

void DTMatlabDataFile::Save(const DTFloatArray &A,const std::string &VarName)
{
    FMatrix TheMatrix;

    if (IsReadOnly()) {
        DTErrorMessage("DTMatlabDataFile::Save","File is read only.");
        return;
    }

    if (A.o()<=1) {
        TheMatrix.mrows = int(A.m());
        TheMatrix.ncols = int(A.n());
    }
    else {
        Save(int(A.m()),VarName+"_3D");
        TheMatrix.mrows = int(A.m()*A.n());
        TheMatrix.ncols = int(A.o());
    }

    FILE *theFile = content->file.FILEForWriting();
    fseek(theFile,0,SEEK_END);

    TheMatrix.type = (DTFile::RunningOnBigEndianMachine() ? 1010 : 10);
    TheMatrix.imagf = 0;
    TheMatrix.namelen = int(1+VarName.length());

    DTMatlabDataEntry entry;
    // entry.Fmtrx = TheMatrix;
    entry.m = TheMatrix.mrows;
    entry.n = TheMatrix.ncols;
    entry.type = TheMatrix.type;
    content->content[VarName] = entry;

    fwrite(&TheMatrix,sizeof(FMatrix), 1, theFile);
    fwrite(VarName.c_str(),sizeof(char),TheMatrix.namelen,theFile);
    entry.location = ftell(theFile);
    if (A.Length()) fwrite(A.Pointer(),sizeof(float),A.Length(),theFile);
}

void DTMatlabDataFile::Save(const DTIntArray &A,const std::string &VarName)
{
    FMatrix TheMatrix;

    if (IsReadOnly()) {
        DTErrorMessage("DTMatlabDataFile::Save","File is read only.");
        return;
    }

    if (A.o()<=1) {
        TheMatrix.mrows = int(A.m());
        TheMatrix.ncols = int(A.n());
    }
    else {
        Save(int(A.m()),VarName+"_3D");
        TheMatrix.mrows = int(A.m()*A.n());
        TheMatrix.ncols = int(A.o());
    }

    FILE *theFile = content->file.FILEForWriting();
    fseek(theFile,0,SEEK_END);

    TheMatrix.type = (DTFile::RunningOnBigEndianMachine() ? 1020 : 20);
    TheMatrix.imagf = 0;
    TheMatrix.namelen = int(1+VarName.length());

    DTMatlabDataEntry entry;
    entry.m = TheMatrix.mrows;
    entry.n = TheMatrix.ncols;
    entry.type = TheMatrix.type;
    content->content[VarName] = entry;

    fwrite(&TheMatrix,sizeof(FMatrix), 1, theFile);
    fwrite(VarName.c_str(),sizeof(char),TheMatrix.namelen,theFile);
    entry.location = ftell(theFile);
    if (A.Length()) fwrite(A.Pointer(),sizeof(int),A.Length(),theFile);
}

void DTMatlabDataFile::Save(const DTUCharArray &A,const std::string &VarName)
{
    FMatrix TheMatrix;

    if (IsReadOnly()) {
        DTErrorMessage("DTMatlabDataFile::Save","File is read only.");
        return;
    }

    FILE *theFile = content->file.FILEForWriting();
    fseek(theFile,0,SEEK_END);

    TheMatrix.type = (DTFile::RunningOnBigEndianMachine() ? 1050 : 50);
    TheMatrix.mrows = int(A.m());
    TheMatrix.ncols = int(A.n());
    TheMatrix.imagf = 0;
    TheMatrix.namelen = int(1+VarName.length());

    DTMatlabDataEntry entry;
    entry.m = TheMatrix.mrows;
    entry.n = TheMatrix.ncols;
    entry.type = TheMatrix.type;
    content->content[VarName] = entry;

    fwrite(&TheMatrix,sizeof(FMatrix), 1, theFile);
    fwrite(VarName.c_str(),sizeof(char),TheMatrix.namelen,theFile);
    entry.location = ftell(theFile);
    if (A.Length()) fwrite(A.Pointer(),sizeof(char),A.Length(),theFile);
}

void DTMatlabDataFile::Save(const DTCharArray &A,const std::string &VarName)
{
    FMatrix TheMatrix;

    if (IsReadOnly()) {
        DTErrorMessage("DTMatlabDataFile::Save","File is read only.");
        return;
    }

    FILE *theFile = content->file.FILEForWriting();
    fseek(theFile,0,SEEK_END);

    TheMatrix.type = (DTFile::RunningOnBigEndianMachine() ? 1051 : 51);
    TheMatrix.mrows = int(A.m());
    TheMatrix.ncols = int(A.n());
    TheMatrix.imagf = 0;
    TheMatrix.namelen = int(1+VarName.length());

    DTMatlabDataEntry entry;
    entry.m = TheMatrix.mrows;
    entry.n = TheMatrix.ncols;
    entry.type = TheMatrix.type;
    content->content[VarName] = entry;

    fwrite(&TheMatrix,sizeof(FMatrix), 1, theFile);
    fwrite(VarName.c_str(),sizeof(char),TheMatrix.namelen,theFile);
    entry.location = ftell(theFile);
    if (A.Length()) fwrite(A.Pointer(),sizeof(char),A.Length(),theFile);
}

void DTMatlabDataFile::Save(const DTShortIntArray &A,const std::string &VarName)
{
    FMatrix TheMatrix;
    
    if (IsReadOnly()) {
        DTErrorMessage("DTMatlabDataFile::Save","File is read only.");
        return;
    }
    
    FILE *theFile = content->file.FILEForWriting();
    fseek(theFile,0,SEEK_END);
    
    TheMatrix.type = (DTFile::RunningOnBigEndianMachine() ? 1030 : 30);
    TheMatrix.mrows = int(A.m());
    TheMatrix.ncols = int(A.n());
    TheMatrix.imagf = 0;
    TheMatrix.namelen = int(1+VarName.length());
    
    DTMatlabDataEntry entry;
    entry.m = TheMatrix.mrows;
    entry.n = TheMatrix.ncols;
    entry.type = TheMatrix.type;
    content->content[VarName] = entry;
    
    fwrite(&TheMatrix,sizeof(FMatrix), 1, theFile);
    fwrite(VarName.c_str(),sizeof(char),TheMatrix.namelen,theFile);
    entry.location = ftell(theFile);
    if (A.Length()) fwrite(A.Pointer(),sizeof(short int),A.Length(),theFile);
}

void DTMatlabDataFile::Save(const DTUShortIntArray &A,const std::string &VarName)
{
    FMatrix TheMatrix;

    if (IsReadOnly()) {
        DTErrorMessage("DTMatlabDataFile::Save","File is read only.");
        return;
    }

    FILE *theFile = content->file.FILEForWriting();
    fseek(theFile,0,SEEK_END);

    TheMatrix.type = (DTFile::RunningOnBigEndianMachine() ? 1040 : 40);
    TheMatrix.mrows = int(A.m());
    TheMatrix.ncols = int(A.n());
    TheMatrix.imagf = 0;
    TheMatrix.namelen = int(1+VarName.length());

    DTMatlabDataEntry entry;
    entry.m = TheMatrix.mrows;
    entry.n = TheMatrix.ncols;
    entry.type = TheMatrix.type;
    content->content[VarName] = entry;

    fwrite(&TheMatrix,sizeof(FMatrix), 1, theFile);
    fwrite(VarName.c_str(),sizeof(char),TheMatrix.namelen,theFile);
    entry.location = ftell(theFile);
    if (A.Length()) fwrite(A.Pointer(),sizeof(unsigned short int),A.Length(),theFile);
}

void DTMatlabDataFile::Save(const std::string &theString,const std::string &VarName)
{
    FMatrix TheMatrix;

    if (IsReadOnly()) {
        DTErrorMessage("DTMatlabDataFile::Save","File is read only.");
        return;
    }
    if (IsOpen()==false) {
        DTErrorMessage("DTMatlabDataFile::Save","File is not open.");
        return;
    }

    FILE *theFile = content->file.FILEForWriting();
    fseek(theFile,0,SEEK_END);

    TheMatrix.type = (DTFile::RunningOnBigEndianMachine() ? 1051 : 51); // 8 bit unsigned, and a string.
    TheMatrix.mrows = int(theString.length());
    TheMatrix.ncols = 1;
    TheMatrix.imagf = 0;
    TheMatrix.namelen = int(1+VarName.length());

    DTMatlabDataEntry entry;
    entry.m = TheMatrix.mrows;
    entry.n = TheMatrix.ncols;
    entry.type = TheMatrix.type;
    content->content[VarName] = entry;

    fwrite(&TheMatrix,sizeof(FMatrix), 1, theFile);
    fwrite(VarName.c_str(),sizeof(char),TheMatrix.namelen,theFile);
    entry.location = ftell(theFile);
    fwrite(theString.c_str(),sizeof(char),TheMatrix.mrows,theFile);
}

void DTMatlabDataFile::Flush(void) const
{
    FILE *theFile = content->file.FILEForReading();
    if (theFile) fflush(theFile);
}

DTMatlabDataEntry DTMatlabDataFile::FindVariable(const std::string &name) const
{
    // should be in the content->content list
    map<string,DTMatlabDataEntry>::const_iterator searchResult = content->content.find(name);
    
    if (searchResult==content->content.end()) {
        return DTMatlabDataEntry();
    }
    else {
        return searchResult->second;
    }
}

DTList<std::string> DTMatlabDataFile::AllVariableNames(void) const
{
    DTMutableList<std::string> toReturn(content->content.size());
    
    map<string,DTMatlabDataEntry>::const_iterator mapIterator;
    int pos = 0;
    DTMatlabDataEntry fileEntry;
    
    for (mapIterator=content->content.begin();mapIterator!=content->content.end();++mapIterator) {
        toReturn(pos++) = mapIterator->first;
    }
    
    return toReturn;
}

struct DTMatlabDataFilePosString {
    long int pos;
    std::string description;

    bool operator<(const DTMatlabDataFilePosString &A) const {return (pos<A.pos);}
};

void DTMatlabDataFile::printInfo(void) const
{
    vector<DTMatlabDataFilePosString> list;
    DTMatlabDataFilePosString entry;
    std::string desc,stringValue;
    DTMatlabDataEntry fileEntry;

    std::cerr << "------------------------------------------------------------------------" << std::endl;
    std::cerr << "Content of \"" << content->file.Name() << "\" - ";
    size_t howMany = content->content.size();
    if (howMany==0)
        std::cerr << "empty" << std::endl;
    else if (howMany==1)
        std::cerr << "one entry" << std::endl;
    else
        std::cerr << howMany << " entries" << std::endl;
    std::cerr << "------------------------------------------------------------------------" << std::endl;

    std::string padding = ".................................";

    map<string,DTMatlabDataEntry>::const_iterator mapIterator;
    for (mapIterator=content->content.begin();mapIterator!=content->content.end();++mapIterator) {
        fileEntry = mapIterator->second;
        entry.pos = fileEntry.location;
        desc = mapIterator->first + " ";
        // Pad to make it 30 characters
        if (desc.length()<30)
            desc = desc + string(padding,0,30-desc.length());
        switch (fileEntry.type%1000) {
            case 0:
                desc += " - double - ";
                break;
            case 10:
                desc += " -  float - ";
                break;
            case 20:
                desc += " -    int - ";
                break;
            case 40:
                desc += " - UShort - ";
                break;
            case 50:
                desc += " -  UChar - ";
                break;
            case 51:
            case 1:
                desc += " - string - ";
                break;
            default:
                desc += " - ?????? - ";
                break;
        }
        // Dimension.
        if (fileEntry.type%1000==51 || fileEntry.type%1000==1) {
            stringValue = ReadString(mapIterator->first);
            if (stringValue.length()>25) {
                stringValue = "\""+string(stringValue,0,15) + "...\" - " + DTInt2String(fileEntry.m*fileEntry.n) + " characters";
            }
            desc += "\""+stringValue+"\"";
        }
        else {
            if (fileEntry.m==0)
                desc += "Empty";
            else if (fileEntry.m==1 && fileEntry.n==1)
                desc += DTFloat2StringShort(ReadNumber(mapIterator->first));
            else if (fileEntry.n==1)
                desc += DTInt2String(fileEntry.m) + " numbers";
            else 
                desc += DTInt2String(fileEntry.m) + " x " + DTInt2String(fileEntry.n) + " array";
        }
        entry.description = desc;
        list.push_back(entry);
    }

	std::sort(list.begin(),list.end());

    // Print the content
    size_t howLong = list.size();
    size_t pos = 0;
    vector<DTMatlabDataFilePosString>::iterator iter;
    for (iter=list.begin();iter!=list.end();++iter) {
        if (pos<390 || pos>howLong-10)
            std::cerr << iter->description << std::endl;
        else if (pos==380 && pos<howLong-20)
            std::cerr << "Skipping " << howLong-400 << " entries." << std::endl;
        pos++;
    }
    std::cerr << flush;
}

bool DTMatlabDataFile::Contains(const std::string &name) const
{
    DTMatlabDataEntry entry = FindVariable(name);
    return (entry.location>=0);
}

bool DTMatlabDataFile::IsReadOnly() const
{
    return content->file.IsReadOnly();
}

bool DTMatlabDataFile::IsOpen() const
{
    return content->file.IsOpen();
}

bool DTMatlabDataFile::SavedAsCharacter(const std::string &name) const
{
    DTMatlabDataEntry entry = FindVariable(name);
    if (entry.location<0) return false;
    return (entry.type%1000==50);
}

bool DTMatlabDataFile::SavedAsShort(const std::string &name) const
{
    DTMatlabDataEntry entry = FindVariable(name);
    if (entry.location<0) return false;
    return (entry.type%1000==30 || entry.type%1000==40);
}

bool DTMatlabDataFile::SavedAsInt(const std::string &name) const
{
    DTMatlabDataEntry entry = FindVariable(name);
    if (entry.location<0) return false;
    return (entry.type%1000==20 || entry.type%1000==20);
}

bool DTMatlabDataFile::SavedAsDouble(const std::string &name) const
{
    DTMatlabDataEntry entry = FindVariable(name);
    if (entry.location<0) return false;
    return (entry.type%1000==0);
}

bool DTMatlabDataFile::SavedAsString(const std::string &name) const
{
    DTMatlabDataEntry entry = FindVariable(name);
    if (entry.location<0) return false;
    return (entry.type%1000==51 || entry.type%1000==1);
}

void DTMatlabDataFile::ShouldSaveIndexWhenClosing(void)
{
    if (content->saveIndex) return; // Save it after every entry
    content->saveIndexWhenClosing = true;
}

void DTMatlabDataFile::SaveIndex(void)
{
    content->SaveIndex();
}

DTDoubleArray DTMatlabDataFile::ReadDoubleArray(const std::string &name) const
{
    DTMatlabDataEntry entry = FindVariable(name);
    if (entry.location<0) {
        std::string msg = string("Did not find the variable \"") + name + "\" inside the datafile.";
        DTErrorMessage("dataFile.ReadDoubleArray(name)",msg);
        return DTDoubleArray();
    }

    // See if this is stored as a 3D array - hack since the matlab4 format doesn't
    // directly support it.  An array with size m x n x o
    // is saved as  m*n by o array, and m is saved as the number name_3D
    DTMatlabDataEntry entry3D = FindVariable(name+"_3D");

    // FMatrix TheMatrix = entry.Fmtrx;
    long int startOfData = entry.location;
    
    int m = entry.m;
    int n = entry.n;
    int o = 1;
    if (entry3D.location>=0) {
        int val = ReadInt(name+"_3D");
        if (val<=0)
            o = 1;
        else {
            m = val;
            n = entry.m/val;
            o = entry.n;
        }
    }

    // Now read the file.
    DTMutableDoubleArray toReturn(m,n,o);
    
    FILE *theFile = content->file.FILEForReading();
    if (entry.type%1000==0) {
        fseek(theFile,startOfData,SEEK_SET);
        if (ssize_t(fread(toReturn.Pointer(),sizeof(double),toReturn.Length(),theFile))!=toReturn.Length()) return DTDoubleArray();
        if (content->needToSwapEndian) SwapEndian(toReturn);
    }
    else if (entry.type%1000==10) {
        // This is a float arrray.
        DTMutableFloatArray temp(m,n,o);
        fseek(theFile,startOfData,SEEK_SET);
        if (ssize_t(fread(temp.Pointer(),sizeof(float),temp.Length(),theFile))!=temp.Length()) return DTDoubleArray();
        ConvertArray(temp,toReturn);
    }
    else if (entry.type%1000==20) {
        // This is an int arrray.
        DTMutableIntArray temp(m,n,o);
        fseek(theFile,startOfData,SEEK_SET);
        if (ssize_t(fread(temp.Pointer(),sizeof(int),temp.Length(),theFile))!=temp.Length()) return DTDoubleArray();
        if (content->needToSwapEndian) SwapEndian(temp);
        ConvertArray(temp,toReturn);
    }
    else if (entry.type%1000==30) {
        // This is a short arrray.
        DTMutableShortIntArray temp(m,n,o);
        fseek(theFile,startOfData,SEEK_SET);
        if (ssize_t(fread(temp.Pointer(),sizeof(short),temp.Length(),theFile))!=temp.Length()) return DTDoubleArray();
        if (content->needToSwapEndian) SwapEndian(temp);
        ConvertArray(temp,toReturn);
    }
    else if (entry.type%1000==40) {
        // This is an unsigned short arrray.
        DTMutableUShortIntArray temp(m,n,o);
        fseek(theFile,startOfData,SEEK_SET);
        if (ssize_t(fread(temp.Pointer(),sizeof(short),temp.Length(),theFile))!=temp.Length()) return DTDoubleArray();
        if (content->needToSwapEndian) SwapEndian(temp);
        ConvertArray(temp,toReturn);
    }
    else if (entry.type%1000==50) {
        // This is an unsigned char arrray.
        DTMutableCharArray temp(m,n,o);
        fseek(theFile,startOfData,SEEK_SET);
        if (ssize_t(fread(temp.Pointer(),1,temp.Length(),theFile))!=temp.Length()) return DTDoubleArray();
        ConvertArray(temp,toReturn);
    }
    else {
        DTErrorMessage("dataFile.ReadDoubleArray(name)","Illegal type.");
    }

    return toReturn;
}

DTFloatArray DTMatlabDataFile::ReadFloatArray(const std::string &name) const
{
    DTMatlabDataEntry entry = FindVariable(name);
    if (entry.location<0) {
        std::string msg = string("Did not find the variable \"") + name + "\" inside the datafile.";
        DTErrorMessage("dataFile.ReadFloatArray(name)",msg);
        return DTFloatArray();
    }

    // See if this is stored as a 3D array - hack since the matlab4 format doesn't
    // directly support it.  An array with size m x n x o
    // is saved as  m*n by o array, and m is saved as the number name_3D
    DTMatlabDataEntry entry3D = FindVariable(name+"_3D");

    // FMatrix TheMatrix = entry.Fmtrx;
    long int startOfData = entry.location;

    int m = entry.m;
    int n = entry.n;
    int o = 1;
    if (entry3D.location>=0) {
        int val = ReadInt(name+"_3D");
        if (val<=0)
            o = 1;
        else {
            m = val;
            n = entry.m/val;
            o = entry.n;
        }
    }

    // Now read the file.
    DTMutableFloatArray toReturn(m,n,o);

    FILE *theFile = content->file.FILEForReading();
    if (entry.type%1000==0) {
        // This is a double arrray.
        DTMutableDoubleArray temp(m,n,o);
        fseek(theFile,startOfData,SEEK_SET);
        fread(temp.Pointer(),sizeof(double),temp.Length(),theFile);
        if (content->needToSwapEndian) SwapEndian(temp);
        ConvertArray(temp,toReturn);
    }
    else if (entry.type%1000==10) {
        fseek(theFile,startOfData,SEEK_SET);
        fread(toReturn.Pointer(),sizeof(float),toReturn.Length(),theFile);
        if (content->needToSwapEndian) SwapEndian(toReturn);
    }
    else if (entry.type%1000==20) {
        // This is an int arrray.
        DTMutableIntArray temp(m,n,o);
        fseek(theFile,startOfData,SEEK_SET);
        fread(temp.Pointer(),sizeof(int),temp.Length(),theFile);
        if (content->needToSwapEndian) SwapEndian(temp);
        ConvertArray(temp,toReturn);
    }
    else if (entry.type%1000==30) {
        // This is a short arrray.
        DTMutableShortIntArray temp(m,n,o);
        fseek(theFile,startOfData,SEEK_SET);
        fread(temp.Pointer(),sizeof(short),temp.Length(),theFile);
        if (content->needToSwapEndian) SwapEndian(temp);
        ConvertArray(temp,toReturn);
    }
    else if (entry.type%1000==40) {
        // This is an unsigned short arrray.
        DTMutableUShortIntArray temp(m,n,o);
        fseek(theFile,startOfData,SEEK_SET);
        fread(temp.Pointer(),sizeof(short),temp.Length(),theFile);
        if (content->needToSwapEndian) SwapEndian(temp);
        ConvertArray(temp,toReturn);
    }
    else if (entry.type%1000==50) {
        // This is an unsigned char arrray.
        DTMutableCharArray temp(m,n,o);
        fseek(theFile,startOfData,SEEK_SET);
        fread(temp.Pointer(),1,temp.Length(),theFile);
        ConvertArray(temp,toReturn);
    }
    else {
        DTErrorMessage("dataFile.ReadFloatArray(name)","Illegal type.");
    }
    
    return toReturn;
}

DTIntArray DTMatlabDataFile::ReadIntArray(const std::string &name) const
{
    DTMatlabDataEntry entry = FindVariable(name);
    if (entry.location<0) {
        std::string msg = string("Did not find the variable \"") + name + "\" inside the datafile.";
        DTErrorMessage("dataFile.ReadIntArray(name)",msg);
        return DTIntArray();
    }

    DTMatlabDataEntry entry3D = FindVariable(name+"_3D");
    
    // FMatrix TheMatrix = entry.Fmtrx;
    long int startOfData = entry.location;

    int m = entry.m;
    int n = entry.n;
    int o = 1;
    if (entry3D.location>=0) {
        int val = ReadInt(name+"_3D");
        if (val<=0)
            o = 1;
        else {
            m = val;
            n = entry.m/val;
            o = entry.n;
        }
    }

    // Now read the file.
    DTMutableIntArray toReturn(m,n,o);
    FILE *theFile = content->file.FILEForReading();

    if (entry.type%1000==0) {
        // This is a double arrray.
        DTMutableDoubleArray temp(m,n,o);
        fseek(theFile,startOfData,SEEK_SET);
        fread(temp.Pointer(),sizeof(double),temp.Length(),theFile);
        if (content->needToSwapEndian) SwapEndian(temp);
        ConvertArray(temp,toReturn);
    }
    else if (entry.type%1000==10) {
        // This is a float arrray.
        DTMutableFloatArray temp(m,n,o);
        fseek(theFile,startOfData,SEEK_SET);
        fread(temp.Pointer(),sizeof(float),temp.Length(),theFile);
        if (content->needToSwapEndian) SwapEndian(temp);
        ConvertArray(temp,toReturn);
    }
    else if (entry.type%1000==20) {
        fseek(theFile,startOfData,SEEK_SET);
        fread(toReturn.Pointer(),sizeof(int),toReturn.Length(),theFile);
        if (content->needToSwapEndian) SwapEndian(toReturn);
    }
    else if (entry.type%1000==30) {
        // This is a short arrray.
        DTMutableShortIntArray temp(m,n,o);
        fseek(theFile,startOfData,SEEK_SET);
        fread(temp.Pointer(),sizeof(short),temp.Length(),theFile);
        if (content->needToSwapEndian) SwapEndian(temp);
        ConvertArray(temp,toReturn);
    }
    else if (entry.type%1000==40) {
        // This is an unsigned short arrray.
        DTMutableUShortIntArray temp(m,n,o);
        fseek(theFile,startOfData,SEEK_SET);
        fread(temp.Pointer(),sizeof(short),temp.Length(),theFile);
        if (content->needToSwapEndian) SwapEndian(temp);
        ConvertArray(temp,toReturn);
    }
    else if (entry.type%1000==50) {
        // This is an unsigned char arrray.
        DTMutableCharArray temp(m,n,o);
        fseek(theFile,startOfData,SEEK_SET);
        fread(temp.Pointer(),1,temp.Length(),theFile);
        ConvertArray(temp,toReturn);
    }
    else {
        DTErrorMessage("dataFile.ReadIntArray(name)","Illegal type.");
    }
    
    return toReturn;
}

DTUCharArray DTMatlabDataFile::ReadUCharArray(const std::string &name) const
{
    DTMatlabDataEntry entry = FindVariable(name);
    if (entry.location<0) {
        std::string msg = string("Did not find the variable \"") + name + "\" inside the datafile.";
        DTErrorMessage("dataFile.ReadUCharArray(name)",msg);
        return DTUCharArray();
    }

    DTMatlabDataEntry entry3D = FindVariable(name+"_3D");
    
    long int startOfData = entry.location;

    int m = entry.m;
    int n = entry.n;
    int o = 1;
    if (entry3D.location>=0) {
        int val = ReadInt(name+"_3D");
        if (val<=0)
            o = 1;
        else {
            m = val;
            n = entry.m/val;
            o = entry.n;
        }
    }

    // Now read the file.
    DTMutableUCharArray toReturn(m,n,o);
    FILE *theFile = content->file.FILEForReading();

    if (entry.type%1000==0) {
        // This is a double arrray.
        DTMutableDoubleArray temp(m,n,o);
        fseek(theFile,startOfData,SEEK_SET);
        fread(temp.Pointer(),sizeof(double),temp.Length(),theFile);
        if (content->needToSwapEndian) SwapEndian(temp);
        ConvertArray(temp,toReturn);
    }
    else if (entry.type%1000==10) {
        // This is a float arrray.
        DTMutableFloatArray temp(m,n,o);
        fseek(theFile,startOfData,SEEK_SET);
        fread(temp.Pointer(),sizeof(float),temp.Length(),theFile);
        if (content->needToSwapEndian) SwapEndian(temp);
        ConvertArray(temp,toReturn);
    }
    else if (entry.type%1000==20) {
        // This is an int arrray.
        DTMutableIntArray temp(m,n,o);
        fseek(theFile,startOfData,SEEK_SET);
        fread(temp.Pointer(),sizeof(int),temp.Length(),theFile);
        if (content->needToSwapEndian) SwapEndian(temp);
        ConvertArray(temp,toReturn);
    }
    else if (entry.type%1000==30) {
        // This is a short arrray.
        DTMutableShortIntArray temp(m,n,o);
        fseek(theFile,startOfData,SEEK_SET);
        fread(temp.Pointer(),sizeof(short),temp.Length(),theFile);
        if (content->needToSwapEndian) SwapEndian(temp);
        ConvertArray(temp,toReturn);
    }
    else if (entry.type%1000==40) {
        // This is an unsigned short arrray.
        DTMutableUShortIntArray temp(m,n,o);
        fseek(theFile,startOfData,SEEK_SET);
        fread(temp.Pointer(),sizeof(short),temp.Length(),theFile);
        if (content->needToSwapEndian) SwapEndian(temp);
        ConvertArray(temp,toReturn);
    }
    else if (entry.type%1000==50) {
        fseek(theFile,startOfData,SEEK_SET);
        fread(toReturn.Pointer(),sizeof(char),toReturn.Length(),theFile);
    }
    else {
        DTErrorMessage("dataFile.ReadUCharArray(name)","Illegal type.");
    }
    
    return toReturn;
}

DTCharArray DTMatlabDataFile::ReadCharArray(const std::string &name) const
{
    DTMatlabDataEntry entry = FindVariable(name);
    if (entry.location<0) {
        std::string msg = string("Did not find the variable \"") + name + "\" inside the datafile.";
        DTErrorMessage("dataFile.ReadCharArray(name)",msg);
        return DTCharArray();
    }

    DTMatlabDataEntry entry3D = FindVariable(name+"_3D");

    // FMatrix TheMatrix = entry.Fmtrx;
    long int startOfData = entry.location;

    int m = entry.m;
    int n = entry.n;
    int o = 1;
    if (entry3D.location>=0) {
        int val = ReadInt(name+"_3D");
        if (val<=0)
            o = 1;
        else {
            m = val;
            n = entry.m/val;
            o = entry.n;
        }
    }

    // Now read the file.
    DTMutableCharArray toReturn(m,n,o);
    FILE *theFile = content->file.FILEForReading();

    if (entry.type%1000==0 || entry.type%1000==1) {
        // This is a double arrray.
        DTMutableDoubleArray temp(m,n,o);
        fseek(theFile,startOfData,SEEK_SET);
        fread(temp.Pointer(),sizeof(double),temp.Length(),theFile);
        if (content->needToSwapEndian) SwapEndian(temp);
        ConvertArray(temp,toReturn);
    }
    else if (entry.type%1000==10) {
        // This is a float arrray.
        DTMutableFloatArray temp(m,n,o);
        fseek(theFile,startOfData,SEEK_SET);
        fread(temp.Pointer(),sizeof(float),temp.Length(),theFile);
        if (content->needToSwapEndian) SwapEndian(temp);
        ConvertArray(temp,toReturn);
    }
    else if (entry.type%1000==20) {
        // This is an int arrray.
        DTMutableIntArray temp(m,n,o);
        fseek(theFile,startOfData,SEEK_SET);
        fread(temp.Pointer(),sizeof(int),temp.Length(),theFile);
        if (content->needToSwapEndian) SwapEndian(temp);
        ConvertArray(temp,toReturn);
    }
    else if (entry.type%1000==30) {
        // This is a short arrray.
        DTMutableShortIntArray temp(m,n,o);
        fseek(theFile,startOfData,SEEK_SET);
        fread(temp.Pointer(),sizeof(short),temp.Length(),theFile);
        if (content->needToSwapEndian) SwapEndian(temp);
        ConvertArray(temp,toReturn);
    }
    else if (entry.type%1000==40) {
        // This is an unsigned short arrray.
        DTMutableUShortIntArray temp(m,n,o);
        fseek(theFile,startOfData,SEEK_SET);
        fread(temp.Pointer(),sizeof(short),temp.Length(),theFile);
        if (content->needToSwapEndian) SwapEndian(temp);
        ConvertArray(temp,toReturn);
    }
    else if (entry.type%1000==50 || entry.type%1000==51) {
        fseek(theFile,startOfData,SEEK_SET);
        fread(toReturn.Pointer(),sizeof(char),toReturn.Length(),theFile);
    }
    else {
        DTErrorMessage("dataFile.ReadCharArray(name)","Haven't taken care of this case.");
    }

    return toReturn;
}

DTShortIntArray DTMatlabDataFile::ReadShortIntArray(const std::string &name) const
{
    DTMatlabDataEntry entry = FindVariable(name);
    if (entry.location<0) {
        std::string msg = string("Did not find the variable \"") + name + "\" inside the datafile.";
        DTErrorMessage("dataFile.ReadShortIntArray(name)",msg);
        return DTShortIntArray();
    }

    DTMatlabDataEntry entry3D = FindVariable(name+"_3D");

    long int startOfData = entry.location;

    int m = entry.m;
    int n = entry.n;
    int o = 1;
    if (entry3D.location>=0) {
        int val = ReadInt(name+"_3D");
        if (val<=0)
            o = 1;
        else {
            m = val;
            n = entry.m/val;
            o = entry.n;
        }
    }

    // Now read the file.
    DTMutableShortIntArray toReturn(m,n,o);
    FILE *theFile = content->file.FILEForReading();

    if (entry.type%1000==0) {
        // This is a double arrray.
        DTMutableDoubleArray temp(m,n,o);
        fseek(theFile,startOfData,SEEK_SET);
        fread(temp.Pointer(),sizeof(double),temp.Length(),theFile);
        if (content->needToSwapEndian) SwapEndian(temp);
        ConvertArray(temp,toReturn);
    }
    else if (entry.type%1000==10) {
        // This is a float arrray.
        DTMutableFloatArray temp(m,n,o);
        fseek(theFile,startOfData,SEEK_SET);
        fread(temp.Pointer(),sizeof(float),temp.Length(),theFile);
        if (content->needToSwapEndian) SwapEndian(temp);
        ConvertArray(temp,toReturn);
    }
    else if (entry.type%1000==20) {
        // This is an int arrray.
        DTMutableIntArray temp(m,n,o);
        fseek(theFile,startOfData,SEEK_SET);
        fread(temp.Pointer(),sizeof(int),temp.Length(),theFile);
        if (content->needToSwapEndian) SwapEndian(temp);
        ConvertArray(temp,toReturn);
    }
    else if (entry.type%1000==30) {
        // This is a short arrray.
        fseek(theFile,startOfData,SEEK_SET);
        fread(toReturn.Pointer(),sizeof(short),toReturn.Length(),theFile);
        if (content->needToSwapEndian) SwapEndian(toReturn);
    }
    else if (entry.type%1000==40) {
        // This is an unsigned short arrray.
        DTMutableUShortIntArray temp(m,n,o);
        fseek(theFile,startOfData,SEEK_SET);
        fread(temp.Pointer(),sizeof(short),temp.Length(),theFile);
        if (content->needToSwapEndian) SwapEndian(temp);
        ConvertArray(temp,toReturn);
    }
    else if (entry.type%1000==50) {
        // This is an unsigned char arrray.
        DTMutableCharArray temp(m,n,o);
        fseek(theFile,startOfData,SEEK_SET);
        fread(temp.Pointer(),1,temp.Length(),theFile);
        ConvertArray(temp,toReturn);
    }
    else {
        DTErrorMessage("dataFile.ReadShortArray(name)","Illegal type.");
    }
    
    return toReturn;
}

DTUShortIntArray DTMatlabDataFile::ReadUShortIntArray(const std::string &name) const
{
    DTMatlabDataEntry entry = FindVariable(name);
    if (entry.location<0) {
        std::string msg = string("Did not find the variable \"") + name + "\" inside the datafile.";
        DTErrorMessage("dataFile.ReadUShortIntArray(name)",msg);
        return DTUShortIntArray();
    }
    
    DTMatlabDataEntry entry3D = FindVariable(name+"_3D");
    
    // FMatrix TheMatrix = entry.Fmtrx;
    long int startOfData = entry.location;

    int m = entry.m;
    int n = entry.n;
    int o = 1;
    if (entry3D.location>=0) {
        int val = ReadInt(name+"_3D");
        if (val<=0)
            o = 1;
        else {
            m = val;
            n = entry.m/val;
            o = entry.n;
        }
    }
    
    // Now read the file.
    DTMutableUShortIntArray toReturn(m,n,o);
    FILE *theFile = content->file.FILEForReading();
    
    if (entry.type%1000==0) {
        // This is a double arrray.
        DTMutableDoubleArray temp(m,n,o);
        fseek(theFile,startOfData,SEEK_SET);
        fread(temp.Pointer(),sizeof(double),temp.Length(),theFile);
        if (content->needToSwapEndian) SwapEndian(temp);
        ConvertArray(temp,toReturn);
    }
    else if (entry.type%1000==10) {
        // This is a float arrray.
        DTMutableFloatArray temp(m,n,o);
        fseek(theFile,startOfData,SEEK_SET);
        fread(temp.Pointer(),sizeof(float),temp.Length(),theFile);
        if (content->needToSwapEndian) SwapEndian(temp);
        ConvertArray(temp,toReturn);
    }
    else if (entry.type%1000==20) {
        // This is an int arrray.
        DTMutableIntArray temp(m,n,o);
        fseek(theFile,startOfData,SEEK_SET);
        fread(temp.Pointer(),sizeof(int),temp.Length(),theFile);
        if (content->needToSwapEndian) SwapEndian(temp);
        ConvertArray(temp,toReturn);
    }
    else if (entry.type%1000==30) {
        // This is a short arrray.
        DTMutableShortIntArray temp(m,n,o);
        fseek(theFile,startOfData,SEEK_SET);
        fread(temp.Pointer(),sizeof(short),temp.Length(),theFile);
        if (content->needToSwapEndian) SwapEndian(temp);
        ConvertArray(temp,toReturn);
    }
    else if (entry.type%1000==40) {
        // Unsigned short array
        fseek(theFile,startOfData,SEEK_SET);
        fread(toReturn.Pointer(),sizeof(unsigned short int),toReturn.Length(),theFile);
        if (content->needToSwapEndian) SwapEndian(toReturn);
    }
    else if (entry.type%1000==50) {
        // This is an unsigned char arrray.
        DTMutableCharArray temp(m,n,o);
        fseek(theFile,startOfData,SEEK_SET);
        fread(temp.Pointer(),1,temp.Length(),theFile);
        ConvertArray(temp,toReturn);
    }
    else {
        DTErrorMessage("dataFile.ReadUShortArray(name)","Illegal type.");
    }
    
    return toReturn;
}

double DTMatlabDataFile::ReadNumber(const std::string &name) const
{
    DTDoubleArray theArr = ReadDoubleArray(name);
    if (theArr.IsEmpty() || theArr.Length()!=1)
        return 0.0;

    return theArr(0);
}

int DTMatlabDataFile::ReadInt(const std::string &name) const
{
    DTIntArray theArr = ReadIntArray(name);
    if (theArr.IsEmpty() || theArr.Length()!=1)
        return 0;

    return theArr(0);
}

string DTMatlabDataFile::ReadString(const std::string &name) const
{
    DTMatlabDataEntry entry = FindVariable(name);
    if (entry.location<0) {
        std::string msg = string("Did not find the variable \"") + name + "\" inside the datafile.";
        DTErrorMessage("dataFile.ReadString(name)",msg);
        return std::string();
    }

    // FMatrix TheMatrix = entry.Fmtrx;
    long int startOfData = entry.location;

    int theLen = entry.m*entry.n;
    if (theLen==0)
        return std::string();
    
    std::string toReturn;
    if (entry.type%1000==51) {
        FILE *theFile = content->file.FILEForReading();
        DTMutableUCharArray theBytes(theLen+1);
        fseek(theFile,startOfData,SEEK_SET);
        fread(theBytes.Pointer(),sizeof(char),theLen,theFile);
        theBytes(theLen) = 0;
        toReturn = string((char *)theBytes.Pointer());
    }
    else if (entry.type%1000==1) {
        // Double precision
        FILE *theFile = content->file.FILEForReading();
        DTMutableDoubleArray theNumbers(theLen);
        fseek(theFile,startOfData,SEEK_SET);
        fread(theNumbers.Pointer(),sizeof(double),theLen,theFile);
        DTMutableUCharArray theBytes(theLen+1);
        for (int i=0;i<theLen;i++) {
            theBytes(i) = (unsigned char)theNumbers(i);
        }
        theBytes(theLen) = 0;
        toReturn = string((char *)theBytes.Pointer());
    }
    else {
        std::string msg = string("The variable \"") + name + "\" is not saved as a string.";
        DTErrorMessage("dataFile.ReadString(name)",msg);
    }

    return toReturn;
}

