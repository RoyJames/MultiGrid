// Part of DTSource. Copyright 2005-2017. David Adalsteinsson.
// see https://www.visualdatatools.com/DTSource/license.html for more information.

#ifndef DTThread_H
#define DTThread_H

// This is under active development right now.  Might change drastically.


// Use:
// Need to set up a storage class that contains a "Run" member function.
// Example
// struct PartOfJob {
//     DTDoubleArray bigData;
//     int howManyThreads;
//     DTMutableDoubleArray returnValue;
//     void Run(int); // Performs the calculation
// };

// #include <unistd.h>
// int numCPU = sysconf( _SC_NPROCESSORS_ONLN );
//
// DTMutableList<DTThread<PartOfJob> > listOfMyJobs(numCPU); // Note the space needed between "> >"
// bigJob.howManyThreads = numCPU;
// for (i=0;i<numCPU;i++) listOfMyJobs(i) = DTThread<PartOfJob>(bigJob);
// for (i=0;i<numCPU;i++) listOfMyJobs(i).Run(i);
//
// ... very soon, all numCPU jobs will be running in other threads.
// 
// for (i=0;i<numCPU;i++) listOfMyJobs(i).Wait();
// Now all jobs have finished.
// 
// The last two loops can also be replaced by
// RunAllThreads(listOfMyJobs); // This will wait launch them and then wait for all of them.
// Note that this is not a que, since all of the threads will run at the same time.

// Once the thread starts, it will call the run member function to perform the calculation.

#include "DTPointer.h"
#include "DTList.h"

#include <pthread.h>
#include <iostream>

struct DTThreadStorageBase {
    DTThreadStorageBase() : started(false), ended(false), value(0) {}
    virtual ~DTThreadStorageBase() {Wait();}
    
    void Run(int);
    void Wait(void);

    virtual void RunDerived(int) = 0;

    bool started;
    bool ended;
    int value;
    pthread_t thread;
};

template<class T> struct DTThreadStorage : public DTThreadStorageBase
{
    DTThreadStorage(const T &V) : storage(V) {}
    
    void RunDerived(int v) {storage.Run(v);}
    bool Running(void) const {return storage->started;}
    
    T storage;
};


template<class T> class DTThread {
public:
    DTThread() {} // NULL pointer.
    DTThread(const T &V) : storage(new DTThreadStorage<T>(V)) {}
    
    // Launch the thread
    void Run(int v) {if (storage) {storage->Run(v);}}
    void Wait(void) {if (storage) {storage->Wait();}}
    
    T Storage(void) {return storage->storage;}
    
private:
    DTPointer<DTThreadStorage<T> > storage;
};

template <class T> void RunAllThreads(const DTMutableList<DTThread<T> > &A)
{
    int len = A.Length();
    for (int i=0;i<len;i++) A(i).Run(i);
    for (int i=0;i<len;i++) A(i).Wait();
}


#endif
