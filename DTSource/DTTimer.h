// Part of DTSource. Copyright 2006. David Adalsteinsson.
// see https://www.visualdatatools.com/DTSource/license.html for more information.

#ifndef DTTimer_H
#define DTTimer_H

#include <sys/time.h>

class DTTimer
{
public:
    DTTimer() : timerStarted(false), timerStopped(false) {}
    void Start(void) {gettimeofday(&startTime,NULL); timerStarted = true; timerStopped = false;}
    void Restart(void) {timerStarted = true; timerStopped = false; gettimeofday(&startTime,NULL);}
    double Stop(void) {
        if (!timerStopped) gettimeofday(&stopTime,NULL);
        timerStopped = true;
        return ((stopTime.tv_sec-startTime.tv_sec)+(stopTime.tv_usec-startTime.tv_usec)*1e-6);
    }
    
    double Time(void) {
        if (timerStarted && timerStopped) {
            return ((stopTime.tv_sec-startTime.tv_sec)+(stopTime.tv_usec-startTime.tv_usec)*1e-6);
        }
        else {
            return -1.0;
        }
    }
        
private:
    timeval startTime,stopTime;
    bool timerStarted,timerStopped;
};
    
#endif
