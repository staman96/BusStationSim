Manolas Stamatios
DIT  Operating Systems 2019 Project 3


General:

The execution command is:

    ./mystation -l config.txt -f 2 
-f is the bus process creation frequency and -l is the configuration file. The input arguments initialize with default values, so they are optional. The project is developed in C++. The compilation is done with the 'make' command, and 'make clean' deletes all executables and .ο files. The output file is IO/log.txt. Also, with the command 'make cleanOuts', there is an option to erase the output file. I also used Valgrind to check for memory leaks with the command:

    make clean && make && valgrind -v --leak-check=yes --show-leak-kinds=all --track-origins=yes --trace-children=yes ./mystation -l config.txt -f 2

******************************************************************

Shared Memory Structure:

Class stationDATA contains all semaphores and variables used for the communication between inbound/outbound bus processes. There is an array of aisles (class Bay) with different amount of parking space.


* size of stationDATA
* totalbays * size of Bay(baysOffSet)
* total parking slots of each bay * size of Bus(busesOfBaysOffSet)

Semaphores:

* CommBusWrote, CommStMngrWrote: Used to synchronize the communication between inbound bus and station manager (starting value: 0)
* inDriveway: depicts the road from the entrance till the parking (starting value: 1)
* outDriveway: depicts the road from the parking till the exit (starting value: 1)
* inBoundQueue: It's a queue for the bus processes waiting to enter the parking.
* outGoingQueue: It's a queue for the bus processes waiting to exit the parking.
* OutGoingComm: Used to synchronize the communication between inbound bus and station manager (starting value: 0)

Mutexes:(starting value: 1)
* inBoundMutex: it's used to sychronize all entrace operations
* outGoingMutex: it's used to sychronize all exit operations
* bayMutex:it's used to sychronize access on the aisles and the bus parking spots
* coutMutex: it's used to sychronize concurrent prints

******************************************************************

#Notes:

Memory is a snapshot of the station. Each bus process, and station manager processes, write on the shared memory. The Comptroller process saves snapshots of the station to the ledger to extract statistics.
The configuraion file's format is as follows:
"
1 1 1 #parking_aisles (ASK PEL VOR)
5 6 7 #parking_aisles_capacity_per_type (ASK PEL VOR)
50 #bus_max_capacity
20 #bus_max_period_time_at_aisle
1 #maneuver_time
12 4 #comptroller_intervals(stat_snapshot) (stat snap)
2 2 4 #buses_of_each_type (ASK PEL VOR)
"
The space seperated values in front correspond to the aisle types in brackets(ASK, PEL, VOR).

******************************************************************


******************************************************************
Ανάπτυξη Κώδικα:

mystation.cpp:
	Διαβάζει το config αρχείο, και αρχικοποιεί με βάση αυτό τη κοινή μνήμη.  Έπειτα, ξεκινά το station manager, το comptroller και με μία επανάληψη
ξεκινά τη δημιουργία λεωφορείων, που θα σταματήσει όταν δωθεί εντολή τερματισμού ή αν ξεκινήσουν όσα λεωφορεία έγραφε
στο config αρχείο. Τυχαία, επιλέγεται τι τύπος λεωφορείου θα ξεκινήσει αν έχει οριστεί στο config. Τυχαία, επίσης
σε διάστημα f(όρισμα) δευτερολέπτων θα ξεκινήσει κι ένα λεωφορείο. Αφού, με waitpid περιμένει για όλες τις διαδικασίες
που ξεκίνησε να κλείσουν, διαγράφει και καταστρέφει τη κοινή μνήμη.

station manager.cpp:
	Κάνει 2 βασικές λειτουργίες. Ελέγχει αν υπάρχει χώρος να μπει λεωφορείο που έχει δηλώσει ενδιαφέρον, με διάλογο που
συχρονίζεται από 2 σεμαφόρους (CommBusWrote, CommStMngrWrote) ανταλλάσουν πληροφορίες, το λεωφορείο από που έρχεται,
ποιο είναι και με πόσους επίβατες, και ο station manager, σε ποιά θέση μιας νησίδας θα πάει να παρκάρει. Επιπλέον, όταν
κάποιο λεωφορείο θέλει να φύγει, ο station manager δέχεται ως πληροφορία, που ήταν παρκαρισμένο το λεωφορείο. Αν θέλει
ένα ΠΕΛ λεωφορείο να εισέλθει και δεν υπάρχει ίδιου είδους νησίδα, τότε βλέπει κλειστό το σταθμό. Όλα τα παραπάνω 
καταγράφονται από τον station manager στο log.txt. Τέλος, όταν έχει έρθει σήμα για τερματισμό και έχει αδειάσει ο
σταθμός στέλνει σήμα στο comptroller να τελειώσει.

bus.cpp:
	Περιμένει στην ουρά, μόλις τον ξυπνήσει ο station manager, γράφει τα στοιχεία του στη κοινή μνήμη και περιμένει να
μάθει από το station manager σε ποια θεση μιας νησίδας θα πάει ή αν θα αποχωρήσει από το σταθμό. Αφότου πάρει το οκ,
για κάθε βήμα που κάνει στο σταθμό, γράφει τις απαραίτητες πληφορείες στη κοινή μνήμη(class Bus), όπως χρόνους και επιβάτες
και κατάσταση. ο χρόνος που θα μείνει παρκαρισμένο το λεωφορείο εξαρτάται ποσοστιαία από τη πληρότητά του μπαίνοντας, αλλά
και από τη πληρότητά βγαίνοντας, σε σχέση με το μέγιστο χρόνο παρακαρίσματος(ο αριθμός των επιβατών που θα επιβιστούν
αποφασίζεται τυχαία). Μέχρι και τη στιγμή πριν φύγουν από το σταθμό γράφουν πληροφορίες στη κοινή μνήμη.