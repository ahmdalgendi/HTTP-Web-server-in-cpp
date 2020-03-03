
#ifndef THREAD_HPP
#define THREAD_HPP

#include <stdio.h>
#include <stdlib.h>
#include <process.h>

#define	STKSIZE	 16536
class Thread {
public:

	Thread()
	{}
	virtual ~Thread()
	{}

	static void * pthread_callback(void * ptrThis);

	virtual void run() = 0;
	void  start();
};
#endif
