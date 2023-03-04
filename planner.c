#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include <ctype.h>

bool isNumber(char* str);
bool containsNegative(int n, ...);
bool compareTm(struct tm *elementA, struct tm *elementB);
bool isValidDate(struct tm* ptr);
int convertStrWithCheck(char* toConvert);
struct tm* parse_time(char* in, bool dateOnly);
time_t inputTime(bool dateOnly);

#define MAX_INPUT_LENGTH 255
#define TIME_COMPONENTS 6

typedef struct
{
    time_t start;
    char *description;
} Appointment;

typedef struct Element
{
    Appointment *appointment;
    struct Element *next;
} Element;

typedef struct
{
    Element *head, *tail;
} List;

List createList();
void clearList(List list);
void insertAppointment(List list, time_t start, const char* description);
Element *findElement(List list, const char* query);
bool deleteElement(List list, const char* query);
void printAppointment(Appointment *toPrint);
void printList(List list, int day, int month, int year);

void displayListEpoch(List list, time_t time);
void saveList(List list, char *filename);
List readList(char *filename);
Appointment* newAppointment(time_t start, const char *description);
void logMallocErr();
void toLowercase(char* str);
void clearStdin();
void readFromStdin(char* buffer, int len);
void menu(List list);

//log an error to stderr after malloc failed to allocate new memory
void logMallocErr(){
    fprintf(stderr, "FATAL ERROR: memory exhausted, malloc() failed.\n");
}

//create a new appointment starting at the given time 'start'
//allocate memory for the structure & description usinc malloc() and return a pointer to said structure
Appointment* newAppointment(time_t start, const char *description){
    Appointment *appointment = (Appointment*) malloc(sizeof(Appointment));
    if(appointment == NULL){
        logMallocErr();
        return NULL;
    }
    appointment->start = start;
    appointment->description = malloc(strlen(description)+1);
    if(appointment->description == NULL){
        logMallocErr();
        return NULL;
    }
    strncpy(appointment->description, description, strlen(description)+1);

    return appointment;
}

//create & return an empty list
List createList(){
    List list;
    Element *head = malloc (sizeof (Element));
    Element *tail = malloc (sizeof (Element));
    if(head == NULL || tail == NULL){
        logMallocErr();
        exit(EXIT_FAILURE);
    }
    head->appointment = tail->appointment = NULL;
    head->next = tail->next = tail;
    list.head = head;
    list.tail = tail;

    return list;
}

//create a new appointment and insert it at the appropriate position in the given list
void insertAppointment(List list, time_t start, const char *description){
    Appointment* appointment = newAppointment(start, description);
    Element *element = (Element*) malloc(sizeof(Element));
    if(element == NULL){
        logMallocErr();
        return;
    }
    element->appointment = appointment;

    if(list.head->next == list.tail){
        element->next = list.tail;
        list.head->next = element;
    }else{
        Element *current = list.head->next;
        Element *previous = list.head;
        while(current->appointment != NULL && appointment->start > current->appointment->start){
            previous = current;
            current = current->next;
        }
        element->next = current;
        previous->next = element;
    }
}

// Function to save the list to a CSV file
void saveList(List list, char *filename){
    if(list.head->next != list.tail){
        FILE *file = fopen(filename, "w");

        Element *current = list.head->next;
        while (current->appointment != NULL){
            //y2k38-bug possible depending on data model and size of time_t.. %ld should be replaced with %lld
            fprintf(file, "%ld,%s\n", current->appointment->start, current->appointment->description);

            current = current->next;
        }
        fclose(file);
    }
}

// Function to read the appointment list from file
List readList(char *filename){
    List list = createList();
    time_t curr_time = time(NULL);

    int appointments_skipped = 0;
    bool file_damaged = false;

    FILE *file = fopen(filename, "r");

    if(file != NULL){
        char line[MAX_INPUT_LENGTH+50];
        while (fgets(line, sizeof(line), file) != NULL){
            long start;
            char description[256];
            //y2k38-bug possible depending on data model and size of time_t %ld should be replaced with %lld(+ l.150: long long start;)
            if(sscanf(line, "%ld,%[^\n]", &start, description) == 2){ // Check if sscanf was able to read both parameters from the line, ignore the line otherwise
                if(start > curr_time){
                    Appointment *appointment = newAppointment(start, description);
                    if(appointment != NULL){
                        insertAppointment(list, start, description);
                    }
                }else{
                    appointments_skipped++;
                }
            }else{
                file_damaged = true;
            }
        }

        fclose(file);
    }else{
        fprintf(stderr, "ERROR: %s couldn't be read. Does the file exist?\n", filename);
    }

    // Display some status information
    if (appointments_skipped > 0)
        printf("] Skipped %d appointments because they expired.\n", appointments_skipped);
    if (file_damaged)
        printf("] The file %s seems to be damaged, some data might not be available as expected.\n Before issuing the command 'quit', make sure to create a copy of said file.\nUpon issuing the command, all data that couldn't be read will be lost.\n", filename);

    return list;
}

// empty the provided list and release the allocated memory of all included items
void clearList(List list){
    if(list.head->next != list.tail){
        Element *current = list.head->next;
        while (current->appointment != NULL){
            Element *next = current->next;

            free(current->appointment->description);
            free(current->appointment);
            free(current);

            current = next;
        }
        list.head->next = list.tail;
    }
}

//prints all appointments to console which satisfy the following criteria:
// - the start time of the appointment is on the same day as the provided argument time
void displayListEpoch(List list, time_t time){
    struct tm* now = localtime(&time);
    printList(list, now->tm_mday, now->tm_mon+1, now->tm_year+1900);
}

//prints information of the given appointment to stdout in a single line
void printAppointment(Appointment *toPrint){
    struct tm* now = localtime(&(toPrint->start));
    printf("%04d-%02d-%02d %02d:%02d:%02d // Description: %s\n", now->tm_year+1900, now->tm_mon+1, now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec, toPrint->description);
}

/* Function to display the appointments in the given list:
 * if all 3 integer arguments are set to zero, list every appointment in the list
 * otherwise, print only those appointments, which happen to have their start time on the provided day.
 * (all elements in the provided list have to be sorted by start time of appointment in ascending order)*/
void printList(List list, int day, int month, int year){
    bool printAll = day == 0 && month == 0 && year == 0;
    bool appointmentFound = false;

    struct tm st;
    time_t time;
    if(!printAll) {
        st.tm_year = year-1900;
        st.tm_mon = month-1;
        st.tm_mday = day;
        st.tm_isdst = -1;
        st.tm_sec = st.tm_min = st.tm_hour = 0;
        time = mktime(&st);
    }

    if(list.head->next == list.tail){
        printf("] List of appointments is empty.\n");
    }else{
        Element *current = list.head->next;

        while (current->appointment != NULL){
            Appointment *appointment = current->appointment;

            // Check if the appointment should be printed
            bool print = false;
            if (printAll){
                print = true; // If all the parameters are 0, print the entire list
            }else if(appointment->start >= time){
                if(appointment->start <= time+86400){ // 60s -> 60min -> 24h => 86400s in 1d
                    print = true;
                }else{
                    break;
                }
            }

            if (print){
                if(!printAll && !appointmentFound){
                    printf("] Listing appointments on %04d-%02d-%02d:\n", year, month, day);
                    appointmentFound = true;
                }
                printf("----\n");
                printAppointment(appointment);
            }

            current = current->next;
        }
        if(!appointmentFound && !printAll){
            printf("] No appointment was found on %04d-%02d-%02d.\n", year, month, day);
            return;
        }
        printf("----\n");
    }
}

//turn every character in the provided string into its lowercase version
void toLowercase(char* str){
    size_t n = 0;
    while (str[n] != '\0'){
        str[n] = tolower(str[n]);
        n++;
    }
}

//find the first element in 'list' whose appointment description matches 'query'(case-insensitive)
//returns a pointer to the element or NULL if no matching element was found in 'list'
Element *findElement(List list, const char* query){
    if(list.head->next != list.tail){
        int size = strlen(query);
        char tmp[size+1];
        strncpy(tmp, query, size);
        toLowercase(tmp);
        char desc[MAX_INPUT_LENGTH];
        Element *current = list.head->next;
        while (current->appointment != NULL)
        {
            strncpy(desc, current->appointment->description, strlen(current->appointment->description)+1);
            toLowercase(desc);
            if(strstr(desc, tmp) != NULL){
                return current;
            }
            current = current->next;
        }
    }
    return NULL;
}

//use findElement() with the provided arguments to find a matching element in 'list'
//if found, remove the element from 'list' and free associated memory then return true
//otherwise, return false
bool deleteElement(List list, const char* query){
    Element *toDelete = findElement(list, query);
    bool found = (toDelete != NULL);
    bool listNotEmpty = (list.head->next != list.tail);

    if(found && listNotEmpty){
        Element *current = list.head->next;
        while (current->appointment != NULL){
            if(current->next == toDelete){
                current->next = toDelete->next;
                free(toDelete->appointment->description);
                free(toDelete->appointment);
                free(toDelete);
                return true;
            }
            current = current->next;
        }
    }
    return false;
}

//'flush' the input buffer
void clearStdin(){
    int c;
    while((c=getchar()) != '\n' && c != EOF){}
}

//write text input with a maximum length of 'len'-2 from stdin into 'buffer'
void readFromStdin(char* buffer, int len){
    fgets(buffer, len, stdin);

    size_t size = strlen(buffer);
    buffer[size-1] = '\0'; //truncate last char (\n)

    if (size >= len-1){
        fprintf(stderr, "ERROR: Please enter a maximum of %d characters\n", len-2);
        printf(">");
        clearStdin();
        readFromStdin(buffer, len);
    }
}

//start an interactive prompt in the console, allowing someone to manipulate 'list' via text commands
void menu(List list){
    char input[MAX_INPUT_LENGTH];

    // Loop until the user quits
    while (1){
        printf("] Enter a command ('menu' or '8' will display a list of possible commands): \n>");
        readFromStdin(input, MAX_INPUT_LENGTH);

        // Check for the different commands and perform the appropriate action
        if (!strcmp(input, "create") || !strcmp(input, "1")) {
            printf("] Starting appointment creation\n");
            time_t appTime = inputTime(false);
            printf("] Entered date: %s] Please enter a short description for your appointment: \n>", ctime(&appTime));
            readFromStdin(input, MAX_INPUT_LENGTH);
            insertAppointment(list, appTime, input);
        } else if (!strcmp(input, "deleteall") || !strcmp(input, "3")) {
            printf("] Are you sure you want to delete all appointments? (y/n):");
            char c;
            if((c = getchar()) == 'y' || c == 'Y'){
                clearList(list);
                printf("] Cleared all appointments!\n");
            }else{
                printf("] Operation aborted!\n");
            }
            clearStdin();
        } else if (!strcmp(input, "delete") || !strcmp(input, "2")) {
            printf("] Please enter your search term:\n>");
            readFromStdin(input, MAX_INPUT_LENGTH);
            printf("] Searching.. ");
            Element* ref = findElement(list, input);
            if(ref != NULL){
                printf(" Found!\n] Date: %s] Description: %s\n", ctime(&(ref->appointment->start)), ref->appointment->description);
            }else{
                printf(" Exhausted!\n] No appointment in the list matches your query\n");
                continue;
            }
            printf("] Are you sure you want to delete this appointment? (y/n):");
            char c;
            if((c = getchar()) == 'y' || c == 'Y'){

                printf(deleteElement(list, input) ? "] Deletion complete\n" : "] Deletion unsuccessful\n");
            }else{
                printf("] Deletion aborted\n");
            }
            clearStdin();
        } else if (!strcmp(input, "search") || !strcmp(input, "4")) {
            printf("] Please enter your search term:\n>");
            readFromStdin(input, MAX_INPUT_LENGTH);
            printf("] Searching.. ");
            Element* ref = findElement(list, input);
            if(ref != NULL){
                printf(" Found!\n] Date: %s] Description: %s\n", ctime(&(ref->appointment->start)), ref->appointment->description);
            }else{
                printf(" Exhausted!\n  No appointment in the list matches your query\n");
            }
        } else if (!strcmp(input, "listtoday") || !strcmp(input, "7")) {
            displayListEpoch(list, time(NULL));
        } else if (!strcmp(input, "listday") || !strcmp(input, "6")) {
            displayListEpoch(list, inputTime(true));
        } else if (!strcmp(input, "list") || !strcmp(input, "5")) {
            printList(list, 0, 0, 0);
        } else if (!strcmp(input, "quit") || !strcmp(input, "0")) {
            printf("] Exiting program\n");
            return;
        } else if (!strcmp(input, "menu") || !strcmp(input, "8")) {
            printf("] Available commands: \n");
            printf("] (0) quit - exit the program\n");
            printf("] (1) create - create a new appointment  \n");
            printf("] (2) delete - delete an appointment from file \n");
            printf("] (3) deleteall - delete all appointments  \n");
            printf("] (4) search - search for an appointment  \n");
            printf("] (5) list - list all appointments  \n");
            printf("] (6) listday - list appointments on a specific date  \n");
            printf("] (7) listtoday - list all appointments planned for today \n");
            printf("] (8) menu - show this menu\n");
        } else {
            fprintf(stderr, "ERROR: Unrecognized command\n");
        }
    }
}


int main(int argc, char** argv) {
  char* filename;
  if (argc < 2) { // Check if a filename was passed as a parameter
    printf("] No filename provided, using 'termine.txt'\n");
    filename = "termine.txt";
  }else{
    filename = argv[1];
  }

  List l = readList(filename);
  displayListEpoch(l, time(NULL));
  menu(l);

  saveList(l, filename);
  clearList(l);
  free(l.tail);
  free(l.head);

  return 0;
}

//Function which checks if every character in the provided string is a number(0-9) -> negative numbers will return false
bool isNumber(char* str){
    size_t n = 0;
    while (str[n] != '\0'){
        if(!isdigit(str[n])){
            fprintf(stderr, "ERROR: %c is not a number\n", str[n]);
            return false;
        }
        n++;
    }
    return true;
}

//variadic function that returns true, should all arguments passed to the function be positive integers. false otherwise
bool containsNegative(int n, ...){
    va_list ptr;
    va_start(ptr, n);

    for (int i = 0; i < n; i++){
        int s = va_arg(ptr, int);
        if(s < 0){
            return true;
        }
    }
    va_end(ptr);

    return false;
}

//compare two tm structs by their internal values. mktime() will correct out-of-bounds values, this behaviour can be used
//to determine if user-provided input was a valid date.
bool compareTm(struct tm *elementA, struct tm *elementB){
    bool year = (elementA->tm_year == elementB->tm_year);
    bool mon = (elementA->tm_mon == elementB->tm_mon);
    bool mday = (elementA->tm_mday == elementB->tm_mday);
    bool hour = (elementA->tm_hour == elementB->tm_hour);
    bool min = (elementA->tm_min == elementB->tm_min);
    bool sec = (elementA->tm_sec == elementB->tm_sec);
    return (year && mon && mday && hour && min && sec);
}

//check if the given tm struct represents a valid date
bool isValidDate(struct tm* ptr){
    struct tm check;
    memcpy(&check, ptr, sizeof(struct tm));
    bool parserOk = !containsNegative(6, ptr->tm_year, ptr->tm_mon, ptr->tm_mday, ptr->tm_hour, ptr->tm_min, ptr->tm_sec);
    bool successfulMktime = mktime(ptr) > -1;
    bool mktimeUnchanged = compareTm(ptr, &check);
    return (parserOk && successfulMktime && mktimeUnchanged);
}

//try to parse an integer from the provided string. this function is specific to time input and will therefore
//only accept strings with a length between 1 and 5 containing a positive number (single digit numbers need to be prefixed with "0")
int convertStrWithCheck(char* toConvert){
    if(toConvert != NULL){
        size_t length = strlen(toConvert);
        if(1 < length && length < 5 && isNumber(toConvert)){
            return atoi(toConvert);
        }
    }
    return -1;
}

//parse calendar time into a tm struct from an ISO 8601 standard-format string
//correct delimiters are NOT strictly enforced. recommended format is yyyy-mm-dd hh:mm:ss(ISO 8601 standard)
//but every delimiter will work in every position(e.g. yyyy mm dd:hh-mm-ss would work as well)
//order of units however has to remain consistent with decreasing significance from left to right
//returned tm struct(as pointer) is allocated with malloc() and needs to be freed
struct tm* parse_time(char* in, bool dateOnly){
    struct tm* time = (struct tm*) malloc(sizeof(struct tm));
    if(time == NULL){
        logMallocErr();
        return NULL;
    }
    int components[TIME_COMPONENTS];
    char* delimiter = "-: \n";
    components[0] = convertStrWithCheck(strtok(in, delimiter));

    for (int i = 1; i < TIME_COMPONENTS; ++i) {
        components[i] = convertStrWithCheck(strtok(NULL, delimiter));
    }

    //perform sanity checks on entered Date:
    time->tm_year = (components[0] > 2020 && components[0] < 10000) ? components[0]-1900 : -1;
    time->tm_mon = (components[1] > 0 && components[1] < 13) ? components[1]-1 : -1;
    time->tm_mday = (components[2] > 0 && components[2] < 32) ? components[2] : -1;
    time->tm_hour = (components[3] >= 0 && components[3] < 24) ? components[3] : (dateOnly ? 0 : -1);
    time->tm_min = (components[4] >= 0 && components[4] < 60) ? components[4] : (dateOnly ? 0 : -1);
    time->tm_sec = (components[5] >= 0 && components[5] < 60) ? components[5] : (dateOnly ? 0 : -1);
    time->tm_isdst = -1; //-1 means we don't know whether the provided time has daylight savings or not

    return time;
}

//prompts the user to enter a time in a loop until a valid time has been entered, afterwards return the entered time as time_t
time_t inputTime(bool dateOnly){
    time_t curr_time = time(NULL);
    printf("] Time needs to be formatted according to ISO 8601: yyyy-mm-dd");printf(dateOnly ? "\n":" hh:mm:ss\n");
    char input[MAX_INPUT_LENGTH];
    struct tm *time = NULL;
    time_t unixtime;
    bool done = false;

    while(!done) {
        printf("] Please enter date");printf(dateOnly ? ":\n>":" & time:\n>");
        readFromStdin(input, MAX_INPUT_LENGTH);

        time = parse_time(input, dateOnly);

        bool valid = isValidDate(time);
        if(time != NULL && valid && (mktime(time) > curr_time)){
            unixtime = mktime(time);
            done = true;
        }else if(time != NULL && valid && (mktime(time) <= curr_time)){
            fprintf(stderr, "ERROR: It is only possible to plan FUTURE appointments\n");
        }else{
            fprintf(stderr, "ERROR: Invalid time\n");
        }
        free(time);
    }

    return unixtime;
}