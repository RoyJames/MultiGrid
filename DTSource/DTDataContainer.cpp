// Part of DTSource. Copyright 2004-2017. David Adalsteinsson.
// see http://www.visualdatatools.com/DTSource/license.html for more information.

#include "DTDataContainer.h"

#include "DTArrayConversion.h"
#include "DTError.h"
#include "DTUtilities.h"
#include "DTEndianSwap.h"

#include <sys/types.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdio.h>
#include <netinet/in.h>
#include <netinet/in.h>
#include <netdb.h> 

#include <cstring>
#include <strings.h>
#include <stdlib.h>

using namespace std;
#include <algorithm>

bool DTDataContainerEntry::operator!=(const DTDataContainerEntry &C) const
{
    if (type!=C.type)
        return true;
    
    bool toReturn = false;
    switch (type) {
        case 2:
            toReturn = (dArray!=C.dArray);
            break;
        case 3:
            toReturn = (fArray!=C.fArray);
            break;
        case 4:
            toReturn = (iArray!=C.iArray);
            break;
        case 6:
            toReturn = (sArray!=C.sArray);
            break;
        case 7:
            toReturn = (usArray!=C.usArray);
            break;
        case 8:
            toReturn = (cArray!=C.cArray);
            break;
        case 9:
            toReturn = (ucArray!=C.ucArray);
            break;
    }
    
    return toReturn;
}

size_t DTDataContainerEntry::m(void) const
{
    size_t toReturn = 0;
    
    switch (type) {
        case 2:
            toReturn = dArray.m();
            break;
        case 3:
            toReturn = fArray.m();
            break;
        case 4:
            toReturn = iArray.m();
            break;
        case 6:
            toReturn = sArray.m();
            break;
        case 7:
            toReturn = usArray.m();
            break;
        case 8:
            toReturn = cArray.m();
            break;
        case 9:
            toReturn = ucArray.m();
            break;
    }
    
    return toReturn;
}

size_t DTDataContainerEntry::n(void) const
{
    size_t toReturn = 0;
    
    switch (type) {
        case 2:
            toReturn = dArray.n();
            break;
        case 3:
            toReturn = fArray.n();
            break;
        case 4:
            toReturn = iArray.n();
            break;
        case 6:
            toReturn = sArray.n();
            break;
        case 7:
            toReturn = usArray.n();
            break;
        case 8:
            toReturn = cArray.n();
            break;
        case 9:
            toReturn = ucArray.n();
            break;
    }
    
    return toReturn;
}

size_t DTDataContainerEntry::o(void) const
{
    size_t toReturn = 0;
    
    switch (type) {
        case 2:
            toReturn = dArray.o();
            break;
        case 3:
            toReturn = fArray.o();
            break;
        case 4:
            toReturn = iArray.o();
            break;
        case 6:
            toReturn = sArray.o();
            break;
        case 7:
            toReturn = usArray.o();
            break;
        case 8:
            toReturn = cArray.o();
            break;
        case 9:
            toReturn = ucArray.o();
            break;
    }
    
    return toReturn;
}

DTDataContainerStorage::~DTDataContainerStorage()
{
    if (socketName.length()>0) {
        if (createdSocket)
            unlink(socketName.c_str());
        socketName = std::string();
    }
    else if (portNumber>0) {
        if (communicationSocket) shutdown(communicationSocket,2);
        if (sockfd) shutdown(sockfd,2);
    }
}

DTDataContainerStorage::DTDataContainerStorage() 
{
    sockfd = -1;
    communicationSocket = -1;
    createdSocket = false;
    portNumber = 0;
    waitingForClient = false;
    cancelAfterAccept = false;
    isReadOnly = false;
    currentlyReading = false;
    // buffer = DTMutableCharArray(4*1024*1024); // 4MB buffer, to group together small entries.
}

DTDataContainer::DTDataContainer()
{
    content = DTMutablePointer<DTDataContainerStorage>(new DTDataContainerStorage());
}

bool DTDataContainer::RegisterSocket(const std::string &sockName)
{
    int sockfd;
    
    if (sockName.length()>103) {
        DTErrorMessage("DTDataContainer::RegisterSocket","Invalid socket name (too long)");
        return false;
    }
    if ((sockfd = socket(AF_UNIX,SOCK_STREAM,0)) < 0) {
        DTErrorMessage("DTDataContainer::RegisterSocket","Failed to create socket.");
        return false;
    }
    
    struct sockaddr_un serv_addr;
    
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sun_family = AF_UNIX;
    strcpy(serv_addr.sun_path,sockName.c_str());
    socklen_t servlen = socklen_t(strlen(serv_addr.sun_path) + 1 + sizeof(serv_addr.sun_family));

    if(::bind(sockfd,(struct sockaddr *)&serv_addr,servlen)<0) {
        DTErrorMessage("DTDataContainer::RegisterSocket","Error when binding a socket");
        return false;
    }
    
    listen(sockfd,5);
    
    content->sockfd = sockfd;
    content->socketName = sockName;
    content->createdSocket = true;
    
    return true;
}

bool DTDataContainer::RegisterSocket(unsigned short int portno,const std::string &responseToDT,const std::string &requiredAnswer)
{
    int sockfd;
    
    if ((sockfd = socket(AF_INET,SOCK_STREAM,0)) < 0) {
        DTErrorMessage("DTDataContainer::RegisterSocket","Failed to create socket.");
        return false;
    }
    
    struct sockaddr_in serv_addr;
    
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if (::bind(sockfd, (struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
        DTErrorMessage("DTDataContainer::RegisterSocket","Error when binding a socket to a port");
        return false;
    }

    listen(sockfd,5);

    content->sockfd = sockfd;
    content->createdSocket = true;
    content->portNumber = portno;
    content->responseToDT = responseToDT;
    content->requiredAnswer = requiredAnswer;
    
    return true;
}

bool DTDataContainer::WaitForClient(void)
{
    int sockfd = content->sockfd;
    int tempInt;
    
    if (content->portNumber>0) {
        // Looking at a port
        struct sockaddr_in cli_addr;
        socklen_t clilen = sizeof(cli_addr);
        // Wait until the client registers.
        while (1) {
            content->waitingForClient = true;
            content->communicationSocket = accept(sockfd,(struct sockaddr *)&cli_addr,&clilen);
            content->waitingForClient = false;
            if (content->cancelAfterAccept) return false;
            if (content->communicationSocket<0) return false;
            
            // Ask questions to see if the connection is valid.
            
            // If needed, send over the key to DataTank
            tempInt = int(content->responseToDT.length());
            if (write(content->communicationSocket,&tempInt,4)<4) {
                shutdown(content->communicationSocket,2);
                content->communicationSocket = 0;
                continue;
            }
            if (tempInt) {
                if (write(content->communicationSocket,content->responseToDT.c_str(),tempInt)<tempInt) {
                    shutdown(content->communicationSocket,2);
                    content->communicationSocket = 0;
                    continue;
                }
            }
            
            // Sent the key over to DataTank.  Now ask for the password.
            // DataTank should have realized that this is not a password sniffer.
            if (read(content->communicationSocket,&tempInt,4)<4 || tempInt<0 || tempInt>10000) {
                shutdown(content->communicationSocket,2);
                content->communicationSocket = 0;
                continue;
            }
            
            DTMutableCharArray buff(tempInt);
            if (read(content->communicationSocket,buff.Pointer(),tempInt)<tempInt) {
                shutdown(content->communicationSocket,2);
                content->communicationSocket = 0;
                continue;
            }
            
            // See if this is the right password.
            std::string theStr(buff.Pointer(),tempInt);
            if (theStr!=content->requiredAnswer) {
                DTErrorMessage("DTDataContainer::WaitForClient","Invalid Password");
                shutdown(content->communicationSocket,2);
                content->communicationSocket = 0;
                continue;
            }
            
            // Let the remote know that the password was accepted.
            tempInt = 1;
            if (write(content->communicationSocket,&tempInt,4)!=4) {
                DTErrorMessage("DTDataContainer::WaitForClient","Failed at writing to socket");
            }
            
            break;
        }
    }
    else {
        // Local connection.
        struct sockaddr_un cli_addr;
        socklen_t clilen = sizeof(cli_addr);
        // Wait until the client registers.
        content->waitingForClient = true;
        content->communicationSocket = accept(sockfd,(struct sockaddr *)&cli_addr,&clilen);
        content->waitingForClient = false;
        if (content->cancelAfterAccept) return false;
        if (content->communicationSocket<0) {
            DTErrorMessage("DTDataContainer::WaitForClient","Connection failed.");
            return false;
        }
    }
    
    return true;
}

void DTDataContainer::RemoveAllEntries()
{
    content->accessLock.Lock();
    content->content.erase(content->content.begin(),content->content.end());
    content->accessLock.Unlock();
}

size_t DTDataContainer::ReadFromStream(void *rawPtr,size_t howMuch)
{
    size_t howMuchHasBeenRead = 0;
    size_t howMuchToRead = howMuch;
    size_t bytesInChunk;
    int commSocket = SocketNumber();
    
    content->currentlyReading = true;
    while (howMuchHasBeenRead<howMuchToRead) {
        bytesInChunk = read(commSocket,((char *)rawPtr)+howMuchHasBeenRead,howMuchToRead-howMuchHasBeenRead);
        if (bytesInChunk<=0)
            break;
        howMuchHasBeenRead += bytesInChunk;
    }
    content->currentlyReading = false;
    
    return howMuchHasBeenRead;
}

bool DTDataContainer::WriteIntoSocket()
{
    if (!ClientConnected())
        return false;
    
    int sockfd = SocketNumber();
    int m,n,o;
    
    content->accessLock.Lock();

    // Start transmission.
    const char *msg = "DTDataContainerStart";
    ssize_t singleInt = strlen(msg);
    if (write(sockfd,msg,singleInt)<singleInt) {
        DTErrorMessage("DTDataContainer::WriteIntoSocket","Socket failed (0)");
        ClientFailed();
        content->accessLock.Unlock();
        return false;
    }
    
    ssize_t howMuchWritten;
    
    // Iterate over all the entries, and send them over the socket.
    DTDataContainerEntry dataEntry;
    std::string dataName;
    
    DTMutableCharArray buffer(100);
    int posInBuffer;
    
    // First send over how many entries there are
    ssize_t howMany = content->content.size();
    if (write(sockfd,&howMany,4)<4) {
        DTErrorMessage("DTDataContainer::WriteIntoSocket","Socket failed (0)");
        ClientFailed();
        content->accessLock.Unlock();
        return false;
    }
    
    map<string,DTDataContainerEntry>::const_iterator mapIterator;
    for (mapIterator=content->content.begin();mapIterator!=content->content.end();++mapIterator) {
        dataName = mapIterator->first;
        dataEntry = mapIterator->second;
        
        // Send the entry, name - type - size - data.
        singleInt = dataName.length();
        std::memcpy(buffer.Pointer(),&singleInt,4);
        posInBuffer = 4;
        
        if (singleInt+posInBuffer>buffer.Length()-20) {
            buffer = IncreaseSize(buffer,singleInt+buffer.Length());
        }
        
        // The variable name
        std::memcpy(buffer.Pointer()+posInBuffer,dataName.c_str(),singleInt);
        posInBuffer+=singleInt;
        
        // The variable type
        std::memcpy(buffer.Pointer()+posInBuffer,&dataEntry.type,4);
        posInBuffer+=4;

        if (dataEntry.type==1) {
            // String
            singleInt = dataEntry.sEntry.length();
            std::memcpy(buffer.Pointer()+posInBuffer,&singleInt,4);
            posInBuffer+=4;

            if (singleInt) {
                // Add the string to the buffer.
                if (singleInt+posInBuffer>buffer.Length()-20) {
                    buffer = IncreaseSize(buffer,singleInt+buffer.Length());
                }
                std::memcpy(buffer.Pointer()+posInBuffer,dataEntry.sEntry.c_str(),singleInt);
                posInBuffer += singleInt;
                
                howMuchWritten = write(sockfd,buffer.Pointer(),posInBuffer);
                if (howMuchWritten<posInBuffer) {
                    DTErrorMessage("DTDataContainer::WriteIntoSocket","Socket failed (7)");
                    ClientFailed();
                    break;
                }
            }
        }
        else {
            // An array.
            int dim[3];
            m = dim[0] = int(dataEntry.m());
            n = dim[1] = int(dataEntry.n());
            o = dim[2] = int(dataEntry.o());
            std::memcpy(buffer.Pointer()+posInBuffer,dim,12);
            posInBuffer+=12;
            
            howMuchWritten = write(sockfd,buffer.Pointer(),posInBuffer);
            if (howMuchWritten<posInBuffer) {
                DTErrorMessage("DTDataContainer::WriteIntoSocket","Socket failed (7)");
                ClientFailed();
                break;
            }
            if (m*n*o) {
                bool worked = false;
                switch (dataEntry.type) {
                    case 2:
                        worked = (write(sockfd,(const char *)dataEntry.dArray.Pointer(),m*n*o*sizeof(double))==m*n*o*sizeof(double));
                        break;
                    case 3:
                        worked = (write(sockfd,(const char *)dataEntry.fArray.Pointer(),m*n*o*sizeof(float))==m*n*o*sizeof(float));
                        break;
                    case 4:
                        worked = (write(sockfd,(const char *)dataEntry.iArray.Pointer(),m*n*o*sizeof(int))==m*n*o*sizeof(int));
                        break;
                    case 6:
                        worked = (write(sockfd,(const char *)dataEntry.sArray.Pointer(),m*n*o*sizeof(short))==m*n*o*sizeof(short));
                        break;
                    case 7:
                        worked = (write(sockfd,(const char *)dataEntry.usArray.Pointer(),m*n*o*sizeof(short))==m*n*o*sizeof(short));
                        break;
                    case 8:
                        worked = (write(sockfd,(const char *)dataEntry.cArray.Pointer(),m*n*o)==m*n*o);
                        break;
                    case 9:
                        worked = (write(sockfd,(const char *)dataEntry.ucArray.Pointer(),m*n*o)==m*n*o);
                        break;
                }
                if (worked==0) {
                    DTErrorMessage("DTDataContainer::WriteIntoSocket","Socket failed (8)");
                }
            }
        }
    }
    
    if (!ClientConnected()) {
        content->accessLock.Unlock();
        return false;
    }
    
    content->accessLock.Unlock();
    
    return true;
}

void DTDataContainer::TerminateCommunication(void)
{
    int commSocket = content->communicationSocket;
    const char *termMsg = "Terminate";
    content->accessLock.Lock();
    if (write(commSocket,termMsg,9)!=9) {
        DTErrorMessage("DTDataContainer::TerminateCommunication","Could not terminate connection properly.");
    }
    content->accessLock.Unlock();
    // shutdown(content->sockfd,2);
}

void DTDataContainer::ForceQuit(void)
{
    content->accessLock.Lock();
    content->cancelAfterAccept = true;
    if (content->waitingForClient) {
        int tempsockfd = 0;
        
        if (content->portNumber>0) {
            struct sockaddr_in serv_addr;
            bzero((char *) &serv_addr, sizeof(serv_addr));
            struct hostent *server = gethostbyname("localhost");
            serv_addr.sin_family = AF_INET;
            bcopy((char *)server->h_addr, 
                  (char *)&serv_addr.sin_addr.s_addr,
                  server->h_length);
            serv_addr.sin_port = htons(content->portNumber);

            // Connect
            if ((tempsockfd = socket(AF_INET, SOCK_STREAM,0)) < 0) {
                DTErrorMessage("DTDataContainer::ForceQuit","Could not create socket.");
            }
            else if (connect(tempsockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0) {
                DTErrorMessage("DTDataContainer::ForceQuit","Error connecting.");
            }
            else {
                // Close it continue
                shutdown(tempsockfd,2);
            }
        }
        else {
            struct sockaddr_un serv_addr;
            bzero((char *)&serv_addr,sizeof(serv_addr));
            serv_addr.sun_family = AF_UNIX;
            strcpy(serv_addr.sun_path,(content->socketName.c_str()));
            socklen_t servlen = socklen_t(strlen(serv_addr.sun_path) + 1 + sizeof(serv_addr.sun_family));
            
            // Connect
            if ((tempsockfd = socket(AF_UNIX, SOCK_STREAM,0)) < 0) {
                DTErrorMessage("DTDataContainer::ForceQuit","Could not create socket.");
            }
            else if (connect(tempsockfd, (struct sockaddr *) &serv_addr, servlen) < 0) {
                DTErrorMessage("DTDataContainer::ForceQuit","Error connecting.");
            }
            else {
                // Close it continue
                shutdown(tempsockfd,2);
            }
        }
    }
    if (content->sockfd>0) {
        int how = shutdown(content->sockfd,2);
        if (how!=0) {
            std::cerr << "error = " << how << std::endl;
        }
    }
    content->sockfd = 0;
    if (content->communicationSocket>0)
        shutdown(content->communicationSocket,2);
    content->communicationSocket = 0;
    
    if (content->createdSocket)
        unlink(content->socketName.c_str());
    content->accessLock.Unlock();
}

int DTDataContainer::SocketNumber(void) const
{
    if (content->communicationSocket<=0) {
        DTErrorMessage("DTDataContainer::SocketNumber","No client connected.");
    }
    return content->communicationSocket;
}

void DTDataContainer::ClientFailed(void)
{
    // Called because a read or write failed.
    if (content->communicationSocket) {
        shutdown(content->communicationSocket,2);
    }
    content->communicationSocket = 0;
}

void DTDataContainer::OverwriteContentWith(const DTDataContainer &from)
{
    content->accessLock.Lock();
    // Delete existing entries.
    content->content.erase(content->content.begin(),content->content.end());
    
    // Copy entries from the existing map.    
    from.content->accessLock.Lock();
    map<string,DTDataContainerEntry>::const_iterator mapIterator;
    for (mapIterator=from.content->content.begin();mapIterator!=from.content->content.end();++mapIterator) {
        content->content[mapIterator->first] = mapIterator->second;
    }
    from.content->accessLock.Unlock();
    content->accessLock.Unlock();
}

bool DTDataContainer::ConnectToServerSocket(const std::string &sockName)
{
    int sockfd;
    struct sockaddr_un serv_addr;
    
    bzero((char *)&serv_addr,sizeof(serv_addr));
    serv_addr.sun_family = AF_UNIX;
    strcpy(serv_addr.sun_path,sockName.c_str());
    socklen_t servlen = socklen_t(strlen(serv_addr.sun_path) + 1 + sizeof(serv_addr.sun_family));
    
    // Connect
    if ((sockfd = socket(AF_UNIX, SOCK_STREAM,0)) < 0) {
        DTErrorMessage("DTDataContainer::ConnectToServerSocket","Could not create socket.");
        return false;
    }
    if (connect(sockfd, (struct sockaddr *) &serv_addr, servlen) < 0) {
        DTErrorMessage("DTDataContainer::ConnectToServerSocket","Error connecting.");
        return false;
    }
    
    content->socketName = sockName;
    content->communicationSocket = sockfd;
    
    return true;
}

bool DTDataContainer::ReadFromSocket()
{
    // Wait for the start.
    char message[22];
    ssize_t howMuchRead;
    
    content->accessLock.Lock();

    howMuchRead = ReadFromStream(message,20);
    if (howMuchRead<20) {
        content->accessLock.Unlock();
        return false;
    }
    
    // Need to make sure that the server is valid.
    if (strncmp(message,"DTDataContainerStart",20)!=0) {
        DTErrorMessage("DTDataContainer::ReadFromSocket","Invalid response from server.");
        content->accessLock.Unlock();
        return false;
    }
    
    DTMutableCharArray nameArr;
    DTMutableDoubleArray dArr;
    DTMutableFloatArray fArr;
    DTMutableIntArray iArr;
    DTMutableShortIntArray sArr;
    DTMutableUShortIntArray usArr;
    DTMutableCharArray cArr;
    DTMutableUCharArray ucArr;
    
    std::string varName;
    DTDataContainerEntry dataEntry;
    
    int howManyEntries;
    if (ReadFromStream(&howManyEntries,4)<4) {
        content->accessLock.Unlock();
        return false;
    }

    // Possible that this is running with different endian-ness.
    // Check this by seeing the sanity of the number that gets sent over.
    bool swapBytes = (howManyEntries>256*256);
    if (swapBytes) DTSwap4Bytes((unsigned char *)&howManyEntries,4);

    int theType,stringLen,m,n,o;
    int i;
    for (i=0;i<howManyEntries;i++) {
        // The length of the variable name
        howMuchRead = ReadFromStream(&stringLen,4);
        if (swapBytes) DTSwap4Bytes((unsigned char *)&stringLen,4);
        if (howMuchRead<4 || stringLen<=0)
            break;
        
        nameArr = DTMutableCharArray(stringLen);
        howMuchRead = ReadFromStream(nameArr.Pointer(),stringLen);
        if (howMuchRead<stringLen)
            break;
        
        varName = string(nameArr.Pointer(),stringLen);
        
        // Read in the type number
        howMuchRead = ReadFromStream(&theType,4);
        if (swapBytes) DTSwap4Bytes((unsigned char *)&theType,4);
        if (howMuchRead<4 || theType<0)
            break;
        
        dataEntry = DTDataContainerEntry();
        if (theType==1) {
            // The entry is a string.
            howMuchRead = ReadFromStream(&stringLen,4);
            if (swapBytes) DTSwap4Bytes((unsigned char *)&stringLen,4);
            if (howMuchRead<4 || stringLen<0)
                break;
            nameArr = DTMutableCharArray(stringLen);
            howMuchRead = ReadFromStream((char *)nameArr.Pointer(),stringLen);
            if (howMuchRead<stringLen)
                break;
            dataEntry.type = 1;
            dataEntry.sEntry = string(nameArr.Pointer(),stringLen);
        }
        else {
            // This is an array, begins with the dimension.
            howMuchRead = ReadFromStream(&m,4);
            if (swapBytes) DTSwap4Bytes((unsigned char *)&m,4);
            if (howMuchRead<4 || m<0)
                break;
            howMuchRead = ReadFromStream(&n,4);
            if (swapBytes) DTSwap4Bytes((unsigned char *)&n,4);
            if (howMuchRead<4 || n<0)
                break;
            howMuchRead = ReadFromStream(&o,4);
            if (swapBytes) DTSwap4Bytes((unsigned char *)&o,4);
            if (howMuchRead<4 || o<0)
                break;
            dataEntry.type = theType;
            if (m*n*o) {
                switch (theType) {
                    case 2:
                        // Double array
                        dArr = DTMutableDoubleArray(m,n,o);
                        howMuchRead = ReadFromStream((char *)dArr.Pointer(),m*n*o*sizeof(double))/sizeof(double);
                        if (swapBytes) DTSwap8Bytes((unsigned char *)dArr.Pointer(),m*n*o*sizeof(double));
                        dataEntry.dArray = dArr;
                        break;
                    case 3:
                        fArr = DTMutableFloatArray(m,n,o);
                        howMuchRead = ReadFromStream((char *)fArr.Pointer(),m*n*o*sizeof(float))/sizeof(float);
                        if (swapBytes) DTSwap4Bytes((unsigned char *)fArr.Pointer(),m*n*o*sizeof(float));
                        dataEntry.fArray = fArr;
                        break;
                    case 4:
                        iArr = DTMutableIntArray(m,n,o);
                        howMuchRead = ReadFromStream((char *)iArr.Pointer(),m*n*o*sizeof(int))/sizeof(int);
                        if (swapBytes) DTSwap4Bytes((unsigned char *)iArr.Pointer(),m*n*o*sizeof(int));
                        dataEntry.iArray = iArr;
                        break;
                    case 6:
                        sArr = DTMutableShortIntArray(m,n,o);
                        howMuchRead = ReadFromStream((char *)sArr.Pointer(),m*n*o*2)/2;
                        if (swapBytes) DTSwap8Bytes((unsigned char *)sArr.Pointer(),m*n*o*sizeof(short));
                        dataEntry.sArray = sArr;
                        break;
                    case 7:
                        usArr = DTMutableUShortIntArray(m,n,o);
                        howMuchRead = ReadFromStream((char *)usArr.Pointer(),m*n*o*2)/2;
                        if (swapBytes) DTSwap8Bytes((unsigned char *)usArr.Pointer(),m*n*o*sizeof(short));
                        dataEntry.usArray = usArr;
                        break;
                    case 8:
                        cArr = DTMutableCharArray(m,n,o);
                        howMuchRead = ReadFromStream((char *)cArr.Pointer(),m*n*o);
                        dataEntry.cArray = cArr;
                        break;
                    case 9:
                        ucArr = DTMutableUCharArray(m,n,o);
                        howMuchRead = ReadFromStream((char *)ucArr.Pointer(),m*n*o);
                        dataEntry.ucArray = ucArr;
                        break;
                    default:
                        DTErrorMessage("Unexpected type sent over","Sent over the type number " + DTInt2String(theType));
                        howMuchRead = 0;
                }
                if (howMuchRead<m*n*o) {
                    break;
                }
            }
        }
        content->content[varName] = dataEntry;
    }

    content->accessLock.Unlock();

    return true;
}

DTMutablePointer<DTDataStorage> DTDataContainer::AsPointer() const
{
    return DTMutablePointer<DTDataStorage>(new DTDataContainer(*this));
}

DTDataContainerEntry DTDataContainer::FindVariable(const std::string &name) const
{
    DTDataContainerEntry toReturn;
    
    content->accessLock.Lock();
    // should be in the content->content list
    map<string,DTDataContainerEntry>::const_iterator searchResult = content->content.find(name);

    if (searchResult!=content->content.end()) {
        toReturn = searchResult->second;
    }
    content->accessLock.Unlock();
    
    return toReturn;
}

DTDataContainerEntry DTDataContainer::FindVariableInsideLock(const std::string &name) const
{
    DTDataContainerEntry toReturn;
    
    // should be in the content->content list
    map<string,DTDataContainerEntry>::const_iterator searchResult = content->content.find(name);
    
    if (searchResult!=content->content.end()) {
        toReturn = searchResult->second;
    }
    
    return toReturn;
}

DTList<std::string> DTDataContainer::AllVariableNames(void) const
{
    content->accessLock.Lock();

    DTMutableList<std::string> toReturn(content->content.size());
    
    map<string,DTDataContainerEntry>::const_iterator mapIterator;
    int pos = 0;
    DTDataContainerEntry fileEntry;
    
    for (mapIterator=content->content.begin();mapIterator!=content->content.end();++mapIterator) {
        toReturn(pos++) = mapIterator->first;
    }
    
    content->accessLock.Unlock();
    
    return toReturn;
}

struct DTDataContainerString {
    std::string name;
    std::string description;
    
    bool operator<(const DTDataContainerString &A) const {return (name<A.name);}
};

void DTDataContainer::printInfo(void) const
{
    vector<DTDataContainerString> list;
    DTDataContainerString entry;
    std::string desc,stringValue;
    DTDataContainerEntry fileEntry;
    
    content->accessLock.Lock();
    
    std::cerr << "----------------------------------------------------------------" << std::endl;
    std::cerr << "Content of data container" << std::endl;
    std::cerr << "----------------------------------------------------------------" << std::endl;
    
    std::string padding = ".................................";
    
    map<string,DTDataContainerEntry>::const_iterator mapIterator;
    for (mapIterator=content->content.begin();mapIterator!=content->content.end();++mapIterator) {
        fileEntry = mapIterator->second;
        entry.name = mapIterator->first;
        desc = mapIterator->first + " ";
        // Pad to make it 30 characters
        if (desc.length()<30)
            desc = desc + string(padding,0,30-desc.length());
        switch (fileEntry.type) {
            case 1:
                desc += " - string - ";
                break;
            case 2:
                desc += " - double - ";
                break;
            case 3:
                desc += " -  float - ";
                break;
            case 4:
                desc += " -    int - ";
                break;
            case 6:
                desc += " -  Short - ";
                break;
            case 7:
                desc += " - UShort - ";
                break;
            case 8:
                desc += " -   Char - ";
                break;
            case 9:
                desc += " -  UChar - ";
                break;
            default:
                desc += " - ?????? - ";
                break;
        }
        // Dimension.
        if (fileEntry.type==1) {
            stringValue = ReadStringInsideLock(mapIterator->first);
            if (stringValue.length()>25) {
                stringValue = "\""+string(stringValue,0,15) + "...\" - " + DTSize2String(stringValue.length()) + " characters";
            }
            desc += "\""+stringValue+"\"";
        }
        else {
            int m = int(fileEntry.m());
            int n = int(fileEntry.n());
            int o = int(fileEntry.o());
            if (m==0)
                desc += "Empty";
            else if (m==1 && n==1 && o==1)
                desc += DTFloat2StringShort(ReadNumberInsideLock(mapIterator->first));
            else if (n==1 && o==1)
                desc += DTInt2String(m) + " numbers";
            else if (o==1)
                desc += DTInt2String(m) + " x " + DTInt2String(n) + " array";
            else
                desc += DTInt2String(m) + " x " + DTInt2String(n) + " x " + DTInt2String(o) + " array";
        }
        entry.description = desc;
        list.push_back(entry);
    }
    
    sort(list.begin(),list.end());
    
    // Print the content
    ssize_t howLong = list.size();
    int pos = 0;
    vector<DTDataContainerString>::iterator iter;
    for (iter=list.begin();iter!=list.end();++iter) {
        if (pos<390 || pos>howLong-10)
            std::cerr << iter->description << std::endl;
        else if (pos==380 && pos<howLong-20)
            std::cerr << "Skipping " << howLong-400 << " entries." << std::endl;
        pos++;
    }
    
    content->accessLock.Unlock();
}

bool DTDataContainer::Contains(const std::string &name) const
{
    DTDataContainerEntry entry = FindVariable(name);
    return (entry.type!=0);
}

bool DTDataContainer::IsReadOnly(void) const
{
    return content->isReadOnly;
}

void DTDataContainer::Save(int v,const std::string &name)
{
    DTMutableIntArray temp(1);
    temp(0) = v;
    Save(temp,name);
}

void DTDataContainer::Save(double v,const std::string &name)
{
    DTMutableDoubleArray temp(1);
    temp(0) = v;
    Save(temp,name);
}

void DTDataContainer::Save(const DTDoubleArray &A,const std::string &name)
{
    if (IsReadOnly()) {
        DTErrorMessage("DTDataContainer::Save","Container is read only.");
        return;
    }

    DTDataContainerEntry entry;
    entry.type = 2;
    entry.dArray = A.Copy();

    content->accessLock.Lock();
    content->content[name] = entry;
    content->accessLock.Unlock();
}

void DTDataContainer::Save(const DTFloatArray &A,const std::string &name)
{
    if (IsReadOnly()) {
        DTErrorMessage("DTDataContainer::Save","Container is read only.");
        return;
    }

    DTDataContainerEntry entry;
    entry.type = 3;
    entry.fArray = A;

    content->accessLock.Lock();
    content->content[name] = entry;
    content->accessLock.Unlock();
}

void DTDataContainer::Save(const DTIntArray &A,const std::string &name)
{
    if (IsReadOnly()) {
        DTErrorMessage("DTDataContainer::Save","Container is read only.");
        return;
    }

    DTDataContainerEntry entry;
    entry.type = 4;
    entry.iArray = A;

    content->accessLock.Lock();
    content->content[name] = entry;
    content->accessLock.Unlock();
}

void DTDataContainer::Save(const DTShortIntArray &A,const std::string &name)
{
    if (IsReadOnly()) {
        DTErrorMessage("DTDataContainer::Save","Container is read only.");
        return;
    }
    
    DTDataContainerEntry entry;
    entry.type = 6;
    entry.sArray = A;
    
    content->accessLock.Lock();
    content->content[name] = entry;
    content->accessLock.Unlock();
}

void DTDataContainer::Save(const DTUShortIntArray &A,const std::string &name)
{
    if (IsReadOnly()) {
        DTErrorMessage("DTDataContainer::Save","Container is read only.");
        return;
    }
    
    DTDataContainerEntry entry;
    entry.type = 7;
    entry.usArray = A;
    
    content->accessLock.Lock();
    content->content[name] = entry;
    content->accessLock.Unlock();
}

void DTDataContainer::Save(const DTCharArray &A,const std::string &name)
{
    if (IsReadOnly()) {
        DTErrorMessage("DTDataContainer::Save","Container is read only.");
        return;
    }

    DTDataContainerEntry entry;
    entry.type = 8;
    entry.cArray = A;

    content->accessLock.Lock();
    content->content[name] = entry;
    content->accessLock.Unlock();
}

void DTDataContainer::Save(const DTUCharArray &A,const std::string &name)
{
    if (IsReadOnly()) {
        DTErrorMessage("DTDataContainer::Save","Container is read only.");
        return;
    }

    DTDataContainerEntry entry;
    entry.type = 9;
    entry.ucArray = A;

    content->accessLock.Lock();
    content->content[name] = entry;
    content->accessLock.Unlock();
}

void DTDataContainer::Save(const std::string &theString,const std::string &name)
{
    if (IsReadOnly()) {
        DTErrorMessage("DTDataContainer::Save","Container is read only.");
        return;
    }
    
    DTDataContainerEntry entry;
    entry.type = 1;
    entry.sEntry = theString;
    
    content->accessLock.Lock();
    content->content[name] = entry;
    content->accessLock.Unlock();
}

void DTDataContainer::AddEntry(DTDataContainerEntry entry,const std::string &name)
{
    content->content[name] = entry;
}

bool DTDataContainer::SavedAsCharacter(const std::string &name) const
{
    DTDataContainerEntry entry = FindVariable(name);
    return (entry.type==8 || entry.type==9);
}

bool DTDataContainer::SavedAsShort(const std::string &name) const
{
    DTDataContainerEntry entry = FindVariable(name);
    return (entry.type==6 || entry.type==7);
}

bool DTDataContainer::SavedAsInt(const std::string &name) const
{
    DTDataContainerEntry entry = FindVariable(name);
    return (entry.type==4);
}

bool DTDataContainer::SavedAsDouble(const std::string &name) const
{
    DTDataContainerEntry entry = FindVariable(name);
    return (entry.type==2);
}

bool DTDataContainer::SavedAsString(const std::string &name) const
{
    DTDataContainerEntry entry = FindVariable(name);
    return (entry.type==1);
}

void DTDataContainer::Flush(void) const
{
}

DTDoubleArray DTDataContainer::ReadDoubleArray(const std::string &name) const
{
    DTDoubleArray toReturn;
    content->accessLock.Lock();
    toReturn = ReadDoubleArrayInsideLock(name);
    content->accessLock.Unlock();
    return toReturn;    
}

DTDoubleArray DTDataContainer::ReadDoubleArrayInsideLock(const std::string &name) const
{
    DTDataContainerEntry entry = FindVariableInsideLock(name);
    if (entry.type==0) {
        std::string msg = string("Did not find the variable \"") + name + "\".";
        DTErrorMessage("DTDataContainer::ReadDoubleArray(name)",msg);
    }
    
    DTDoubleArray toReturn;
    switch (entry.type) {
        case 2:
            toReturn = entry.dArray;
            break;
        case 3:
            toReturn = ConvertToDouble(entry.fArray);
            break;
        case 4:
            toReturn = ConvertToDouble(entry.iArray);
            break;
        case 6:
            toReturn = ConvertToDouble(entry.sArray);
            break;
        case 7:
            toReturn = ConvertToDouble(entry.usArray);
            break;
        case 8:
            toReturn = ConvertToDouble(entry.cArray);
            break;
        case 9:
            toReturn = ConvertToDouble(entry.ucArray);
            break;
    }
    
    return toReturn;
}

DTFloatArray DTDataContainer::ReadFloatArray(const std::string &name) const
{
    content->accessLock.Lock();
    DTDataContainerEntry entry = FindVariableInsideLock(name);
    if (entry.type==0) {
        std::string msg = string("Did not find the variable \"") + name + "\".";
        DTErrorMessage("DTDataContainer::ReadFloatArray(name)",msg);
    }
    
    DTFloatArray toReturn;
    switch (entry.type) {
        case 2:
            toReturn = ConvertToFloat(entry.dArray);
            break;
        case 3:
            toReturn = entry.fArray;
            break;
        case 4:
            toReturn = ConvertToFloat(entry.iArray);
            break;
        case 6:
            toReturn = ConvertToFloat(entry.sArray);
            break;
        case 7:
            toReturn = ConvertToFloat(entry.usArray);
            break;
        case 8:
            toReturn = ConvertToFloat(entry.cArray);
            break;
        case 9:
            toReturn = ConvertToFloat(entry.cArray);
            break;
    }
    content->accessLock.Unlock();
    
    return toReturn;
}

DTIntArray DTDataContainer::ReadIntArray(const std::string &name) const
{
    content->accessLock.Lock();
    DTDataContainerEntry entry = FindVariableInsideLock(name);
    if (entry.type==0) {
        std::string msg = string("Did not find the variable \"") + name + "\".";
        DTErrorMessage("DTDataContainer::ReadIntArray(name)",msg);
    }
    
    DTIntArray toReturn;
    switch (entry.type) {
        case 2:
            toReturn = ConvertToInt(entry.dArray);
            break;
        case 3:
            toReturn = ConvertToInt(entry.fArray);
            break;
        case 4:
            toReturn = entry.iArray;
            break;
        case 6:
            toReturn = ConvertToInt(entry.sArray);
            break;
        case 7:
            toReturn = ConvertToInt(entry.usArray);
            break;
        case 8:
            toReturn = ConvertToInt(entry.cArray);
            break;
        case 9:
            toReturn = ConvertToInt(entry.ucArray);
            break;
    }
    content->accessLock.Unlock();
    
    return toReturn;
}

DTCharArray DTDataContainer::ReadCharArray(const std::string &name) const
{
    content->accessLock.Lock();
    DTDataContainerEntry entry = FindVariableInsideLock(name);
    if (entry.type==0) {
        std::string msg = string("Did not find the variable \"") + name + "\".";
        DTErrorMessage("DTDataContainer::ReadIntArray(name)",msg);
    }
    
    DTCharArray toReturn;
    switch (entry.type) {
        case 2:
            toReturn = ConvertToChar(entry.dArray);
            break;
        case 3:
            toReturn = ConvertToChar(entry.fArray);
            break;
        case 4:
            toReturn = ConvertToChar(entry.iArray);
            break;
        case 6:
            toReturn = ConvertToChar(entry.sArray);
            break;
        case 7:
            toReturn = ConvertToChar(entry.usArray);
            break;
        case 8:
            toReturn = entry.cArray;
            break;
        case 9:
            toReturn = ConvertToChar(entry.ucArray);
            break;
    }
    content->accessLock.Unlock();
    
    return toReturn;
}

DTUCharArray DTDataContainer::ReadUCharArray(const std::string &name) const
{
    content->accessLock.Lock();
    DTDataContainerEntry entry = FindVariableInsideLock(name);
    if (entry.type==0) {
        std::string msg = string("Did not find the variable \"") + name + "\".";
        DTErrorMessage("DTDataContainer::ReadIntArray(name)",msg);
    }
    
    DTUCharArray toReturn;
    switch (entry.type) {
        case 2:
            toReturn = ConvertToUChar(entry.dArray);
            break;
        case 3:
            toReturn = ConvertToUChar(entry.fArray);
            break;
        case 4:
            toReturn = ConvertToUChar(entry.iArray);
            break;
        case 6:
            toReturn = ConvertToUChar(entry.sArray);
            break;
        case 7:
            toReturn = ConvertToUChar(entry.usArray);
            break;
        case 8:
            toReturn = ConvertToUChar(entry.cArray);
            break;
        case 9:
            toReturn = entry.ucArray;
            break;
    }
    content->accessLock.Unlock();
    
    return toReturn;
}

DTShortIntArray DTDataContainer::ReadShortIntArray(const std::string &name) const
{
    content->accessLock.Lock();
    DTDataContainerEntry entry = FindVariableInsideLock(name);
    if (entry.type==0) {
        std::string msg = string("Did not find the variable \"") + name + "\".";
        DTErrorMessage("DTDataContainer::ReadShortIntArray(name)",msg);
    }
    
    DTShortIntArray toReturn;
    switch (entry.type) {
        case 2:
            toReturn = ConvertToShortInt(entry.dArray);
            break;
        case 3:
            toReturn = ConvertToShortInt(entry.fArray);
            break;
        case 4:
            toReturn = ConvertToShortInt(entry.iArray);
            break;
        case 6:
            toReturn = entry.sArray;
            break;
        case 7:
            toReturn = ConvertToShortInt(entry.usArray);
            break;
        case 8:
            toReturn = ConvertToShortInt(entry.cArray);
            break;
        case 9:
            toReturn = ConvertToShortInt(entry.ucArray);
            break;
    }
    content->accessLock.Unlock();
    
    return toReturn;
}

DTUShortIntArray DTDataContainer::ReadUShortIntArray(const std::string &name) const
{
    content->accessLock.Lock();
    DTDataContainerEntry entry = FindVariableInsideLock(name);
    if (entry.type==0) {
        std::string msg = string("Did not find the variable \"") + name + "\".";
        DTErrorMessage("DTDataContainer::ReadUShortIntArray(name)",msg);
    }
    
    DTUShortIntArray toReturn;
    switch (entry.type) {
        case 2:
            toReturn = ConvertToUShortInt(entry.dArray);
            break;
        case 3:
            toReturn = ConvertToUShortInt(entry.fArray);
            break;
        case 4:
            toReturn = ConvertToUShortInt(entry.iArray);
            break;
        case 6:
            toReturn = ConvertToUShortInt(entry.sArray);
            break;
        case 7:
            toReturn = entry.usArray;
            break;
        case 8:
            toReturn = ConvertToUShortInt(entry.cArray);
            break;
        case 9:
            toReturn = ConvertToUShortInt(entry.ucArray);
            break;
    }
    content->accessLock.Unlock();
    
    return toReturn;
}

double DTDataContainer::ReadNumber(const std::string &name) const
{
    double toReturn;
    content->accessLock.Lock();
    toReturn = ReadNumberInsideLock(name);
    content->accessLock.Unlock();
    return toReturn;
}

double DTDataContainer::ReadNumberInsideLock(const std::string &name) const
{
    DTDoubleArray theArr = ReadDoubleArrayInsideLock(name);
    if (theArr.IsEmpty() || theArr.Length()!=1)
        return 0.0;
    
    return theArr(0);
}

string DTDataContainer::ReadString(const std::string &name) const
{
    std::string toReturn;
    content->accessLock.Lock();
    toReturn = ReadStringInsideLock(name);
    content->accessLock.Unlock();
    return toReturn;
}

string DTDataContainer::ReadStringInsideLock(const std::string &name) const
{
    DTDataContainerEntry entry = FindVariableInsideLock(name);
    if (entry.type==0) {
        std::string msg = string("Did not find the variable \"") + name + "\".";
        DTErrorMessage("DTDataContainer::ReadString(name)",msg);
        return std::string();
    }
    
    std::string toReturn;
    
    if (entry.type==1) {
        toReturn = entry.sEntry;
    }
    else {
        std::string msg = string("The variable \"") + name + "\" has to be a string.";
        DTErrorMessage("DTDataContainer::ReadString(name)",msg);
    }
    
    return toReturn;
}
