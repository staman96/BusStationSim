Manolas Stamatios Operating Systems 2019 Project 3

General:

The execution command is:

    ./mystation -l config.txt -f 2 
where -f is the bus process creation frequency. The input arguments initialize with default values, so they are optional. The project is developed in C++. The compilation is done with the 'make' command, and 'make clean' deletes all executables and .ο files. The output file is IO/log.txt. Also, with the command 'make cleanOuts', there is an option to erase the output file. I also used Valgrind to check for memory leaks with the command:

    make clean && make && valgrind -v --leak-check=yes --show-leak-kinds=all --track-origins=yes --trace-children=yes ./mystation -l config.txt -f 2

Shared Memory Structure:

Class stationDATA contains all semaphores and variables used for the communication between inbound/outbound bus processes. There is an array of aisles (class Bay) with different amount of parking space.

/******************************************************************
 * MEMORY STRUCTURE
 * ****************************************************************
 * size of stationDATA
 * totalbays * size of Bay(baysOffSet)
 * total parking slots of each bay * size of Bus(busesOfBaysOffSet)
 * 
*/

Semaphores:

CommBusWrote, CommStMngrWrote: Για το συχρονισμό επικοινωνίας λεωφορείου-station manager
	για την είσοδο του λεωφορείου (αρχική τιμή 0)
inDriveway: αναπαριστά το δρόμο από την είσοδο μέχρι το πάρκινγκ (αρχική τιμή 1)
outDriveway: αναπαριστά το δρόμο από το πάρκινγκ μέχρι την έξοδο (αρχική τιμή 1)
inBoundQueue: αναπαριστά την ουρά για είσοδο
outGoingQueue: αναπαριστά την ουρά για έξοδο
OutGoingComm: Για το συχρονισμό επικοινωνίας λεωφορείου-station manager για την έξοδο του λεωφορείου (αρχική τιμή 0)

	Mutexes:(αρχική τιμή 1)
inBoundMutex: για το συγχρονισμό των λειτουργιών εισόδου(και τις ανάλογες μεταβλητές)
outGoingMutex: για το συγχρονισμό των λειτουργιών εξόδου(και τις ανάλογες μεταβλητές)
bayMutex: για το συγχρονισμό πρόσβασης στις νησίδες και τις θέσεις για παρκάρισμα/λεωφορεία
coutMutex: για το συγχρονισμό μη ταυτόχρονης εκτύπωσης στην οθόνη
	
Σημειώσεις:
	Ουσιαστικά η μνήμη αποτελεί φωτογραφία του σταθμού, και το κάθε λεωφορείο γράφει στη class Bus τα στοιχεία
του αφού λάβει οδηγία από τον station manager σε ποια θα πάει. Ο comptroller με τη σειρά του παίρνει όλες τις
πληροφορίες από όλη τη κοινή μνήμη για να φτιάξει το ledger, το οποίο μόνο αυτός χρησιμοποιεί για τα στατιστικά.
	Είχα υλοποιήσει και λίστα που κρατούσε όλες τις πληφορίες από κάθε εισερχόμενο λεωφορείο, αλλά τη κατέργησα
(σχολιάζωντας το κώδικα) αφού δε ήταν χρήσιμη σε κάτι.
	Οι μεταβλητές OUTqueueSize, INqueueSize χρησιμοποιούνται για να ξέρω το μέγεθος της λίστας αφού η συνάρτηση
sem_getvalue επιστρέφει μόνο μη αρνητικούς αριθμούς.


Ανάπτυξη Κώδικα:

mystation.cpp:
	Διαβάζει το config αρχείο, και αρχικοποιεί με βάση αυτό τη κοινή μνήμη. Δεν είναι υποχρωτικό να δωθεί σαν όρισμα
εφόσον όλες οι μεταβλητές έχουν αρχική τιμή. Έπειτα, ξεκινά το station manager, το comptroller και με μία επανάληψη
ξεκινά τη δημιουργία λεωφορείων, που θα σταματήσει όταν δωθεί εντολή τερματισμού ή αν ξεκινήσουν όσα λεωφορεία έγραφε
στο config αρχείο. Τυχαία, επιλέγεται τι τύπος λεωφορείου θα ξεκινήσει αν έχει οριστεί στο config. Τυχαία, επίσης
σε διάστημα f(όρισμα) δευτερολέπτων θα ξεκινήσει κι ένα λεωφορείο. Αφού, με waitpid περιμένει για όλες τις διαδικασίες
που ξεκίνησε να κλείσουν, διαγράφει και καταστρέφει τη κοινή μνήμη.

Το configuraion file έχει τη μορφή:

1 1 1 #parking_aisles (ASK PEL VOR)
5 6 7 #parking_aisles_capacity_per_type (ASK PEL VOR)
50 #bus_max_capacity
20 #bus_max_period_time_at_aisle
1 #maneuver_time
12 4 #comptroller_intervals(stat_snapshot) (stat snap)
2 2 4 #buses_of_each_type (ASK PEL VOR)

και οι πολλοί αριθμοί μπροστά αντιστοιχούν στους τύπους των παρενθέσεων(είναι space seperated values).

station manager.cpp:
	Κάνει 2 βασικές λειτουργίες. Ελέγχει αν υπάρχει χώρος να μπει λεωφορείο που έχει δηλώσει ενδιαφέρον, με διάλογο που
συχρονίζεται από 2 σεμαφόρους (CommBusWrote, CommStMngrWrote) ανταλλάσουν πληροφορίες, το λεωφορείο από που έρχεται,
ποιο είναι και με πόσους επίβατες, και ο station manager, σε ποιά θέση μιας νησίδας θα πάει να παρκάρει. Επιπλέον, όταν
κάποιο λεωφορείο θέλει να φύγει, ο station manager δέχεται ως πληροφορία, που ήταν παρκαρισμένο το λεωφορείο. Αν θέλει
ένα ΠΕΛ λεωφορείο να εισέλθει και δεν υπάρχει ίδιου είδους νησίδα, τότε βλέπει κλειστό το σταθμό. Όλα τα παραπάνω 
καταγράφονται από τον station manager στο log.txt. Τέλος, όταν έχει έρθει σήμα για τερματισμό και έχει αδειάσει ο
σταθμός στέλνει σήμα στο comptroller να τελειώσει.

comptroller.cpp:
	Τυπώνει τη κατάσταση και τα στατιστικά του σταθμού, στα δοσμένα χρονικά διαστήματα. Κάθε φορά τρέχει μία από τις
δύο λειτουργίες, συλλέγει τα στοιχεία από τα λεωφορεία που έφυγαν πρόσφατα, και όποιο έχει διαβάζεται, αλλάζει τη μεταβλητή
checked σε αληθής. Όταν λάβει σήμα τερματισμού από το station manager, κάνει ένα τελευταίο έλεγχο και τυπώνει τα συνολικά
στατιστικά λειτουργίας του σταθμού.

bus.cpp:
	Περιμένει στην ουρά, μόλις τον ξυπνήσει ο station manager, γράφει τα στοιχεία του στη κοινή μνήμη και περιμένει να
μάθει από το station manager σε ποια θεση μιας νησίδας θα πάει ή αν θα αποχωρήσει από το σταθμό. Αφότου πάρει το οκ,
για κάθε βήμα που κάνει στο σταθμό, γράφει τις απαραίτητες πληφορείες στη κοινή μνήμη(class Bus), όπως χρόνους και επιβάτες
και κατάσταση. ο χρόνος που θα μείνει παρκαρισμένο το λεωφορείο εξαρτάται ποσοστιαία από τη πληρότητά του μπαίνοντας, αλλά
και από τη πληρότητά βγαίνοντας, σε σχέση με το μέγιστο χρόνο παρακαρίσματος(ο αριθμός των επιβατών που θα επιβιστούν
αποφασίζεται τυχαία). Μέχρι και τη στιγμή πριν φύγουν από το σταθμό γράφουν πληροφορίες στη κοινή μνήμη.