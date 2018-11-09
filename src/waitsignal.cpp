#include "waitsignal.h"

sem_t waitsignal::gExitSem;
waitsignal::waitsignal()
{
    sem_init(&waitsignal::gExitSem, 0, 0);
	signal(SIGINT, waitsignal::CallBack);
    printf("wait signal ctrl-c.\r\n");
    sem_wait(&waitsignal::gExitSem);
}

waitsignal::~waitsignal()
{
	sem_destroy(&waitsignal::gExitSem);
}

void waitsignal::CallBack(int sig)
{
    printf(" >> recv signal.\r\n");
	sem_post(&gExitSem);
}
