// Part of DTSource. Copyright 2004-2017. David Adalsteinsson.
// see http://www.visualdatatools.com/DTSource/license.html for more information.

#include "DTLiveUpdate.h"
#include "DTLiveUpdateStorage.h"
#include "DTError.h"
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

DTLiveUpdateStorage sharedStorage;

bool DTLiveHasPublished = false;
pthread_t DTLiveSocketThread;

static void *DTLiveCommThread(void *storagePointer);

void *DTLiveCommThread(void *storagePointer)
{
    ((DTLiveUpdateStorage *)storagePointer)->RunCommunication();
    return NULL;
}

DTLiveUpdateStorage::~DTLiveUpdateStorage()
{
    if (!currentlyConnected)
        return;
    
    waitingToSendToDT.Unlock();
    quitLock.Lock();
    shouldQuit = true;
    quitLock.Unlock();
    
    if (containerToSendOver.WaitingForClient() ||
        containerToSendOver.CurrentlyReading()) {
        // Need to force the issue.
        containerToSendOver.ForceQuit();
    }
    
    if (DTLiveHasPublished)
        pthread_join(DTLiveSocketThread,NULL);
}

bool DTLiveUpdateStorage::Publish(string socketName)
{
    // Ignore the SIGPIPE signal.  We will catch any error codes from the read/write messages,
    // and since this is a multi-threaded code it's fairly hairy to create a good signal handler anyway.
    
    signal(SIGPIPE,SIG_IGN);

    if (DTLiveHasPublished) {
        DTErrorMessage("DTLiveUpdateStorage::Publish","Calling this function twice.");
        return false;
    }
    shouldQuit = false;
    
    // Delete the existing socket if it exists.
    remove(socketName.c_str());
    
    // Create the socket, and launch a thread that will monitor messages on this thread.
    // Only containerToSendOver entry is being sent over.
    if (containerToSendOver.RegisterSocket(socketName)==false)
        return false;
    
    DTLiveHasPublished = true;
    
    syncLock.Lock(); // Locking it for the socket thread
                     // Start the communication thread
    pthread_create(&DTLiveSocketThread,NULL,DTLiveCommThread,this);
    // Wait until this thread starts.
    syncLock.Lock();
    syncLock.Unlock();
    
    return true;
}

bool DTLiveUpdateStorage::Publish(unsigned short int port,string responseToDT,string requiredAnswer)
{
    if (DTLiveHasPublished) {
        DTErrorMessage("DTLiveUpdateStorage::Publish","Calling this function twice.");
        return false;
    }
    shouldQuit = false;
    
    // Create the socket, and launch a thread that will monitor messages on this thread.
    // Only containerToSendOver entry is being sent over.
    if (containerToSendOver.RegisterSocket(port,responseToDT,requiredAnswer)==false)
        return false;
    
    DTLiveHasPublished = true;
    
    syncLock.Lock(); // Locking it for the socket thread
                     // Start the communication thread
    pthread_create(&DTLiveSocketThread,NULL,DTLiveCommThread,this);
    // Wait until this thread starts.
    syncLock.Lock();
    syncLock.Unlock();
    
    return true;
}

bool DTLiveUpdateStorage::WaitForDataTankToAskQuestion(void)
{
    // Wait for a message.
    char msg[6];
    ssize_t howMuchRead = containerToSendOver.ReadFromStream(msg,6);
    if (howMuchRead<6) {
        containerToSendOver.ClientFailed();
        return false;
    }
    
    // This could be a add or remove message.
    int stringLen;
    DTMutableCharArray nameArr;
    std::string varName;
    bool failed = false;
    
    while (failed==false && msg[0]!='S') {
        stringLen = 1;
        while (stringLen>0) {
            howMuchRead = containerToSendOver.ReadFromStream(&stringLen,4);
            if (howMuchRead<4) {
                DTErrorMessage("DTLiveUpdateStorage::Synchronize","Failed (3)");
                containerToSendOver.ClientFailed();
                failed = true;
                break;
            }
            if (stringLen>0) {
                nameArr = DTMutableCharArray(stringLen);
                howMuchRead = containerToSendOver.ReadFromStream(nameArr.Pointer(),stringLen);
                if (howMuchRead<stringLen) {
                    DTErrorMessage("DTLiveUpdateStorage::Synchronize","Failed (4)");
                    containerToSendOver.ClientFailed();
                    failed = true;
                    break;
                }
                varName = string(nameArr.Pointer(),stringLen);
                if (msg[0]=='R') {
                    // Remove the entry from the send set
                    variablesToSendOver.erase(varName);
                }
                else {
                    // Add the entry to the send set
                    variablesToSendOver.insert(varName);
                }
            }
        }
        howMuchRead = containerToSendOver.ReadFromStream(msg,6);
        if (howMuchRead<6) {
            DTErrorMessage("DTLiveUpdateStorage::Synchronize","Failed (5)");
            containerToSendOver.ClientFailed();
            failed = true;
            break;
        }
    }
    
    if (failed)
        return false;
    
    // Possibly later in the separate thread, this will be moved over to DataTank
    DTDataContainer whatToSend;
    DTDataContainer whatWillExistInDataTank;
    
    latestSyncStorageLock.Lock();
    syncStorageIsNewer = false;
    quitLock.Lock();
    if (shouldQuit) {
        quitLock.Unlock();
        return false;
    }
    quitLock.Unlock();
    waitingToSendToDT.TryLock();
    whatToSend.OverwriteContentWith(latestSyncStorage);
    latestSyncStorageLock.Unlock();
    
    // Now compare what is different from contentInDataTank, and put that into the containerToSendOver object
    containerToSendOver.RemoveAllEntries();
    
    // For every entry in whatToSend, one of the following holds
    // It does not exist in contentInDataTank, send it
    // It does exist, but is different - send it
    whatToSend.AccessLock().Lock();
    map<string,DTDataContainerEntry>::const_iterator begin = whatToSend.FirstEntry();
    map<string,DTDataContainerEntry>::const_iterator end = whatToSend.LastEntry();
    map<string,DTDataContainerEntry>::const_iterator mapIterator;
    std::string name;
    DTDataContainerEntry entry;
    DTDataContainerEntry entryInExistingContent;
    for (mapIterator=begin;mapIterator!=end;++mapIterator) {
        name = mapIterator->first;
        entry = mapIterator->second;
        
        if (variablesToSendOver.find(name)==variablesToSendOver.end())
            continue; // Not in the list of entries to send.
        
        entryInExistingContent = contentInDataTank.FindVariable(name);
        whatWillExistInDataTank.AddEntry(entry,name);
        if (entry!=entryInExistingContent) {
            // Otherwise it hasn't changed, so don't send it again.
            containerToSendOver.AddEntry(entry,name);
        }
    }
    whatToSend.AccessLock().Unlock();
    
    if (containerToSendOver.ClientConnected()==false)
        return false;
    
    // Send the data over the socket
    containerToSendOver.WriteIntoSocket();
    
    // Now that sync has been complete, record what is on the other side.
    contentInDataTank.OverwriteContentWith(whatWillExistInDataTank);
    
    return true;
}    

void DTLiveUpdateStorage::RunCommunication(void)
{
    char messageString[6];
    size_t howMuchWritten, howMany, i;
    int sockNumber;
    
    // Running in a separate thread.
    waitingToSendToDT.Lock();
    syncLock.Unlock(); // To allow Publish to continue.
    
    while (1) {
        quitLock.Lock();
        if (shouldQuit) {
            quitLock.Unlock();
            break;
        }
        quitLock.Unlock();
        
        // If the connection to DataTank dies, try again.
        
        // Wait for DataTank to connect
        currentlyConnected = false;
        if (containerToSendOver.WaitForClient()==false)
            return;
        currentlyConnected = true;
        
        quitLock.Lock();
        if (shouldQuit) {
            quitLock.Unlock();
            break;
        }
        quitLock.Unlock();
        
        // Wait until I get a message from DataTank that will start the process.
        // This should be in a separate thread.
        sockNumber = containerToSendOver.SocketNumber();
        
        // Read in the string "SndOvr".
        containerToSendOver.ReadFromStream(messageString,6);
        
        // Send over all of the variable definitions that are stored (and stacked up).
        howMany = variableList.size();
        for (i=0;i<howMany;i++) {
            SendVariableDefinition(variableList[i]);
        }
        
        if (containerToSendOver.ClientConnected()==false) continue;
        
        // Start a loop that will let DataTank know whenever a change is avilable.
        // Wait until DataTank asks for the data before sending the next change message.
        while (1) {
            // Wait until something exists in the "send to DataTank" que.
            // The possibilities are either a new variable definition was added, or the storage was updated.
            
            waitingToSendToDT.Lock();
            // New data in the sync container.
            
            quitLock.Lock();
            if (shouldQuit) {
                quitLock.Unlock();
                break;
            }
            quitLock.Unlock();
            
            lockForWaitingVariables.Lock();
            if (variablesWaiting.size()) {
                // New variables defined.  Need to send those over.  Possibly a chunk of them.
                // DataTank will not reload the data.
                howMany = variablesWaiting.size();
                for (i=0;i<howMany;i++) {
                    variableList.push_back(variablesWaiting[i]);
                    SendVariableDefinition(variablesWaiting[i]);
                }
                variablesWaiting.erase(variablesWaiting.begin(),variablesWaiting.end());
            }
            lockForWaitingVariables.Unlock();
            
            latestSyncStorageLock.Lock();
            if (syncStorageIsNewer==false) {
                DTErrorMessage("DTLiveUpdateStorage::RunCommunication","Unexpected.");
            }
            latestSyncStorageLock.Unlock();
            // The storage has been updated.  DataTank should get the new entries.
            howMuchWritten = write(sockNumber,"NewAvl",6);
            if (howMuchWritten<6) {
                DTErrorMessage("DTLiveUpdateStorage::RunCommunication","Failed (1)");
                containerToSendOver.ClientFailed();
                break;
            }
            
            if (WaitForDataTankToAskQuestion()==false)
                break;
        }
        
        contentInDataTank.RemoveAllEntries();
        
        waitingToSendToDT.Unlock();
    }
    
}

void DTLiveUpdateStorage::Synchronize()
{
    // Copy the current state over.
    latestSyncStorageLock.Lock();
    latestSyncStorage.OverwriteContentWith(containerToSave);
    // the sync storage has changed, so the communication thread should
    // be notified and send over the data.
    syncStorageIsNewer = true;
    // Make sure that the lock is open.
    waitingToSendToDT.TryLock();
    waitingToSendToDT.Unlock();
    latestSyncStorageLock.Unlock();
}

void DTLiveUpdateStorage::AddVariable(string varName,string varType)
{
    DTLiveUpdateNameType nameType(varName,varType);
    
    lockForWaitingVariables.Lock();
    variablesWaiting.push_back(nameType);
    lockForWaitingVariables.Unlock();
}

void DTLiveUpdateStorage::SendVariableDefinition(DTLiveUpdateNameType nameType)
{
    containerToSave.Save(nameType.type,"Seq_"+nameType.name);
    // Let the socket know about this.
    
    if (containerToSendOver.ClientConnected()==false)
        return;
    
    int sockNumber = containerToSendOver.SocketNumber();
    std::string varName = nameType.name;
    std::string varType = nameType.type;
    
    // Send first the message "AddVar"
    bool failed = (write(sockNumber,"AddVar",6)!=6);
    
    // The number of entries that I'm defining
    ssize_t howMany = 1;
    failed = (failed || write(sockNumber,&howMany,4)!=4);
    
    // Write the variable name (length+string), then type (length+string)
    howMany = varName.length();
    failed = (failed || write(sockNumber,&howMany,4)!=4);
    failed = (failed || write(sockNumber,varName.c_str(),howMany)!=howMany);
    
    howMany = varType.length();
    failed = (failed || write(sockNumber,&howMany,4)!=4);
    failed = (failed || write(sockNumber,varType.c_str(),howMany)!=howMany);
    
    if (failed) {
        DTErrorMessage("DTLiveUpdateStorage::SendVariableDefinition","write failed");
        return;
    }
    
    // Wait for an ack from DataTank
    char ackStr[6];
    containerToSendOver.ReadFromStream(ackStr,6);
}

bool DTLiveUpdateStorage::CurrentlySendingOver(string varName)
{
    return (currentlyConnected && (variablesToSendOver.find(varName)!=variablesToSendOver.end()));
}

bool DTLiveUpdateStorage::CurrentlyConnected(void)
{
    return currentlyConnected;
}

bool DTLivePublish(string socketName)
{
    return sharedStorage.Publish(socketName);
}

bool DTLivePublish(int port,string responseToDT,string requiredAnswer)
{
    return sharedStorage.Publish((unsigned short int)port,responseToDT,requiredAnswer);
}

void DTLiveAddVariable(string varName,string varType)
{
    sharedStorage.AddVariable(varName,varType);
}

DTDataContainer DTLiveDataObject()
{
    return sharedStorage.containerToSave;
}

void DTLiveSynchronize()
{
    sharedStorage.Synchronize();
}

bool DTLiveCurrentlySendingOver(string varName)
{
    return sharedStorage.CurrentlySendingOver(varName);
}

bool DTLiveCurrentlyConnected()
{
    return sharedStorage.CurrentlyConnected();
}

int DTCLivePublishLocal(char *socketName)
{
    return DTLivePublish(socketName);
}

int DTCLivePublishRemote(int portNumber,char *responseToDT,char *requiredAnswer)
{
    return DTLivePublish(portNumber,responseToDT,requiredAnswer);
}

void DTCLiveAddVariable(char *varName,char *varType)
{
    DTLiveAddVariable(varName,varType);
}

int DTCLiveCurrentlySendingOver(char *varName)
{
    return DTLiveCurrentlySendingOver(varName);
}

int DTCLiveCurrentlyConnected(void)
{
    return DTLiveCurrentlyConnected();
}

void DTCLiveSynchronize()
{
    DTLiveSynchronize();
}

