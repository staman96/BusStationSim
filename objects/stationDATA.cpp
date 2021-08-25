#include <iostream>
#include <cstring>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdio.h>
#include <cstdlib>
#include <semaphore.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "stationDATA.h"

using std::rand;
using std::cout;
using std::endl;

int randgen(){
	int randint = rand();
	return randint;
};

int translateDest(char * dest){
	for(int i = 0; i < 3; i++){
		if (strcmp(dest, DEST[i]) == 0){
			return i;
		}
	}
	
};

void stationDATA::InitBusStation(void * ptr, int * BPerD, int * BCapPerD){

	/* - init variables - */

	((stationDATA*)ptr)->notServed = 0;
	((stationDATA*)ptr)->INqueueSize = 0;
	((stationDATA*)ptr)->OUTqueueSize = 0;
	((stationDATA*)ptr)->totalBays = 0;
	for (int i = 0; i < 3; i++){
		((stationDATA*)ptr)->baysPerDest[i] = BPerD[i];
		((stationDATA*)ptr)->totalBays += BPerD[i];
	}

	/* - init OffSets - */

	((stationDATA*)ptr)->baysOffSet = ST_SIZE;
	((stationDATA*)ptr)->busesOfBaysOffSet = ((stationDATA*)ptr)->baysOffSet + BAY_SIZE*totalBays;

	/* - init objects - */

	void * pointer = ptr + ST_SIZE;
	int bayCounter = 0,	busCounter = 0, c = 0;
		/*loop for each type of bay*/
	for (int i = 0; i < 3; i++){
		/*loop for each bay of a type*/
		for (int B = 0; B < BPerD[i]; B++){
			size_t bayoffset = bayCounter*BAY_SIZE;

			((Bay*)(pointer + bayoffset))->occupied = 0;
			((Bay*)(pointer + bayoffset))->destination = i;
			((Bay*)(pointer + bayoffset))->size = BCapPerD[i];
			int busesOffSet = ((stationDATA*)ptr)->busesOfBaysOffSet + busCounter*BUS_SIZE;
			((Bay*)(pointer + bayoffset))->busesOffSet = busesOffSet;

			busCounter += BCapPerD[i];
			bayCounter++;
		}
	}

	/*parking slots initiation*/
	for(int i = 0; i < ((stationDATA*)ptr)->totalBays; i++){
		Bay * bay = ((stationDATA*)ptr)->getBay(ptr, i);
		for (int j = 0; j < bay->size; j++){
			Bus * bus = ((stationDATA*)ptr)->getBayBus(ptr,i,j);
			bus->checked = true;
			bus->state = -1;
		}
	}
	


	/* - init semaphores - */

	if( sem_init(&((stationDATA*)ptr)->inDriveway, 1, 1) != 0){
		cout << "Error initializing semaphore!" << endl;
	}
	if( sem_init(&((stationDATA*)ptr)->outDriveway, 1, 1) != 0){
		cout << "Error initializing semaphore!" << endl;
	}
	if( sem_init(&((stationDATA*)ptr)->inBoundQueue, 1, 0) != 0){
		cout << "Error initializing semaphore!" << endl;
	}
	if( sem_init(&((stationDATA*)ptr)->OutGoingComm, 1, 0) != 0){
		cout << "Error initializing semaphore!" << endl;
	}
	if( sem_init(&((stationDATA*)ptr)->outGoingQueue, 1, 0) != 0){
		cout << "Error initializing semaphore!" << endl;
	}
	if( sem_init(&((stationDATA*)ptr)->CommBusWrote, 1, 0) != 0){
		cout << "Error initializing semaphore!" << endl;
	}
	if( sem_init(&((stationDATA*)ptr)->CommStMngrWrote, 1, 0) != 0){
		cout << "Error initializing semaphore!" << endl;
	}
	if( sem_init(&((stationDATA*)ptr)->bayMutex, 1, 1) != 0){
		cout << "Error initializing semaphore!" << endl;
	}
	if( sem_init(&((stationDATA*)ptr)->inBoundMutex, 1, 1) != 0){
		cout << "Error initializing semaphore!" << endl;
	}
	if( sem_init(&((stationDATA*)ptr)->outGoingMutex, 1, 1) != 0){
		cout << "Error initializing semaphore!" << endl;
	}
	if( sem_init(&((stationDATA*)ptr)->coutMutex, 1, 1) != 0){
		cout << "Error initializing semaphore!" << endl;
	}


};

Bay * stationDATA::getBay(void * ptr, int index){
	return (Bay*)(ptr + ((stationDATA*)ptr)->baysOffSet + index*BAY_SIZE);
};

/**/
int stationDATA::getBaysPerDest(int dest,int & startingIndex){
	startingIndex = 0;
	int endingIndex = baysPerDest[0];
	for (int i = 0; i < dest; i++){
		startingIndex += baysPerDest[i];
		endingIndex += baysPerDest[i+1];
	}
	
	return endingIndex;
};

/*get parkingslot/bus of specific bay*/
Bus * stationDATA::getBayBus(void * ptr, int BayIdx, int busIdx){
	return (Bus*)(ptr + getBay(ptr, BayIdx)->busesOffSet + busIdx*BUS_SIZE);
};

void stationDATA::closeBusStation(void * ptr){
	/*destroy sems*/
	if( sem_destroy(&((stationDATA*)ptr)->inDriveway) != 0){
		cout << "Error destroying semaphore!" << endl;
	}
	if( sem_destroy(&((stationDATA*)ptr)->outDriveway) != 0){
		cout << "Error destroying semaphore!" << endl;
	}
	if( sem_destroy(&((stationDATA*)ptr)->inBoundQueue) != 0){
		cout << "Error destroying semaphore!" << endl;
	}
	if( sem_destroy(&((stationDATA*)ptr)->OutGoingComm) != 0){
		cout << "Error destroying semaphore!" << endl;
	}
	if( sem_destroy(&((stationDATA*)ptr)->outGoingQueue) != 0){
		cout << "Error destroying semaphore!" << endl;
	}
	if( sem_destroy(&((stationDATA*)ptr)->CommBusWrote) != 0){
		cout << "Error destroying semaphore!" << endl;
	}
	if( sem_destroy(&((stationDATA*)ptr)->CommStMngrWrote) != 0){
		cout << "Error destroying semaphore!" << endl;
	}
	if( sem_destroy(&((stationDATA*)ptr)->bayMutex) != 0){
		cout << "Error destroying semaphore!" << endl;
	}
	if( sem_destroy(&((stationDATA*)ptr)->inBoundMutex) != 0){
		cout << "Error destroying semaphore!" << endl;
	}
	if( sem_destroy(&((stationDATA*)ptr)->outGoingMutex) != 0){
		cout << "Error destroying semaphore!" << endl;
	}
	if( sem_destroy(&((stationDATA*)ptr)->coutMutex) != 0){
		cout << "Error destroying semaphore!" << endl;
	}
};
