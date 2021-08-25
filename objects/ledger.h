#ifndef ledger_h
#define ledger_h

#include "stationDATA.h"

// class BusNode{
//     private:
//         Bus bus;
//         BusNode * next;
//     public:
//         BusNode(Bus b);
//         ~BusNode();
//         /*setters*/
//         void setNext(BusNode * newNode);
//         /*getters*/
//         BusNode * getNext();
//         Bus & getBus();

// };

class Ledger{
    private:
        // BusNode * head, * tail;/*list of bus history(exited the station)*/
        // int length;
        double totalTimeParkedPerDest[3],
            totalBusOccupancyPercentage[3],/*for ave bus occ(in and out)*/
            totalStationTimePerDest[3];
        int totalPassengersCamePerDest[3], 
            totalPassengersLeftPerDest[3],
            totalbusesServedPerDest[3],
            notServed;
    /*IMPORTANT!!
    **bus data is used for stat's calculation after 
    **bus has left the station*/
    public:
        Ledger();
        ~Ledger();
        /*add node to list, when called it refreshes stats*/
        void addBus(Bus * bus);
        /*getters*/
        // Bus & getBus(int index);
        int getLength();
        /*prints statistics*/
        void printStats();

        void SetNotServed(int notServ);

};

#endif