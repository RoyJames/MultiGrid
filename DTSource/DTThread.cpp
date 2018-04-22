// Part of DTSource. Copyright 2005-2017. David Adalsteinsson.
// see https://www.visualdatatools.com/DTSource/license.html for more information.

#include "DTThread.h"
#include "DTError.h"

static void *DTThreadComm(void *);

void DTThreadStorageBase::Run(int v)
{
    if (started) {
        DTErrorMessage("DTThread::Run(int)","Already running");
        return;
    }
    value = v;
    started = true;
    pthread_create(&thread,NULL,DTThreadComm,(void *)this);
}

void DTThreadStorageBase::Wait(void)
{
    if (ended) {
        return;
    }
    
    pthread_join(thread,NULL);
    ended = true;
}

void *DTThreadComm(void *p)
{
    DTThreadStorageBase *ptr = (DTThreadStorageBase *)p;
    ptr->RunDerived(ptr->value);
    return NULL;
}
