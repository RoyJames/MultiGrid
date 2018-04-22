// Part of DTSource. Copyright 2004-2017. David Adalsteinsson.
// see https://www.visualdatatools.com/DTSource/license.html for more information.

#include "DTLock.h"

// Template to support thread safe access to a value.  The main use
// it for on/off flags etc.

template <class T>
class DTLocked {
public:
    DTLocked() {}
    DTLocked(const DTLocked<T> &A) {A.lock.Lock(); value = A.value; A.lock.Unlock();}
    
    DTLocked<T> &operator=(const DTLocked<T> &A) {
        if (lock==A.lock) return *this;
        lock.Lock();
        A.lock.Lock();
        value = A.value;
        A.lock.Unlock();
        lock.Unlock();
        return *this;
    }
    
    DTLocked<T> &operator=(const T &A) {
        lock.Lock();
        value = A;
        lock.Unlock();
        return *this;
    }
    
    T operator--(int) {
        lock.Lock();
        T toReturn = --value;
        lock.Unlock();
        return toReturn;
    }
    
    T operator++(int) {
        lock.Lock();
        T toReturn = ++value;
        lock.Unlock();
        return toReturn;
    }
    
    operator T(void) const {lock.Lock(); T toReturn(value); lock.Unlock(); return toReturn;}
    
private:
    DTLock lock;
    T value;
};
