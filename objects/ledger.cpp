#include <iostream>
#include <stdio.h>
#include <cstdlib>
#include "ledger.h"
#include "stationDATA.h"

using std::cout;
using std::endl;

/******************************** BUSNODE *******************************/
// BusNode::BusNode(Bus b)
// :bus(b){
//     this->next = NULL;
//     // this->bus.checked = false;
// };

// BusNode::~BusNode(){
//     if(this->next != NULL) delete this->next;
// };

// void BusNode::setNext(BusNode * newNode){
//     this->next = newNode;
// };

// BusNode * BusNode::getNext(){
//     return this->next;
// };

// Bus & BusNode::getBus(){
//     return this->bus;
// };


/*******************************  Ledger  **********************************/

Ledger::Ledger(){
    // this->head = NULL;
    // this->tail = NULL;
    // this->length = 0;
    
    for(int i = 0; i < 3; i++){
        this->totalTimeParkedPerDest[i] = 0.0;
        this->totalBusOccupancyPercentage[i] = 0.0;
        this->totalStationTimePerDest[i] = 0.0;
        this->totalPassengersCamePerDest[i] = 0;
        this->totalPassengersLeftPerDest[i] = 0;
        this->totalbusesServedPerDest[i] = 0;
        this->notServed = 0;
    }

};

Ledger::~Ledger(){
    // if(this->head != NULL) delete this->head;
};

void Ledger::addBus(Bus * bus){
    /*add bus data to stats*/
    int dest = bus->dest;
    this->totalPassengersCamePerDest[dest] += bus->passengersCame;
    this->totalPassengersLeftPerDest[dest] += bus->passengersLeft;
    this->totalTimeParkedPerDest[dest] += (double)(bus->departed - bus->parked);
    this->totalBusOccupancyPercentage[dest] += (double)bus->passengersCame/(double)bus->capacity;
    this->totalBusOccupancyPercentage[dest] += (double)bus->passengersLeft/(double)bus->capacity;
    this->totalStationTimePerDest[dest] = this->totalStationTimePerDest[dest]
        + (double)(bus->exited - bus->entered);
    
    this->totalbusesServedPerDest[dest]++;

    /*add bus to list*/
    // BusNode * temp = new BusNode(*bus);
    // if(this->tail != NULL){
    //     this->tail->setNext(temp);
    //     this->tail = temp;
    // }
    // else{
    //     this->head = temp;
    //     this->tail = temp;
    // }
    // this->length++; 
};

// Bus & Ledger::getBus(int index){
//     if(index == this->length-1) return this->tail->getBus();
//     else if(index < this->length){
//         BusNode * temp = this->head;
//         for(int i = 0; i < index; i++){
//             temp = temp->getNext();
//         }
//         return temp->getBus();
//     }
// };

int Ledger::getLength(){
    // return this->length;
};

void Ledger::printStats(){
    //bus occupanvy is for incoming and outgoing so need to div with 2*length
    int totalbuses = 0, totalPassCame = 0, totalpassLeft = 0;
    double totalStationTime = 0.0, totalTimeparked = 0.0, totalBusOccPercent = 0.0;
    for (int d = 0; d < 3; d++){
        int busesServed = this->totalbusesServedPerDest[d];
        cout << "Destination   " << DEST[d] << endl;
        if (busesServed == 0) cout << "No bus was served yet." << endl << endl;
        else cout << busesServed << " buses were served while:" << endl
            << "    Average bus occupancy (from and to location) was " 
                << this->totalBusOccupancyPercentage[d]*100.0/(double)(busesServed*2) << "%" << endl
            << "    Average time parked was "
                << this->totalTimeParkedPerDest[d]/busesServed << " seconds" << endl
            << "    Average total time spent in station was "
                << this->totalStationTimePerDest[d]/busesServed << " seconds" << endl
            << "    Total passengers that dropped off were " 
                << this->totalPassengersCamePerDest[d] << endl
            << "    Total passengers that picked up were " 
                << this->totalPassengersLeftPerDest[d] << endl << endl;
        
        totalbuses += busesServed;
        totalPassCame += this->totalPassengersCamePerDest[d];
        totalpassLeft += this->totalPassengersLeftPerDest[d];
        totalStationTime += this->totalStationTimePerDest[d];
        totalTimeparked += this->totalTimeParkedPerDest[d];
        totalBusOccPercent += this->totalBusOccupancyPercentage[d];

    }

    if(totalbuses > 0){
        cout << "TOTALS(for all destintions)" << endl;
        cout << totalbuses << " buses were served while:" << endl
                << "    Average bus occupancy (from and to all locations) was " 
                    << totalBusOccPercent*100.0/(double)(totalbuses*2) << "%" << endl
                << "    Average time parked was "
                    << totalTimeparked/totalbuses << " seconds" << endl
                << "    Average total time spent in station was "
                    << totalStationTime/totalbuses << " seconds" << endl
                << "    Total passengers that dropped off were " 
                    << totalPassCame << endl
                << "    Total passengers that picked up were " 
                    << totalpassLeft << endl << endl;
    }
    else cout << "0 buses have left the station..." << endl << endl;

    cout << this->notServed 
        << " buses left station before they enter because station was closed." << endl << endl; 
};

void Ledger::SetNotServed(int notServ){
    this->notServed = notServ;
};