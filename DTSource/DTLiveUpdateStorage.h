// Part of DTSource. Copyright 2004-2017. David Adalsteinsson.
// see http://www.visualdatatools.com/DTSource/license.html for more information.

#include "DTDataContainer.h"
#include "DTLock.h"

#include <set>
#include <vector>

struct DTLiveUpdateNameType
{
    DTLiveUpdateNameType() {}
    DTLiveUpdateNameType(string n,string t) : name(n), type(t) {}
    
    std::string name;
    std::string type;
};

struct DTLiveUpdateStorage
{
    DTLiveUpdateStorage() : syncStorageIsNewer(false), newVariableDefinitions(false), syncChanged(false), currentlyConnected(false) {}
    ~DTLiveUpdateStorage();
    
    // Current state.  Constantly being added to.
    DTDataContainer containerToSave; // Currently being added to.
    
    // State at the latest sync call.
    DTLock latestSyncStorageLock;
    DTDataContainer latestSyncStorage; // The value at the latest sync call.
    bool syncStorageIsNewer;
    
    // State that is currently in DataTank.
    DTDataContainer contentInDataTank;
    
    // The storage that should be sent over, and contains the connection.
    DTDataContainer containerToSendOver;
    
    DTLock waitingToSendToDT;
    
    // A list of variable definitions that have to be sent over to DataTank.
    bool newVariableDefinitions;
    
    DTLock syncLock;
    bool syncChanged;
    
    // Member functions.
    bool Publish(string socketName);
    bool Publish(unsigned short int port,string responseToDT,string requiredAnswer);
    void Synchronize();
    void AddVariable(string varName,string varType);
    bool CurrentlySendingOver(string);
    bool CurrentlyConnected(void);
    bool WaitForDataTankToAskQuestion(void);
    
    void SendVariableDefinition(DTLiveUpdateNameType nameType);
    
    void RunCommunication(void); // The communication thread.
    
    // set of entries that should be sent over.  DT creates that list and updates it as necessary through
    // the socket connection.
    set<std::string> variablesToSendOver;
    vector<DTLiveUpdateNameType> variableList; // DataTank knows about this.
    
    // Entries that DataTank doesn't know about yet.
    DTLock lockForWaitingVariables;
    vector<DTLiveUpdateNameType> variablesWaiting;
    
    DTLock quitLock;
    bool shouldQuit;
    bool currentlyConnected;
};
