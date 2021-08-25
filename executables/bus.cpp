#include <iostream>
#include <cstring>
#include <stdio.h>
#include <cstdlib>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ipc.h> 
#include <sys/shm.h> 
#include <signal.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <time.h>
#include "../objects/busDATA.h"
#include "../objects/stationDATA.h"

using std::cout;
using std::endl;

int main(int argc, char * argv[]){
    srand(time(NULL));
    int shmfd, shmSize, eRR;
    signal(SIGINT, SIG_IGN);
    int counter = 0;
    double inqueue, maneuvering, parking, outqueue, exiting, passPerC, sleeptime;
    clock_t start, end;

    int bayToPark = -1, slotToPark;
    Bus * parkSlot;

    busDATA * mydata = new busDATA(getpid());

    int i = 1;
    while(i<argc){
        if(strcmp(argv[i],"-t") == 0){
            //type
            i++;
            mydata->setType(translateDest(argv[i]));
        }
        else if(strcmp(argv[i],"-n") == 0){
           //current passengers
            i++;
            mydata->setCarryingPassengers(atoi(argv[i]));
        }
        else if(strcmp(argv[i],"-c") == 0){
            //max capacity
            i++;
            mydata->setCapacity(atoi(argv[i]));
        }
        else if (strcmp(argv[i],"-p") == 0) {
            //parking period
            i++;
            mydata->setParkPeriod(atoi(argv[i]));
        }
        else if (strcmp(argv[i], "-m") == 0) {
            //maneuver time
            i++;
            mydata->setManeuverTime(atoi(argv[i]));
        }
        else if (strcmp(argv[i],"-s") == 0) {
            //shmSize
            i++;
            shmSize = atoi(argv[i]);
        }
        else{
            cout << getpid() << " || ";
            cout << "Invalid parameter" << endl;
        }

        i++;
    }
    
    /**** get shared mem ****/

    /* shm_open returns the shmfd */
    if((shmfd = shm_open(sharedMemName, O_RDWR, 0666)) == -1){
        cout << "Getting shared memory failed" << endl;
        delete mydata;
        exit(1);
    }
    stationDATA * sharedData = (stationDATA *)mmap(0, shmSize, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);
    if(sharedData == MAP_FAILED){
        cout << "MAP_FAILED" << endl;
        eRR = close(shmfd);
        delete mydata;
        exit(2);
    }

    eRR = sem_wait(&sharedData->coutMutex);    /*lock cout mutex*/
    if(eRR != 0) cout << "Sem wait ERROR " << counter << endl;
    counter++;
    // cout << getpid() << " || " << ++counter << endl;

    cout << getpid() << " || "
        << DEST[mydata->getType()] << "    "
        << "Occupancy: " << mydata->getCurrentPassengers() << "/"
        << mydata->getCapacity() << "   "
        << "Park: " << mydata->getParkPeriod() << "s "
        << "Maneuver: " << mydata->getManeuverTime() << "s   "
        << "shmSize: " << shmSize << " bytes" << endl;

    eRR = sem_post(&sharedData->coutMutex);    /*unlock cout mutex*/
    if(eRR != 0) cout << "Sem post ERROR" << endl;

    /********  BUS  FUNCTIONALITY  *********/

    /* Arrived To Station */

    /*time of arrived*/
    time_t arrived;
    time(&arrived);

    /*wait to queue, and inform about bus existence in queue*/
    eRR = sem_wait(&sharedData->inBoundMutex);    /*lock inbound mutex*/
    if(eRR != 0) cout << "Sem wait ERROR " << counter << endl;
    counter++;
    // cout << getpid() << " || " << ++counter << endl;

    start = clock();

    sharedData->INqueueSize++;/*update size of incoming buses's queue*/

    eRR = sem_post(&sharedData->inBoundMutex);    /*unlock inbound mutex*/
    if(eRR != 0) cout << "Sem post ERROR" << endl;

    eRR = sem_wait(&sharedData->inBoundQueue);    /*lock inbound queue, wait in queue*/
    if(eRR != 0) cout << "Sem wait ERROR " << counter << endl;
    counter++;
    // cout << getpid() << " || " << (++counter) << endl;

    /*when unblocked from queue start communication with stmngr, so change protocol vars*/
    eRR = sem_wait(&sharedData->inBoundMutex);    /*lock inbound mutex*/
    if(eRR != 0) cout << "Sem wait ERROR " << counter << endl;
    counter++;
    // cout << getpid() << " || " << ++counter << endl;

    // cout << getpid() << " || " << "Writing my id and dest to protocol" << endl;
    sharedData->ID = getpid();
    sharedData->dest = mydata->getType();
    sharedData->passengersIN = mydata->getCurrentPassengers();

    eRR = sem_post(&sharedData->inBoundMutex);    /*unlock inbound mutex*/
    if(eRR != 0) cout << "Sem post ERROR" << endl;

    /*when finish writing*/
    eRR = sem_post(&sharedData->CommBusWrote);    /*unlock comm sem, so that stmngr can continue*/
    if(eRR != 0) cout << "Sem post ERROR" << endl;
    eRR = sem_wait(&sharedData->CommStMngrWrote);    /*lock comm sem, wait for stmngr to read and reply*/
    if(eRR != 0) cout << "Sem wait ERROR " << counter << endl;
    counter++;
    // cout << getpid() << " || " << ++counter << endl;

    /*when unblocked from Comm get data that will set how the bus continues*/
    eRR = sem_wait(&sharedData->inBoundMutex);    /*lock inbound mutex*/
    if(eRR != 0) cout << "Sem wait ERROR " << counter << endl;
    counter++;
    // cout << getpid() << " || " << ++counter << endl;

    bayToPark = sharedData->INbayIndex;
    slotToPark = sharedData->INparkingIndex;

    eRR = sem_post(&sharedData->inBoundMutex);    /*unlock inbound mutex*/
    if(eRR != 0) cout << "Sem post ERROR" << endl;

    /*if bus station closes leave*/
    if (bayToPark == -1){
        eRR = sem_post(&sharedData->CommBusWrote);    /*unlock comm sem, so that stmngr can continue*/
        if(eRR != 0) cout << "Sem post ERROR" << endl;

        eRR = sem_wait(&sharedData->coutMutex);    /*lock cout mutex*/
        if(eRR != 0) cout << "Sem wait ERROR " << counter << endl;
        counter++;
        // cout << getpid() << " || " << ++counter << endl;

        cout << getpid() << " || " << "Station closes so i'm leaving" << endl;

        eRR = sem_post(&sharedData->coutMutex);    /*unlock cout mutex*/
        if(eRR != 0) cout << "Sem post ERROR" << endl;

        goto end;
    }

    /*Continue by acquring the road that goes to bay*/
    time_t entered;
    time(&entered);
    end = clock();
    inqueue = double(end - start) / double(CLOCKS_PER_SEC);
    start = end;

    /*write data to shared mem*//*i lock the mutex here so that comptroller can read correct bus data*/
    eRR = sem_wait(&sharedData->bayMutex);    /*lock bay mutex*/
    if(eRR != 0) cout << "Sem wait ERROR " << counter << endl;
    counter++;
    // cout << getpid() << " || " << ++counter << endl;

    eRR = sem_wait(&sharedData->inDriveway);    /*lock inbound Driveway and go to bay*/
    if(eRR != 0) cout << "Sem wait ERROR " << counter << endl;
    counter++;
    // cout << getpid() << " || " << ++counter << endl;

    eRR = sem_post(&sharedData->CommBusWrote);    /*unlock comm sem, so that stmngr can continue*/
    if(eRR != 0) cout << "Sem post ERROR" << endl;

    parkSlot = sharedData->getBayBus(sharedData, bayToPark, slotToPark);

    parkSlot->arrived = arrived;
    parkSlot->entered = entered;
    parkSlot->ID = mydata->getBusID();
    parkSlot->dest = mydata->getType();
    parkSlot->state = ENTERING;
    parkSlot->inqueue = inqueue;
    parkSlot->checked = false;

    eRR = sem_post(&sharedData->bayMutex);    /*unlock bay mutex*/
    if(eRR != 0) cout << "Sem post ERROR" << endl;

    /*sleep for maneuver time*/
    sleep(mydata->getManeuverTime());

    /*unlock inboundDriveway sem, so that another bus can enter the station*/
    eRR = sem_post(&sharedData->inDriveway);
    if(eRR != 0) cout << "Sem post ERROR" << endl;

    /*Arrived to parking spot*/
    time_t parked;
    time(&parked);
    end = clock();
    maneuvering = double(end - start) / double(CLOCKS_PER_SEC);
    start = end;

    /*update parked data to shared mem, dissembark passengers*/
    eRR = sem_wait(&sharedData->bayMutex);    /*lock bay mutex*/
    if(eRR != 0) cout << "Sem wait ERROR " << counter << endl;
    counter++;
    // cout << getpid() << " || " << ++counter << endl;

    parkSlot->state = PARKED;
    parkSlot->parked = parked;
    parkSlot->passengersCame = mydata->getCurrentPassengers();
    parkSlot->capacity = mydata->getCapacity();
    parkSlot->maneuvering = maneuvering;

    mydata->setCarryingPassengers(randgen()%mydata->getCapacity() + 1);/*get passengers*/
    parkSlot->passengersLeft = mydata->getCurrentPassengers();

    eRR = sem_post(&sharedData->bayMutex);    /*unlock bay mutex*/
    if(eRR != 0) cout << "Sem post ERROR" << endl;

    /*sleep park time, time depends on amount of passengers came and left*/
    passPerC = (double)parkSlot->passengersCame / (double)parkSlot->capacity;
    sleeptime = (double)mydata->getParkPeriod() * passPerC;
    sleep((int)sleeptime);

    passPerC = (double)parkSlot->passengersLeft / (double)parkSlot->capacity;
    sleeptime = (double)mydata->getParkPeriod() * passPerC;
    sleep((int)sleeptime);


    /* Leaving Station */

    eRR = sem_wait(&sharedData->outGoingMutex);    /*lock OutGoing mutex*/
    if(eRR != 0) cout << "Sem wait ERROR " << counter << endl;
    counter++;
    // cout << getpid() << " || " << ++counter << endl;

    end = clock();
    parking = double(end - start) / double(CLOCKS_PER_SEC);
    start = end;

    sharedData->OUTqueueSize++;

    eRR = sem_post(&sharedData->outGoingMutex);    /*unlock inbound mutex*/
    if(eRR != 0) cout << "Sem post ERROR" << endl;

    eRR = sem_wait(&sharedData->outGoingQueue);    /*lock and wait to OutGoing queue*/
    if(eRR != 0) cout << "Sem wait ERROR " << counter << endl;
    counter++;
    // cout << getpid() << " || " << ++counter << endl;

    /*write data and acquire road that leads to station's exit*/
    eRR = sem_wait(&sharedData->outGoingMutex);    /*lock outgoing mutex*/
    if(eRR != 0) cout << "Sem wait ERROR " << counter << endl;
    counter++;
    // cout << getpid() << " || " << ++counter << endl;

    sharedData->OUTbayIndex = bayToPark;
    sharedData->OUTparkingIndex = slotToPark;

    eRR = sem_post(&sharedData->outGoingMutex);    /*unlock outgoing mutex*/
    if(eRR != 0) cout << "Sem post ERROR" << endl;

    time_t departed;
    time(&departed);
    end = clock();
    outqueue = double(end - start) / double(CLOCKS_PER_SEC);
    parking += outqueue;/*bus was parked while while waiting in queue to leave*/
    start = end;
    
    eRR = sem_wait(&sharedData->outDriveway);    /*lock outgoing Driveway and head to exit(leave bay)*/
    if(eRR != 0) cout << "Sem wait ERROR " << counter << endl;
    counter++;
    // cout << getpid() << " || " << ++counter << endl;

    /*let stmngr read data written and continue*/
    eRR = sem_post(&sharedData->OutGoingComm);    /*unlock outgoingCOmm */
    if(eRR != 0) cout << "Sem post ERROR" << endl;

    /*update to leaving status*/
    eRR = sem_wait(&sharedData->bayMutex);    /*lock bay mutex*/
    if(eRR != 0) cout << "Sem wait ERROR " << counter << endl;
    counter++;
    // cout << getpid() << " || " << ++counter << endl;

    parkSlot->state = LEAVING;
    parkSlot->departed = departed;
    parkSlot->parking = parking;
    parkSlot->outqueue = outqueue;

    eRR = sem_post(&sharedData->bayMutex);    /*unlock bay mutex*/
    if(eRR != 0) cout << "Sem post ERROR" << endl;

    sleep(mydata->getManeuverTime());

    eRR = sem_post(&sharedData->outDriveway);    /*unlock outgoing Driveway and exit station*/
    if(eRR != 0) cout << "Sem post ERROR" << endl;

    time_t exited;
    time(&exited);
    end = clock();
    exiting = double(end - start) / double(CLOCKS_PER_SEC);
    start = end;

    /*update to exit status*/
    eRR = sem_wait(&sharedData->bayMutex);    /*lock bay mutex*/
    if(eRR != 0) cout << "Sem wait ERROR " << counter << endl;
    counter++;
    // cout << getpid() << " || " << ++counter << endl;

    parkSlot->exited = exited;
    parkSlot->exiting = exiting;
    parkSlot->state = LEFT;

    eRR = sem_post(&sharedData->bayMutex);    /*unlock bay mutex*/
    if(eRR != 0) cout << "Sem post ERROR" << endl;

    eRR = sem_wait(&sharedData->coutMutex);    /*lock cout mutex*/
    if(eRR != 0) cout << "Sem wait ERROR " << counter << endl;
    counter++;
    // cout << getpid() << " || " << ++counter << endl;

    cout << getpid() << " || " << "Bus left" << endl;
    cout.flush();

    eRR = sem_post(&sharedData->coutMutex);    /*unlock cout mutex*/
    if(eRR != 0) cout << "Sem post ERROR" << endl;
    
    
    end:

    /*free shared memory*/
    eRR = munmap(sharedData, shmSize);
    if (eRR == -1){
        cout << "munmap ERROR" << endl;
    }
    eRR = close(shmfd);
    if (eRR == -1){
        cout << "closing shmfd ERROR" << endl;
    }

    delete mydata;
}
