// Part of DTSource. Copyright 2004-2015. David A. David Adalsteinsson.
// see http://www.visualdatatools.com/DTSource/license.html for more information.

#ifndef DTLiveUpdate_Header
#define DTLiveUpdate_Header


// First come bindings that are C++ only.  Further down is an interface that can be called from both
// C++ and C.

#ifdef __cplusplus
    
class DTDataContainer;

#include <string>
using namespace std;

// Starts up the socket
extern bool DTLivePublish(string);
extern bool DTLivePublish(int,string responseToDT,string requiredAnswer);

// Before each type can be saved
extern void DTLiveAddVariable(string varName,string varType);

// Data where you save the entry, using Write(...) with this as the first argument.
extern DTDataContainer DTLiveDataObject();

// Checking connecting status.
extern bool DTLiveCurrentlySendingOver(string varName);
extern bool DTLiveCurrentlyConnected();

// To sync up any changes.
extern void DTLiveSynchronize(); // Let DataTank know of any changes.


#endif


//C bindings. These are wrapper functions for the C++ functions above, but stay away from
// the string variable and DTSource classes.
#ifdef __cplusplus
extern "C" {
#endif
    
    int DTCLivePublishLocal(char *);
    int DTCLivePublishRemote(int portNumber,char *responseToDT,char *requiredAnswer);
    
    void DTCLiveAddVariable(char *varName,char *varType);
    
    int DTCLiveCurrentlySendingOver(char *varName);
    int DTCLiveCurrentlyConnected(void);
    
    void DTCLiveSynchronize();
    
    
#ifdef __cplusplus
}
#endif

#endif
