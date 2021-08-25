#ifndef busDATA_h
#define busDATA_h

#include <sys/types.h>
#include <sys/ipc.h> 
#include <sys/shm.h> 

/*used only in bus executable to store its data*/
class busDATA{
    private:
        unsigned int capacity, passengers, parkperiod, mantime, type;
        pid_t busID;

    public:
        busDATA(pid_t id);
        ~busDATA();

        /*setters*/
        void setCapacity(unsigned int cap);
        void setCarryingPassengers(unsigned int passers);
        void setParkPeriod(unsigned int PPeriod);
        void setManeuverTime(unsigned int MTime);
        void setType(int t);

        /*getters*/
        unsigned int getCapacity();
        unsigned int getCurrentPassengers();
        unsigned int getParkPeriod();
        unsigned int getManeuverTime();
        pid_t getBusID();
        int getType();
};

#endif