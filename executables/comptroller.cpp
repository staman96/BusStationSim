#include <iostream>
#include <cstring>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <cstdlib>
#include <sys/ipc.h> 
#include <sys/shm.h> 
#include <sys/mman.h>
#include <fcntl.h>
#include <signal.h>
#include <ctime>
#include "../objects/stationDATA.h"
#include "../objects/ledger.h"

using std::cout;
using std::endl;

void termHandler(int sig);
static bool terminate;
int calcSleepDuration(int interval, int & snapTimeLeft, int statTime, int & statTimeLeft, char & function);
void printStats(stationDATA * sharedData, Ledger * ledger, struct tm * NOW);

int main(int argc, char * argv[]){
    terminate = false;
    signal(SIGINT, SIG_IGN);
    signal(SIGUSR1, termHandler);
    int shmSize, shmfd, interval = 10, statTime = 30, eRR;
    Ledger * ledger = new Ledger();

    int i = 1;
    while(i<argc){
        if(strcmp(argv[i],"-d") == 0){
            //interval
            i++;
            interval = atoi(argv[i]);
        }
        else if(strcmp(argv[i],"-t") == 0){
           //stat times
            i++;
            statTime = (atoi(argv[i]));
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

    cout << getpid() << " || " << "Comptroller snapshot interval is " << interval
        << " secs, statistic is " << statTime << " secs and shm size is " << shmSize << endl;

    /* shm_open returns the shmfd */
    if((shmfd = shm_open(sharedMemName, O_RDWR, 0666)) == -1){
        cout << "Getting shared memory failed" << endl;
    }
    else{
        cout << "Comptroller got the shared mem" << endl;
    }

    stationDATA * sharedData;
    sharedData = (stationDATA *)mmap(0, shmSize, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);

    sharedData->comptroller = getpid();

    /*variables*/
    int statTimeLeft = statTime, snapTimeLeft = interval; 

    while (!terminate){
        char function;
        int counter = 0;
        sleep(calcSleepDuration(interval, snapTimeLeft, statTime, statTimeLeft, function));

        /*sem wait all sems, to take snapshot of shared memory every other process altering it
        has to block*/
        eRR = sem_wait(&sharedData->inBoundMutex);    /*lock inbound mutex*/
        if(eRR != 0) cout << "COMPTROLLER || " << "Sem wait ERROR " << counter << endl;
        counter++;
        // cout << counter << endl;
        eRR = sem_wait(&sharedData->outGoingMutex);    /*lock outGoing mutex*/
        if(eRR != 0) cout << "COMPTROLLER || " << "Sem wait ERROR " << counter << endl;
        counter++;
        // cout << counter << endl;
        eRR = sem_wait(&sharedData->bayMutex);    /*lock bay mutex*/
        if(eRR != 0) cout << "COMPTROLLER || " << "Sem wait ERROR " << counter << endl;
        counter++;
        // cout << counter << endl;
        eRR = sem_wait(&sharedData->coutMutex);    /*lock cout mutex*/
        if(eRR != 0) cout << "COMPTROLLER || " << "Sem wait ERROR " << counter << endl;
        counter++;
        // cout << counter << endl;

        /*cycle variables*/
        time_t now;
        struct tm * NOW;
        time(&now);
        NOW = localtime(&now);
        Bus * bus;
        Bay * bay;

        if(function == 'P' || function == 'B'){
            /*print snapshot (P:PHOTO)*/
            
            cout << endl << endl << ">>>>>>>>>>>>>>>>>>>>>  STATION STATUS at " << asctime(NOW) << endl;

            /*QUEUE to enter station status*/
            cout << sharedData->INqueueSize << " buses are waiting to be served from station manager." << endl << endl;
        
            /*in driveway status*/
            int semval;
            if(sem_getvalue(&sharedData->inDriveway,&semval) != 0){
                cout << "Error getting semaphore value" << endl;
            }
            if(semval == 0) cout << "Bus " << sharedData->ID
                << " entered station from " << DEST[sharedData->dest] 
                << " carrying " << sharedData->passengersIN
                << " passengers." << endl;
            else cout << "No bus is entering the station now." << endl;
            cout << endl;

            /*printing bays status*/
            int passengersCame[3] = {0,0,0};
            for(int B = 0; B < sharedData->totalBays; B++){
                bay = sharedData->getBay(sharedData, B);
                cout << "BAY No " << B
                << " to " << DEST[bay->destination] 
                << " Occupancy: " << bay->occupied << "/" << bay->size << endl;

                for (int b = 0; b < bay->size; b++){
                    bus = sharedData->getBayBus(sharedData, B, b);
                    if(bus->state == PARKED){
                        cout << "   Bus " << bus->ID
                            << " from " << DEST[bus->dest]
                            << " parked " << now-bus->parked
                            << " seconds ago and dropped off " << bus->passengersCame
                            << " passengers." << endl;
                        passengersCame[bus->dest] += bus->passengersCame;
                    }
                    else if (bus->state == LEFT && !bus->checked){
                        bus->checked = true;
                        cout << "   Bus " << bus->ID
                            << " left the station " << now-bus->exited
                            << " seconds ago carrying " << bus->passengersLeft
                            << " passengers to " << DEST[bus->dest] << "." << endl;
                        ledger->addBus(bus);
                    }
                    
                }
                
                cout << endl;
            }
            /*total passengers per destination*/
            for(int i = 0; i < 3; i++){
                cout << "Parked buses dropped off " << passengersCame[i]
                    << " total passengers from " << DEST[i] << "." << endl;
            }
            cout << endl;

            /*out driveway status*/
            if(sem_getvalue(&sharedData->outDriveway,&semval) != 0){
                cout << "Error getting semaphore value" << endl;
            }
            
            if(semval == 0){
                bus = sharedData->getBayBus(sharedData,sharedData->OUTbayIndex, sharedData->OUTparkingIndex);
                cout << "Bus " << bus->ID
                    << " departed to " << DEST[bus->dest] 
                    << " carrying " << bus->passengersLeft
                    << " passengers " << now - bus->departed
                    << " seconds ago." << endl;
            }
            else cout << "No bus is exiting the station now." << endl;
            cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>" << endl << endl;
        }
        
        if(function == 'S' || function == 'B'){
            /*print statistics (S:STATS)*/
            printStats(sharedData, ledger, NOW);
            
        }

        cout.flush();

        /*sem post all sems*/

        eRR = sem_post(&sharedData->coutMutex);    /*unlock cout mutex*/
        if(eRR != 0) cout << "Sem post ERROR" << endl;

        eRR = sem_post(&sharedData->bayMutex);    /*unlock bay mutex*/
        if(eRR != 0) cout << "Sem post ERROR" << endl;

        eRR = sem_post(&sharedData->outGoingMutex);    /*unlock outGoing mutex*/
        if(eRR != 0) cout << "Sem post ERROR" << endl;

        eRR = sem_post(&sharedData->inBoundMutex);    /*unlock inbound mutex*/
        if(eRR != 0) cout << "Sem post ERROR" << endl;

    }

    printStats(sharedData, ledger, NULL);

    /*unmap shared memory*/
    eRR = munmap(sharedData, shmSize);
    if (eRR == -1){
        cout << "munmap ERROR" << endl;
    }
    eRR = close(shmfd);
    if (eRR == -1){
        cout << "closing shmfd ERROR" << endl;
    }

    delete ledger;

}


void termHandler(int sig){
    cout << "Comptroller caught station termination signal: " << sig << endl;
    terminate = true;
};


/*prints statistics of station(needs sync before call)*/
void printStats(stationDATA * sharedData, Ledger * ledger, struct tm * NOW){
    /*refresh statistics*/

    for(int B = 0; B < sharedData->totalBays; B++){
        Bay * bay = sharedData->getBay(sharedData, B);

        for (int b = 0; b < bay->size; b++){
            Bus * bus = sharedData->getBayBus(sharedData, B, b);
            if (bus->state == LEFT && !bus->checked){
                bus->checked = true;
                ledger->addBus(bus);
            }
        }
    }

    ledger->SetNotServed(sharedData->notServed);
    
    cout << endl << endl;
    if(terminate) cout << "_________________ FINAL STATISTICS _________________" << endl << endl;
    else cout << "__________    STATISTICS TILL " << asctime(NOW) << endl;
    ledger->printStats();
    cout << "________________________________________________________________" << endl << endl;
};

/*calculates sleep duration*/
int calcSleepDuration(int interval, int & snapTimeLeft, int statTime, int & statTimeLeft, char & function){
    int ret;
    if (snapTimeLeft < statTimeLeft){
        ret = snapTimeLeft;
        if(ret == interval){
            statTimeLeft -= interval;
        }
        else{
            statTimeLeft -= ret;
        }
        snapTimeLeft = interval;
        function = 'P';
    }
    else if(snapTimeLeft == statTimeLeft){
        ret = statTimeLeft;
        function = 'B';
        statTimeLeft = statTime;
        snapTimeLeft = interval;
    }
    else if (snapTimeLeft > statTimeLeft){
        ret = statTimeLeft;
        if(ret == statTime){
            snapTimeLeft -= statTime;
        }
        else{
            snapTimeLeft -= ret;
        }
        statTimeLeft = statTime;
        function = 'S';
    }
    return ret;
};