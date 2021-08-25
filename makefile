CC = g++

all: bus\
	stationmanager\
	comptroller\
	mystation

mystation: mystation.o objects/stationDATA.o
	$(CC) -g -pthread mystation.o objects/stationDATA.o -o mystation -lrt

bus: executables/bus.o objects/stationDATA.o objects/busDATA.o
	$(CC) -g -pthread executables/bus.o objects/stationDATA.o objects/busDATA.o -o bus -lrt

stationmanager: executables/stationmanager.o objects/stationDATA.o
	$(CC) -g -pthread executables/stationmanager.o objects/stationDATA.o -o stationmanager -lrt

comptroller: executables/comptroller.o objects/stationDATA.o objects/ledger.o
	$(CC) -g -pthread executables/comptroller.o objects/stationDATA.o objects/ledger.o -o comptroller -lrt

stationDATA.o: objects/stationDATA.cpp objects/stationDATA.h
	$(CC) -c -g objects/stationDATA.cpp objects/

busDATA.o: objects/busDATA.cpp objects/busDATA.h
	$(CC) -c -g objects/busDATA.cpp objects/

ledger.o: objects/ledger.cpp objects/ledger.h
	$(CC) -c -g objects/ledger.cpp objects/

bus.o: executables/bus.cpp objects/stationDATA.h objects/busDATA.h
	$(CC) -c -g executables/bus.cpp executables/

stationmanager.o: executables/stationmanager.cpp objects/stationDATA.h
	$(CC) -c -g executables/stationmanager.cpp executables/

comptroller.o: executables/comptroller.cpp objects/stationDATA.h objects/ledger.h
	$(CC) -c -g executables/comptroller.cpp executables/

clean:
	rm -rvf *.o executables/*.o mystation bus stationmanager comptroller objects/*.o

cleanOuts:
	rm -rvf IO/*.txt