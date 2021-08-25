#include <iostream>
#include <cstring>
#include <unistd.h>
#include <stdio.h>
#include <cstdlib>
#include <fcntl.h> 
#include <sys/shm.h> 
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h> 
#include <signal.h>
#include <stdlib.h> 
#include <ctime>
#include <sys/mman.h>
#include "objects/stationDATA.h"

using std::cout;
using std::endl;

void termHandler(int sig);
int configure(char * conf, int * bays, int * bayCap, int & busCap,int & parkTime, int & manTime, int * busTypes, int & statTimer, int & snapshotTimer);
int calcBytes(int * bays, int * bayCap);

static bool term;

int main(int argc, char * argv[]){
    srand(time(NULL));
    cout << "STATIONDATA SIZE: " << sizeof(stationDATA) << endl;
    cout << "BAY SIZE: " << sizeof(Bay) << endl;
    term = false;
    signal(SIGINT, termHandler);

    int numOfAislesPerDest[3], aisleCap[3], busCap = 50, maxParkedTime = 5, maneuverTime = 2, numOfBusesPerDest[3];
    char * config = NULL;
    int waiters = 0, shmfd = -1, shmSize = -1;
    int statTimer = 15, snapShotTimer = 5, eRR, busFreq = 3;


    int i = 1;
    while(i<argc){
        if(strcmp(argv[i],"-l") == 0){
            //configuration file
            i++;
            config = argv[i];
            cout << "Configuration file is: " << argv[i] << endl;
        }
        else if(strcmp(argv[i],"-f") == 0){
            //bus creation frequency
            i++;
            busFreq = atoi(argv[i]);
            cout << "Bus arrival frequency is: " << argv[i] << " seconds." << endl;
        }
        else{
            cout << "Invalid parameter" << endl;
        }
        i++;
    }

    /*checks if input file argument was given, if not term*/
    if (config == NULL){
        cout << "Configuration file argument wasn't given. Using default values." << endl;     
    }
    else{
        configure(config, numOfAislesPerDest, aisleCap, busCap, maxParkedTime, maneuverTime, numOfBusesPerDest, statTimer, snapShotTimer);
    }

    /*SHARED MEMORY*/
  
    /* shm_open returns the shmfd */
    if((shmfd = shm_open(sharedMemName, O_CREAT | O_RDWR, 0666)) == -1){
        cout << "Getting shared memory failed" << endl;
    }
  

    shmSize = calcBytes(numOfAislesPerDest, aisleCap);
    ftruncate(shmfd, shmSize);
    stationDATA * sharedData;

    sharedData = (stationDATA *)mmap(0, shmSize, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);

    sharedData->InitBusStation(sharedData, numOfAislesPerDest, aisleCap);

    /*Open bus station(start processes)*/
    char shmSizestr[12];
    sprintf(shmSizestr, "%d", shmSize);

    /*--->start station manager*/
    pid_t stMngPID;
    
    /*FORK station manager*/
    if ((stMngPID = fork()) == 0){
        /*free space from heap??*/

        if (execlp("./stationmanager", "stationmanager", "-s", shmSizestr, NULL) == -1){
            perror("STATION MANAGER EXEC ERROR\n");
            return 3;
        }
    }
    else if(stMngPID == -1){
        cout << getpid() << " || ";
        cout << "Station Manager fork error!" << endl;
        return 4;
    }


    /*--->start comptroller*/
    pid_t cmptrPID;
    char statTimerstr[12], snapShotTimerstr[12];
    sprintf(statTimerstr, "%d", statTimer);
    sprintf(snapShotTimerstr, "%d", snapShotTimer);

    /*FORK comptroller*/
    if ((cmptrPID = fork()) == 0){
        /*free space from heap??*/

        if (execlp("./comptroller", "comptroller", "-d", snapShotTimerstr, "-t", statTimerstr, "-s", shmSizestr, NULL) == -1){
            perror("COMPTROLLER EXEC ERROR\n");
            return 3;
        }
    }
    else if(cmptrPID == -1){
        cout << getpid() << " || ";
        cout << "Comptroller fork error!" << endl;
        return 4;
    }

    /*--->start buses*/
    cout << endl << "Buses started arriving..." << endl;
    int amountOfBuses = 0;
    int busesLeft[3];
    for(int i = 0; i < 3; i++){
        amountOfBuses += numOfBusesPerDest[i];
        busesLeft[i] = numOfBusesPerDest[i];
    }
    
    for (int b = 0; b < amountOfBuses && term == false; b++){

        cout << endl;

        /*set bus arguments*/
        char capacitystr[12], passengersstr[12], parkedTimestr[12], maneuverTimestr[12];
        pid_t busID;
        char busTypestr[4];
        int passengers = randgen()%busCap + 1;/*random amount of passengers less or equal than max cap*/

        /*type*/
        int busType;
        do{
            busType = randgen()%3;/*buses arrive randomly from every location*/
        }while(busesLeft[busType] == 0);
        busesLeft[busType]--;
        strcpy(busTypestr, DEST[busType]);      
        

        sprintf(passengersstr, "%d", passengers);
        sprintf(capacitystr, "%d", busCap);
        sprintf(parkedTimestr, "%d", maxParkedTime);
        sprintf(maneuverTimestr, "%d", maneuverTime);

        /*FORK*/
        if ((busID = fork()) == 0){
            /*free space from heap*/
        
            if (execlp("./bus", "bus", "-t", busTypestr, "-n", passengersstr, "-c", capacitystr, "-p", parkedTimestr, "-m", maneuverTimestr, "-s", shmSizestr, NULL) == -1){
                perror("BUS EXEC ERROR\n");
                return 3;
            }

        }
        else if(busID == -1){
            cout << getpid() << " || ";
            cout << "Bus fork error!" << endl;
            return 4;
        }
        else{
          waiters++;
          setpgid(busID, getpid());/*add to process group id to Wuntraced wait*/
        }

        sleep(randgen()%busFreq);/*and arrive with delay from each other*/
        
    }
    

    /*wait for buses to terminate and for station manager and comptroller*/
    int status;
    for (int w = 0; w < waiters; w++) waitpid(0, &status, WUNTRACED);
    waitpid(stMngPID, &status, WUNTRACED);
    waitpid(cmptrPID, &status, WUNTRACED);
    

    /*free shared memory*/
    sharedData->closeBusStation(sharedData);
    eRR = munmap(sharedData, shmSize);
    if (eRR == -1){
        cout << "munmap ERROR" << endl;
    }
    eRR = close(shmfd);
    if (eRR == -1){
        cout << "closing shmfd ERROR" << endl;
    }
    if(shm_unlink(sharedMemName) == -1){
        cout << "ERROR DESTROYING SHARED SEGMENT" << endl;
    };
    
    cout << "BUS STATION CLOSED" << endl;
}

void termHandler(int sig){
    cout << "Caught termination signal: " << sig << endl;
    cout << "Terminating bus station operation..." << endl;
    /*if true, main exits the loop after current cycle and exits after children
    finish and memory was freed*/  
    term = true;
}


 /* *************************
  * read configuration file*/
int configure(char * conf, int * bays, int * bayCap, int & busCap, int & parkTime, int & manTime, int * busTypes, int & statTimer, int & snapshotTimer){
   
    /*temporary variables*/
    char * pch;
    size_t length = 256;
    char line[length];/*its probably too much*/

    /*open file to read*/
    FILE* configfile = fopen(conf,"r");
    
    /*check if file is open*/
    if (configfile != NULL){
        /*reads file line by line*/

        if ( fgets(line, length, configfile) != NULL ){
            /*number of parking bays per destination*/
            pch = strtok(line," \n");
            bays[0] = atoi(pch);
            for (int i = 1; i < 3; i++){
                pch = strtok(NULL," \n");
                bays[i] = atoi(pch);
            }
            pch = strtok(NULL," \n");
            cout << pch;
            for (int i = 0; i < 3; i++) cout << " " << bays[i];
            cout << endl;
            
        }

        if ( fgets(line, length, configfile) != NULL ){
            /*parking bay's capacity per destination*/
            pch = strtok(line," \n");
            bayCap[0] = atoi(pch);
            for (int i = 1; i < 3; i++){
                pch = strtok(NULL," \n");
                bayCap[i] = atoi(pch);
            }
            pch = strtok(NULL," \n");
            cout << pch;
            for (int i = 0; i < 3; i++) cout << " " << bayCap[i];
            cout << endl;
            
        }

        if ( fgets(line, length, configfile) != NULL ){
            /*bus capacity*/
            pch = strtok(line," \n");
            busCap = atoi(pch);
            pch = strtok(NULL," \n");
            cout << pch << " " << busCap << endl;
            
        }

        if ( fgets(line, length, configfile) != NULL ){
            /*Maximum parking time*/
            pch = strtok(line," \n");
            parkTime = atoi(pch);
            pch = strtok(NULL," \n");
            cout << pch << " " << parkTime << endl;
            
        }

        if ( fgets(line, length, configfile) != NULL ){
            /*maneuver time*/
            pch = strtok(line," \n");
            manTime = atoi(pch);
            pch = strtok(NULL," \n");
            cout << pch << " " << manTime << endl;
            
        }

        if ( fgets(line, length, configfile) != NULL ){
            /*comptroller interval timers*/
            pch = strtok(line," \n");
            statTimer = atoi(pch);
            pch = strtok(NULL," \n");
            snapshotTimer = atoi(pch);
            pch = strtok(NULL," \n");
            cout << pch << " " << statTimer << " " << snapshotTimer << endl;
            
        }

        if ( fgets(line, length, configfile) != NULL ){
            /*parking bay's capacity per destination*/
            pch = strtok(line," \n");
            busTypes[0] = atoi(pch);
            for (int i = 1; i < 3; i++){
                pch = strtok(NULL," \n");
                busTypes[i] = atoi(pch);
            }
            pch = strtok(NULL," \n");
            cout << pch;
            for (int i = 0; i < 3; i++) cout << " " << busTypes[i];
            cout << endl;
            
        }
        
        fclose(configfile);/*closing file*/
    }
    return 0;
}

/* ******************************
 * calculate shared memory size*/
int calcBytes(int * bays, int * bayCap){
    int total = 0;
    total += ST_SIZE;
    for(int i = 0; i < 3; i++){
        total += BAY_SIZE * bays[i];
        total += BUS_SIZE * bayCap[i] * bays[i];
    }
    return total;
}