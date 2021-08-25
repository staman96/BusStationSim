#include <iostream>
#include <cstring>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <cstdlib>
#include <sys/shm.h> 
#include <sys/mman.h>
#include <signal.h>
#include <fcntl.h>
#include <fstream>
#include <ctime>
#include "../objects/stationDATA.h"

using std::cout;
using std::endl;
using std::ofstream;

void termHandler(int sig);
static bool term;

int main(int argc, char * argv[]){
    srand(time(NULL));
    term = false;
    signal(SIGINT, termHandler);
    int shmfd, shmSize, eRR, occupied = 0, counter = 0, frequency = 1;
    time_t now;
    struct tm * NOW;    

    int i = 1;
    while(i<argc){
        if(strcmp(argv[i],"-s") == 0){
            //shared memory size
            i++;
            shmSize = atoi(argv[i]);
            cout << getpid() << " || ";
            cout << "(Station Manager)Shm size is: " << argv[i] << endl;
        }
        else if(strcmp(argv[i],"-f") == 0){
            //loop frequency
            i++;
            frequency = atoi(argv[i]);
        }
        else{
            cout << getpid() << " || ";
            cout << "Invalid parameter" << endl;
        }
        i++;
    }

    /* shm_open returns the shmfd */
    if((shmfd = shm_open(sharedMemName, O_RDWR, 0666)) == -1){
        cout << "Getting shared memory failed" << endl;
    }
    else{
        cout << "Station manager got the shared mem" << endl;
    }

    stationDATA * sharedData;
    sharedData = (stationDATA *)mmap(0, shmSize, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);

    ofstream output;
    output.open(logFile);
    

    /* * * STATION MANAGER FUNCTIONALITY * * */

    bool activeCom = false, newBus = true, busLeft = false, outDrivewayEmpty = true, inComingQEmpty = true;
    pid_t currentBus = 0;
    
    /*station manager stops only if bays, driveways, and queues are empty when was called to finish*/
    while(!term || occupied > 0 || !outDrivewayEmpty || !inComingQEmpty) {
        sleep(frequency);
        counter = 0;
        int semval, queue;
        bool accept = false;


        /* ---> INCOMING BUSES OPERATION <--- */

        /*one bus wants to enter(already active communication)*/
        if (activeCom){

            /*if it's a new communication with a bus (not already waiting)*/
            if(newBus){
                // cout << "Communicating with bus " << sharedData->ID << endl;

                eRR = sem_wait(&sharedData->CommBusWrote);    /*lock comm sem, wait for bus to write*/
                if(eRR != 0) cout << "stmngr || Sem wait ERROR " << counter << endl;
                counter++;
                // cout << counter++ << endl;

                eRR = sem_wait(&sharedData->inBoundMutex);    /*lock inbound mutex*/
                if(eRR != 0) cout << "stmngr || Sem wait ERROR " << counter << endl;
                counter++;

                currentBus = sharedData->ID;

                eRR = sem_post(&sharedData->inBoundMutex);    /*unlock inbound mutex*/
                if(eRR != 0) cout << "Sem post ERROR" << endl;
            }

            int B, bEnd, dest, b, passengers;/*B:bay, b:bus(parking slot), bus dest*/

            eRR = sem_wait(&sharedData->inBoundMutex);    /*lock inbound mutex*/
            if(eRR != 0) cout << "stmngr || Sem wait ERROR " << counter << endl;
            counter++;
            // cout << counter++ << endl;

            dest = sharedData->dest;

            eRR = sem_post(&sharedData->inBoundMutex);    /*unlock inbound mutex*/
            if(eRR != 0) cout << "Sem post ERROR" << endl;

            /*if station remains operational buses are accepte
            **check if station has available space
            **also check if a PEL bus wants to enter and there are
            **no PEL bays*/
            if(!term && !(dest == PEL && sharedData->baysPerDest[PEL] == 0)){
                


                eRR = sem_wait(&sharedData->bayMutex);    /*lock bay mutex*/
                if(eRR != 0) cout << "stmngr || Sem wait ERROR " << counter << endl;
                counter++;
                // cout << counter++ << endl;

                /*firstly check bays of same destination*/
                bEnd = sharedData->getBaysPerDest(dest, B);
                for (B; B <= bEnd; B++){
                    Bay * bay = sharedData->getBay(sharedData, B);  /*current bay*/
                    if (bay->occupied < bay->size){
                        accept = true;
                        // cout << DEST[bay->destination] << " Occupancy: " << bay->occupied << "/" << bay->size << endl;
                        // cout << "Space found in same dest " << DEST[dest] << endl;
                        break;
                    }
                }

                /*secondly check PEL (as ASK and VOR can park here)*/
                if(!accept && dest != PEL){
                    bEnd = sharedData->getBaysPerDest(PEL, B);
                    for (B; B <= bEnd; B++){
                        Bay * bay = sharedData->getBay(sharedData, B);  /*current bay*/
                        if (bay->occupied < bay->size){
                            accept = true;
                            // cout << DEST[bay->destination] << " Occupancy: " << bay->occupied << "/" << bay->size << endl;
                            // cout << "Space found in PEL for bus from " << DEST[dest] << endl; 
                            break;
                        }
                    }
                }

                /*lastly check ASK (as only VOR can park here)*/
                if(!accept && dest == VOR){
                    bEnd = sharedData->getBaysPerDest(ASK, B);
                    for (B; B <= bEnd; B++){
                        Bay * bay = sharedData->getBay(sharedData, B);  /*current bay*/
                        if (bay->occupied < bay->size){
                            accept = true;
                            // cout << DEST[bay->destination] << " Occupancy: " << bay->occupied << "/" << bay->size << endl;
                            // cout << "Space found in ASK for bus from VOR" << endl;
                            break;
                        }
                    }
                }

                /*find parking slot in bay(if comtroller haven't checked to get stats, bus can't use it)*/
                if(accept){
                    accept = false;
                    Bay * bay = sharedData->getBay(sharedData, B);
                    for (b = 0; bay->size; b++){
                        Bus * bus = sharedData->getBayBus(sharedData, B, b);
                        if(bus->state < 1 && bus->checked ){
                            accept = true;
                            break;
                        }
                    }
                }

                eRR = sem_post(&sharedData->bayMutex);    /*unlock bay mutex*/
                if(eRR != 0) cout << "Sem post ERROR" << endl;

                /*also check if Driveway is clear*/
                semval = -1;
                if(sem_getvalue(&sharedData->inDriveway,&semval) != 0){
                    cout << "Error getting semaphore value" << endl;
                }

                if (accept && semval == 1) {
                    /*accept bus and let it pass*/

                    eRR = sem_wait(&sharedData->inBoundMutex);    /*lock inbound mutex*/
                    if(eRR != 0) cout << "stmngr || Sem wait ERROR " << counter << endl;
                    counter++;
                    // cout << counter++ << endl;

                    sharedData->INbayIndex = B;   /*bay ID*/
                    sharedData->INparkingIndex = b; /*parking slot ID(index)*/

                    eRR = sem_post(&sharedData->inBoundMutex);    /*unlock inbound mutex*/
                    if(eRR != 0) cout << "Sem post ERROR" << endl;

                    eRR = sem_wait(&sharedData->bayMutex);    /*lock bay mutex*/
                    if(eRR != 0) cout << "stmngr || Sem wait ERROR " << counter << endl;
                    counter++;
                    // cout << counter++ << endl;

                    sharedData->getBay(sharedData, B)->occupied++;  /*add bus to station*/
                    occupied++;

                    eRR = sem_post(&sharedData->bayMutex);    /*unlock bay mutex*/
                    if(eRR != 0) cout << "Sem post ERROR" << endl;

                    /*ending communication*/
                    activeCom = false;
                    
                }
                else {
                    // cout << "Bus " << currentBus << " has to wait!" << endl;
                    /*as long as bus has to wait, it becomes old*/
                    newBus = false; 
                }

            }
            else{
                /* bus has to leave cause station is closing and can't enter station*/

                eRR = sem_wait(&sharedData->inBoundMutex);    /*lock inbound mutex*/
                if(eRR != 0) cout << "stmngr || Sem wait ERROR " << counter << endl;
                counter++;
                // cout << counter++ << endl;

                sharedData->INbayIndex = -1;
                sharedData->notServed++;

                output << "Bus " << sharedData->ID << " never entered the station" << endl;

                eRR = sem_post(&sharedData->inBoundMutex);    /*unlock inbound mutex*/
                if(eRR != 0) cout << "Sem post ERROR" << endl;

                /*ending communication*/
                activeCom = false;
            }

             /*if bus can continue execution*/
            if(!activeCom){
                eRR = sem_post(&sharedData->CommStMngrWrote);    /*unlock comm sem, so that bus can continue*/
                if(eRR != 0) cout << "Sem post ERROR" << endl;
                /*logging bus that entered station*/
                time(&now);
                NOW = localtime(&now);
                output << "Bus " << sharedData->ID 
                    << " with destination " << DEST[dest]
                    << " carrying " << sharedData->passengersIN << " passengers,"
                    << " entered station at " << asctime(NOW);
                eRR = sem_wait(&sharedData->CommBusWrote);    /*lock comm sem, wait for bus to read*/
                if(eRR != 0) cout << "stmngr || Sem wait ERROR " << counter << endl;
                counter++;
                // cout << counter++ << endl;
            }
            
        }
        else{
            /*checks if other buses are waiting*/

            eRR = sem_wait(&sharedData->inBoundMutex);    /*lock inbound mutex*/
            if(eRR != 0) cout << "stmngr || Sem wait ERROR " << counter << endl;
            counter++;
            // cout << counter++ << endl;

            queue = sharedData->INqueueSize;

            eRR = sem_post(&sharedData->inBoundMutex);    /*unlock inbound mutex*/
            if(eRR != 0) cout << "Sem post ERROR" << endl;

            /*if another is waiting, wake him to communicate*/
            if (queue > 0) {
                /*starting communication with bus to be released from inbound queue*/
                // cout << "Pull next bus from queue" << endl;
                activeCom = true;
                newBus = true;
                /*reinitialize the communication semaphores*/
                if( sem_init(&sharedData->CommBusWrote, 1, 0) != 0){
                    cout << "Error initializing semaphore!" << endl;
                }
                if( sem_init(&sharedData->CommStMngrWrote, 1, 0) != 0){
                    cout << "Error initializing semaphore!" << endl;
                }

                eRR = sem_post(&sharedData->inBoundQueue);  /*bring next bus, unlock next bus*/
                if(eRR != 0) cout << "Sem post ERROR" << endl;
                // cout << "Next bus from queue is going to served" << endl;

                /*remove bus from queue*/
                eRR = sem_wait(&sharedData->inBoundMutex);    /*lock inbound mutex*/
                if(eRR != 0) cout << "stmngr || Sem wait ERROR " << counter << endl;
                counter++;
                // cout << counter++ << endl;

                sharedData->INqueueSize--;

                eRR = sem_post(&sharedData->inBoundMutex);    /*unlock inbound mutex*/
                if(eRR != 0) cout << "Sem post ERROR" << endl;
                    
            }

        }



        /* ---> OUTGOING BUSES OPERATION <--- */


        /*getting outgoing queue size*/
        eRR = sem_wait(&sharedData->outGoingMutex);    /*lock outGoing mutex*/
        if(eRR != 0) cout << "stmngr || Sem wait ERROR " << counter << endl;
        counter++;
        // cout << counter++ << endl;

        queue = sharedData->OUTqueueSize;

        eRR = sem_post(&sharedData->outGoingMutex);    /*unlock outGoing mutex*/
        if(eRR != 0) cout << "Sem post ERROR" << endl;

        if(queue > 0){

            /*check if outgoing Driveway is clear*/
            semval = -1;
            if(sem_getvalue(&sharedData->outDriveway,&semval) != 0){
                cout << "Error getting semaphore value" << endl;
            }

            /* if Driveway is clear let a bus leave */
            if(semval > 0){
                eRR = sem_post(&sharedData->outGoingQueue);    /*unlock bus from outGoing queue*/
                if(eRR != 0) cout << "Sem post ERROR" << endl;

                /*reduce the size of outgoing queue*/
                eRR = sem_wait(&sharedData->outGoingMutex);    /*lock outGoing mutex*/
                if(eRR != 0) cout << "stmngr || Sem wait ERROR " << counter << endl;
                counter++;
                // cout << counter++ << endl;

                sharedData->OUTqueueSize--;

                eRR = sem_post(&sharedData->outGoingMutex);    /*unlock outGoing mutex*/
                if(eRR != 0) cout << "Sem post ERROR" << endl;
                
                /*wait for bus that left to write its data*/
                eRR = sem_wait(&sharedData->OutGoingComm);    /*lock outGoing Comm*/
                if(eRR != 0) cout << "stmngr || Sem wait ERROR " << counter << endl;
                counter++;
                // cout << counter++ << endl;

                /*read bus data*/
                eRR = sem_wait(&sharedData->outGoingMutex);    /*lock outGoing mutex*/
                if(eRR != 0) cout << "stmngr || Sem wait ERROR " << counter << endl;
                counter++;

                int bayIndex = sharedData->OUTbayIndex;
                int busIndex = sharedData->OUTparkingIndex;

                eRR = sem_post(&sharedData->outGoingMutex);    /*unlock outGoing mutex*/
                if(eRR != 0) cout << "Sem post ERROR" << endl;

                eRR = sem_wait(&sharedData->bayMutex);    /*lock bay mutex*/
                if(eRR != 0) cout << "stmngr || Sem wait ERROR " << counter << endl;
                counter++;
                // cout << counter++ << endl;

                sharedData->getBay(sharedData, bayIndex)->occupied--;   /*mark empty a parking spot*/
                Bus * tempBus = sharedData->getBayBus(sharedData, bayIndex, busIndex);
                occupied--;
                
                /*logging leaving bus*/
                time(&now);
                NOW = localtime(&now);
                output << "Bus " << tempBus->ID 
                    << " with destination " << DEST[tempBus->dest]
                    << " left " << DEST[sharedData->getBay(sharedData, bayIndex)->destination]
                    << " bay No " << bayIndex   
                    << " carrying " << tempBus->passengersLeft << " passengers at "
                    << asctime(NOW);

                eRR = sem_post(&sharedData->bayMutex);    /*unlock bay mutex*/
                if(eRR != 0) cout << "Sem post ERROR" << endl;

                 
            }
        }


        /*checking station status
        **(don't have to check in driveway because occupied counter will be > 0)*/

        /*check if outgoing Driveway is clear*/
        semval = -1;
        if(sem_getvalue(&sharedData->outDriveway,&semval) != 0){
            cout << "Error getting semaphore value" << endl;
        }
        if(semval > 0){
            outDrivewayEmpty = true;
        }
        else{
            outDrivewayEmpty = false;
        }

        /*check if incoming queue is empty*/
        eRR = sem_wait(&sharedData->inBoundMutex);    /*lock inbound mutex*/
        if(eRR != 0) cout << "stmngr || Sem wait ERROR " << counter << endl;
        counter++;
        // cout << counter++ << endl;

        sharedData->INqueueSize > 0 ? inComingQEmpty = false : inComingQEmpty = true; 

        eRR = sem_post(&sharedData->inBoundMutex);    /*unlock inbound mutex*/
        if(eRR != 0) cout << "Sem post ERROR" << endl;

        /*END OF WHILE LOOP*/
        output.flush();
    }

    eRR = sem_wait(&sharedData->coutMutex);    /*lock cout mutex*/
    if(eRR != 0) cout << "stmngr || Sem wait ERROR " << counter << endl;
    counter++;
    // cout << getpid() << " || " << ++counter << endl;

    cout << "Shuting down station..." << endl;

    eRR = sem_post(&sharedData->coutMutex);    /*unlock cout mutex*/
    if(eRR != 0) cout << "Sem post ERROR" << endl;

    /*inform that station is closed*/
    kill(sharedData->comptroller, SIGUSR1);

    /*free shared memory*/
    eRR = munmap(sharedData, shmSize);
    if (eRR == -1){
        cout << "munmap ERROR" << endl;
    }
    eRR = close(shmfd);
    if (eRR == -1){
        cout << "closing shmfd ERROR" << endl;
    }
    output.close();

}

void termHandler(int sig){
    cout << "Caught station termination signal: " << sig << endl;
    cout << "Waiting for all buses to leave station..." << endl;
    term = true;
}