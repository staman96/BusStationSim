Μανωλάς Σταμάτιος 1115201400094 Λειτουργικά Συστήματα Εργασία 3η

Γενικά:

	Έχω ακολουθήσει τις οδηγίες της εκφώνησης και είδα και κάποιες διορθώσεις που υπήρχαν στο φόρουμ. Το πρόγραμμα 
τρέχει ως εξής: ./mystation -l config.txt -f 2 όπου το -f είναι η συχνότητα δημιουργίας διεργασιών λεωφορείων. Τα 
ορίσματα είναι προαιρετικά καθώς όλα έχουν αρχική τιμή. Το πρόγραμμα έχει γραφτεί σε C++. Το compilation γίνεται με 
την εντολή make, και η make clean σβήνει τα .ο αρχεία και τα εκτελέσιμα. Το αρχείο εξόδου είναι το log.txt στο /IO.
Επίσης, στο makefile υπάρχει η επιλογή διαγραφής του αρχείου εξόδου από το φάκελο IO/ με την εντολή make cleanOuts.
Το πρόγραμμα αποδευσμεύει όλη τη μνήμη κανονικά, και το έχω τρέξει αρκετά με valgrind και δε μου βγάζει κανένα 
πρόβλημα με όλα τα input files. Δοκιμάστηκε, επίσης, και στο linux09 της σχολής και έτρεχε χωρίς θέμα. Ενδεικτικά:
make clean && make && valgrind -v --leak-check=yes --show-leak-kinds=all --track-origins=yes --trace-children=yes ./mystation -l config.txt -f 2

Δομή Κοινής Μνήμης:
	Αρχικά είναι το class stationDATA που περιέχει όλους τους σεμαφόρους, και τις μεταβλητές για την επικοινωνία
ανάμεσα στο εισερχόμενο/εξερχόμενο λεωφορείο. Έπειτα, είναι ένας πίνακας από νησίδες (class Bay) με διαφορετικό 
αριθμό από θέσεις παρκαρίσματος λεωφορείων(class Bus), που είναι στο επόμενο κομμάτι μνήμης.

Σεμαφόροι:

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