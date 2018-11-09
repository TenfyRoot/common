#ifndef WAITSIGNAL_H
#define WAITSIGNAL_H

#include <semaphore.h>
#include <signal.h>
#include <stdio.h>

class waitsignal
{
public:
    waitsignal();
    ~waitsignal();

   
private:
    static void CallBack(int sig);
    static sem_t gExitSem;
};
#endif
