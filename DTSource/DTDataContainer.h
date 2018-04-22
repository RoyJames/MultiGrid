// Part of DTSource. Copyright 2004-2017. David Adalsteinsson.
// see http://www.visualdatatools.com/DTSource/license.html for more information.

#ifndef DTDataContainer_Header
#define DTDataContainer_Header

#include "DTDataStorage.h"

#include "DTCharArray.h"
#include "DTDoubleArray.h"
#include "DTFloatArray.h"
#include "DTIntArray.h"
#include "DTShortIntArray.h"
#include "DTUShortIntArray.h"
#include "DTCharArray.h"
#include "DTUCharArray.h"
#include "DTLocked.h"

#include <string>
#include <map>

/*
 This class will handle socket communication with DataTank.  It is still under development, and
 might change frequently.

 To see how it is used, create a project from any of the the "External Program" modules.  You need
 to set the communication method to "Data Streams".
 
 This class is derived from the DTDataStorage base class, just like DTDataFile and DTMatlabDataFile.
 The main difference is that all of the entries are kept in memory.
 
 One usage of this class (separate from DataTank) is to use it as a dynamic data repository where you can
 pass between bigger blocks of your code.  Example:
 
 DTDataContainer inputEntries;
 
 inputEntries = computeInputProperties();
 
 DTDataContainer computed = foo(inputEntries);

 where entries are read and written by using Read and Write, which is overloaded for all of the
 variable types.
 
 */

struct DTDataContainerEntry {
    DTDataContainerEntry() : type(0), sEntry(), dArray(), fArray(), iArray(), sArray(), usArray(), cArray(), ucArray() {}
        
    int type;

    std::string sEntry;
    DTDoubleArray dArray;
    DTFloatArray fArray;
    DTIntArray iArray;
    DTShortIntArray sArray;
    DTUShortIntArray usArray;
    DTCharArray cArray;
    DTUCharArray ucArray;
    
    int NotFound(void) const {return type==0;}
    size_t m(void) const;
    size_t n(void) const;
    size_t o(void) const;
    
    bool operator!=(const DTDataContainerEntry &) const;
};

class DTDataContainerStorage
{
public:
    DTDataContainerStorage();
    ~DTDataContainerStorage();
    
    int sockfd;
    int communicationSocket;
    bool createdSocket;
    std::string socketName;
    std::string responseToDT;
    std::string requiredAnswer;
    DTMutableCharArray buffer;
    short int portNumber;
    
    bool waitingForClient;
    bool cancelAfterAccept;
    
    bool isReadOnly;
    std::map<std::string,DTDataContainerEntry> content;
    DTLock accessLock;
    
    DTLocked<bool> currentlyReading;
    
private:
    DTDataContainerStorage(const DTDataContainerStorage &);
    DTDataContainerStorage &operator=(const DTDataContainerStorage &);
};

class DTDataContainer : public DTDataStorage {
public:
    DTDataContainer();
    
    std::map<std::string,DTDataContainerEntry>::const_iterator FirstEntry(void) {return content->content.begin();}
    std::map<std::string,DTDataContainerEntry>::const_iterator LastEntry(void) {return content->content.end();}
    
    DTLock &AccessLock(void) {return content->accessLock;}
    bool ConnectToServerSocket(const std::string &);
    bool ReadFromSocket();
    void RemoveAllEntries();
    bool WriteIntoSocket();
    bool WaitingForClient(void) const {return content->waitingForClient;}
    size_t NumberOfEntries() {return (content->content.size());}
    
    bool RegisterSocket(const std::string &);
    bool RegisterSocket(unsigned short int port,const std::string &responseToDT,const std::string &requiredAnswer);

    bool WaitForClient(void);
    void TerminateCommunication(void);
    void ForceQuit(void);
    bool ClientConnected(void) const {return (content->communicationSocket>0);}
    void ClientFailed(void);
    bool CurrentlyReading(void) const {return content->currentlyReading;}
    
    int SocketNumber(void) const;
    size_t ReadFromStream(void *,size_t howMuh);
    
    void OverwriteContentWith(const DTDataContainer &);
    
    // Member functions that have to be overwritten.
    DTMutablePointer<DTDataStorage> AsPointer() const;

    void printInfo(void) const;

    DTList<std::string> AllVariableNames(void) const;
    bool Contains(const std::string &name) const;
    bool IsReadOnly(void) const;

    // Saving data.
    void Save(int v,const std::string &name);
    void Save(double v,const std::string &name);
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
    std::string ReadString(const std::string &name) const;

    void AddEntry(DTDataContainerEntry,const std::string &);
    DTDataContainerEntry FindVariable(const std::string &name) const;

private:
    DTDataContainerEntry FindVariableInsideLock(const std::string &name) const;
    double ReadNumberInsideLock(const std::string &name) const;
    std::string ReadStringInsideLock(const std::string &name) const;
    DTDoubleArray ReadDoubleArrayInsideLock(const std::string &name) const;

    DTMutablePointer<DTDataContainerStorage> content;
};


#endif
