#include <iostream>
#include <cstring>
#include <sys/types.h>
#include <sys/ipc.h> 
#include <sys/shm.h>
#include "busDATA.h"

busDATA::busDATA(pid_t id)
:busID(id){

};

busDATA::~busDATA(){

};

/*setters*/
void busDATA::setCapacity(unsigned int cap){
    this->capacity = cap;
};

void busDATA::setCarryingPassengers(unsigned int passers){
    this->passengers = passers;
};

void busDATA::setParkPeriod(unsigned int PPeriod){
    this->parkperiod = PPeriod;
};

void busDATA::setManeuverTime(unsigned int MTime){
    this->mantime = MTime;
};

void busDATA::setType(int t){
    this->type = t;
};

/*getters*/
unsigned int busDATA::getCapacity(){
    return this->capacity;
};

unsigned int busDATA::getCurrentPassengers(){
    return this->passengers;
};

unsigned int busDATA::getParkPeriod(){
    return this->parkperiod;
};

unsigned int busDATA::getManeuverTime(){
    return this->mantime;
};

pid_t busDATA::getBusID(){
    return this->busID;
};

int busDATA::getType(){
    return this->type;
};