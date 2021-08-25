#ifndef stationDATA_h
#define stationDATA_h

/*destinations*/
#define ASK 0
#define PEL 1
#define VOR 2

/*bus states*/
#define ENTERING 3
#define PARKED 2
#define LEAVING 1
#define LEFT 0

#define ST_SIZE sizeof(stationDATA)
#define BAY_SIZE sizeof(Bay)
#define BUS_SIZE sizeof(Bus)

#include <sys/types.h>
#include <semaphore.h>

static const char * sharedMemName = "OS3";
static const char * DEST[3] = {"ASK", "PEL", "VOR"};
static const char * logFile = "IO/log.txt";

int randgen();
int translateDest(char * dest);

class Bus{/*parking slot, has bus info*/
    public:
        pid_t ID;
        int passengersCame, passengersLeft, capacity, dest,
        state; /*3: entering, 2: parked, 1: leaving,
                *0: left, -1: not in use yet*/
        bool checked;/*if comptroller has already checked the Bus*/
        time_t arrived, entered, parked, departed, exited;
        double inqueue, maneuvering, parking, outqueue, exiting;
};

class Bay{
    public:
        size_t busesOffSet;/*from shm's starting address*/
        int size;/*const*/
        int occupied;/*variable*/
        int destination;/*const*/

        /*objects*/
        // Bus * parkingSlots; /*array of bus type parking slots*/
};

class stationDATA{
    public:
        /* variables */
        int baysPerDest[3], totalBays, 
            notServed;/*buses that didn't enter the station because it was closed*/
        pid_t comptroller;/*comptroller pid*/

        /****************comm protocol between incoming bus and stmngr*************/
        int INqueueSize;
        /*variables set by 1st bus inline for stmngr*/
        pid_t ID;
        int dest, passengersIN;
        sem_t CommBusWrote, CommStMngrWrote; /*sems that are used for communication,
        once bus is awaken from inBoundQueue starts writitng to protocol,
        and stmngr waits at commBusWrote.(init to 0) Whenever each process has written
        something posts the equivelant sem, and the other process waits on it.
        After write, bus posts CommBusWrote, and waits to CommStMngrWrote
        until stmngr have written data in shared mem. Afterwards, stmngr posts 
        CommStMngrWrote so that bus can read data, and waits to CommBusWrote
        until bus have read the data.*/

        /*variables for 1st bus in queue, set by stmngr*/
        int INbayIndex,/*is -1 if bus has to leave because station is closing*/
            INparkingIndex;/*index in bus array*/
        /*****************************************************************/

        /* outgoing variables, 
        ** bayIdx is the bus's bay index that was parked */
        int OUTqueueSize, OUTbayIndex, OUTparkingIndex;
        sem_t OutGoingComm;

        /* Offsets */
        size_t baysOffSet, /*offset to mem address of 1st bay*/
            busesOfBaysOffSet; /*offset to mem address of 1st 
                                bus(pid_t) of 1st bay*/
 

        /* "objects" */
        // Bay * bays;  /*array of bays with length of totalBays*/

        /* semaphores */
        sem_t inDriveway, /*sem to represent road to parking(maneuver)*/
            outDriveway,    /*sem to represent road to exit the station*/
            inBoundQueue,   /*sem to represent queue to enter station*/
            outGoingQueue,  /*sem to represent queue to leave station*/
            inBoundMutex,   /*sem is used by buses and stmngr for all inbound traffic*/
            outGoingMutex,
            bayMutex, 
            coutMutex;

        /* *** FUNCTIONS --> API *** */


        void InitBusStation(void * ptr, int * BPerD, int * BCapPerD);
        /*get specific bay*/
        Bay * getBay(void * ptr, int index);
        /*get the amount of bays per destination*/
        int getBaysPerDest(int dest,int & startingIndex);
        /*get parkingslot/bus of specific bay*/
        Bus * getBayBus(void * ptr, int BayIdx, int busIdx);

        void closeBusStation(void * ptr);
};

/******************************************************************
 * MEMORY STRUCTURE
 * ****************************************************************
 * size of stationDATA
 * totalbays * size of Bay(baysOffSet)
 * total parking slots of each bay * size of Bus(busesOfBaysOffSet)
 * 
*/

#endif