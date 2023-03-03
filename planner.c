#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include <ctype.h>

//#include "timehelper.h"

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

void displayList(List* list, int day, int month, int year);
void insertAppointment(List *list, Appointment *appointment);
void clearList(List *list);
Element *findElement(List *list, char* query);
bool deleteElement(List *list, char* query);

void displayListEpoch(List* list, time_t time);
void addAppointment(List *list, Appointment *appointment);
void saveList(List *list, char *filename);
List* readList(char *filename);
List* newList();
Appointment* newAppointment(time_t start, const char *description);
void logMallocErr();
void toLowercase(char* str);
void clearStdin();
void readFromStdin(char* buffer, int len);
void menu(List* list);

void logMallocErr(){
    perror("FATAL ERROR: memory exhausted, malloc() failed.");
}

// Function to create a new appointment
Appointment* newAppointment(time_t start, const char *description)
{
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

// Function to create a new list
List* newList()
{
    List *list = (List*) malloc(sizeof(List));
    if(list == NULL){
        logMallocErr();
        return NULL;
    }
    list->head = list->tail = NULL;

    return list;
}

// Function to add an appointment to the end of the list
void addAppointment(List *list, Appointment *appointment)
{
    Element *element = (Element*) malloc(sizeof(Element));
    if(element == NULL){
        logMallocErr();
        return;
    }
    element->appointment = appointment;
    element->next = NULL;

    if (list->head == NULL)
    {
        list->head = list->tail = element;
    }
    else
    {
        list->tail->next = element;
        list->tail = element;
    }
}

// Function to add an appointment to the list
void insertAppointment(List *list, Appointment *appointment)
{
    Element *element = (Element*) malloc(sizeof(Element));
    if(element == NULL){
        logMallocErr();
        return;
    }
    element->appointment = appointment;
    element->next = NULL;

    if (list->head == NULL)
    {
        // If the list is empty, insert the appointment at the beginning
        list->head = list->tail = element;
    }
    else
    {
        Element *current = list->head;
        Element *previous = NULL;

        // Find the correct position to insert the appointment
        while (current != NULL && current->appointment->start < appointment->start)
        {
            previous = current;
            current = current->next;
        }

        // Insert the appointment at the correct position
        if (previous == NULL)
        {
            // If the appointment is the new head of the list
            element->next = list->head;
            list->head = element;
        }
        else
        {
            // If the appointment is in the middle or at the end of the list
            element->next = current;
            previous->next = element;
        }

        if (current == NULL)
        {
            // If the appointment is the new tail of the list
            list->tail = element;
        }
    }
}

// Function to save the list to a CSV file
void saveList(List *list, char *filename)
{
    // Open the file in write mode
    FILE *file = fopen(filename, "w");

    Element *current = list->head;
    while (current != NULL)
    {
        Appointment *appointment = current->appointment;

        // Write the start time and description of the appointment to the file
        fprintf(file, "%ld,%s\n", appointment->start, appointment->description);

        current = current->next;
    }

    // Close the file
    fclose(file);
}

// Function to read the list from a CSV file
List* readList(char *filename)
{
    // Create a new list
    List *list = newList();

    // Get current system time
    time_t curr_time = time(NULL);

    // Create counter to count skipped appointments
    int appointments_skipped = 0;

    // Log if there were issues while reading from the file
    bool file_damaged = false;

    // Open the file in read mode
    FILE *file = fopen(filename, "r");

    if(file != NULL){

        // Read the file line by line
        char line[256];
        while (fgets(line, sizeof(line), file) != NULL)
        {
            // Parse the start time and description from the line
            long start;
            char description[256];

            // Check if sscanf was able to read 2 parameters from the line, ignore the line otherwise
            if(sscanf(line, "%ld,%[^\n]", &start, description) == 2){
                //printf("read appointment %ld - %s\n", start, description);
                if(start > curr_time){
                    //allocate memory and copy the description, otherwise it would not be persistent
                    /*char * desc = malloc(sizeof(description));
                    if(desc == NULL){
                        printf("FATAL ERROR: memory exhausted, could not read the full list.\n");
                        return list;
                    }
                    strncpy(desc, description, sizeof(description));*/

                    // Create a new appointment using the parsed data
                    Appointment *appointment = newAppointment(start, description);

                    // Add the appointment to the list
                    if(appointment != NULL){
                        addAppointment(list, appointment);
                    }
                }else{
                    appointments_skipped++;
                }
            }else{
                file_damaged = true;
            }

        }

        // Close the file
        fclose(file);
    }else{
        printf("] %s couldn't be read. Does the file exist?\n", filename);
    }

    // Display some status information
    if (appointments_skipped > 0)
        printf("] Skipped %d appointments because they were too old.\n", appointments_skipped);
    if (file_damaged)
        printf("] The file %s seems to be damaged, some data might not be available as expected.\n Before issuing the command 'quit', make sure to create a copy of said file.\nUpon issuing the command, all data that couldn't be read will be lost.\n", filename);


    return list;
}

// Function to clear the list from memory and release allocated memory
void clearList(List *list)
{
    //TODO pseudoelemente freigeben
    Element *current = list->head;
    while (current != NULL)
    {
        Element *next = current->next;

        // Free the memory allocated for the description of the appointment
        //printf("freeing description\n");
        free(current->appointment->description);

        // Free the memory allocated for the appointment
        //printf("freeing appointment\n");
        free(current->appointment);

        // Free the memory allocated for the element
        //printf("freeing list element\n");
        free(current);

        current = next;
    }

    // Set the head and tail of the list to NULL
    list->head = list->tail = NULL;
}

void displayListEpoch(List* list, time_t time)
{
    struct tm* now = localtime(&time);
    displayList(list, now->tm_mday, now->tm_mon+1, now->tm_year+1900);
}

// Function to display the appointments in the list
void displayList(List* list, int day, int month, int year)
{
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

    //printf("time: %lld\n", time);
    //printf("%d day %d month %d year\n", st.tm_mday, st.tm_mon, st.tm_year);
    Element *current = list->head;
    if(current == NULL){
        printf("] List of appointments is empty.\n");
        return;
    }
    while (current != NULL)
    {
        Appointment *appointment = current->appointment;

        // Check if the appointment should be printed
        int print = false;
        if (printAll)
        {
            // If all the parameters are 0, print the entire list
            print = true;
        }
        else
        {
            /*// Get the start time of the appointment
            struct tm *start = localtime(&(appointment->start));

            // Check if the start time matches the specified day, month, and year
            if (start->tm_mday == day && start->tm_mon + 1 == month && start->tm_year + 1900 == year)
            {
                print = 1;
            }*/
            // 60s -> 60min -> 24h => 86400s in 1d
            if(appointment->start >= time && appointment->start <= time+86400){
                print = true;
            }
        }

        if (print)
        {
            if(!printAll && !appointmentFound){
                printf("] Listing appointments on %04d-%02d-%02d:\n", year, month, day);
                appointmentFound = true;
            }
            // Print the start time and description of the appointment to the console
            printf("----\n%s -> %s\n", ctime(&(appointment->start)), appointment->description);
        }

        current = current->next;
    }
    if(!appointmentFound && !printAll){
        printf("] No appointment was found on %04d-%02d-%02d.\n", year, month, day);
        return;
    }
    printf("----\n");
}

//Function which checks if every character in the provided string is a number(0-9) -> negative numbers will return false
void toLowercase(char* str)
{
    size_t n = 0;
    while (str[n] != '\0')
    {
        str[n] = tolower(str[n]);
        n++;
    }
}

Element *findElement(List *list, char* query)
{
    toLowercase(query);
    Element *current = list->head;
    char desc[MAX_INPUT_LENGTH];
    while (current != NULL)
    {
        strncpy(desc, current->appointment->description, strlen(current->appointment->description)+1);
        toLowercase(desc);
        if(strstr(desc, query) != NULL){
            return current;
        }
        current = current->next;
    }
    return NULL;
}

bool deleteElement(List *list, char* query)
{
    Element *toDelete = findElement(list, query);

    if(toDelete != NULL){
        Element *current = list->head;
        while (current != NULL)
        {
            if(current->next == toDelete){
                current->next = toDelete->next;
                free(toDelete->appointment->description);
                free(toDelete->appointment);
                return true;
            }
            current = current->next;
        }
        return false;
    }else{
        return false;
    }
}

void clearStdin()
{
    int c;
    while((c=getchar()) != '\n' && c != EOF){}
}

void readFromStdin(char* buffer, int len)
{

    fgets(buffer, len, stdin);

    size_t size = strlen(buffer);

    //truncate last char (\n)
    buffer[size-1] = '\0';

    if (size >= len-1){
        printf("] Please enter a maximum of %d characters!\n>", len-2);
        clearStdin();
        readFromStdin(buffer, len);
    }
}

void menu(List* list)
{
    char input[MAX_INPUT_LENGTH];

    // Loop until the user quits
    while (1) {
        printf("] Enter a command ('menu' will display a list of possible commands): \n>");

        // Read the user's input
        //scanf("%s", input);
        readFromStdin(input, MAX_INPUT_LENGTH);


        // Check for the different commands and perform the appropriate action
        if (strstr(input, "create") != NULL) {
            printf("] ------------------------\n");
            printf("] - Appointment creation -\n");
            printf("] ------------------------\n");
            time_t appTime = inputTime(false);
            printf("] entered date: %s\n] please enter a short description for your appointment: \n", ctime(&appTime));
            //fgets(input, MAX_INPUT_LENGTH, stdin);
            readFromStdin(input, MAX_INPUT_LENGTH);
            insertAppointment(list, newAppointment(appTime, input));
        } else if (strstr(input, "deleteall") != NULL) {
            printf("] Are you sure you want to delete all appointments? (y/n):");
            char c;
            if((c = getchar()) == 'y' || c == 'Y'){
                clearList(list);
                printf("] Deletion complete!\n");
            }else{
                printf("] Deletion aborted!\n");
            }
            clearStdin();
        } else if (strstr(input, "delete") != NULL) {
            printf("] Please enter your search term:\n>");
            readFromStdin(input, MAX_INPUT_LENGTH);
            printf("] Searching.. ");
            Element* ref = findElement(list, input);
            if(ref != NULL){
                printf(" Found!\n] Date: %s] Description: %s\n", ctime(&(ref->appointment->start)), ref->appointment->description);
            }else{
                printf(" Exhausted!\n] No appointment in the list matches your query.\n");
            }
            printf("] Are you sure you want to delete this appointment? (y/n):");
            char c;
            if((c = getchar()) == 'y' || c == 'Y'){
                deleteElement(list, input);
                printf("] Deletion complete!\n");
            }else{
                printf("] Deletion aborted!\n");
            }
            clearStdin();
        } else if (strstr(input, "search") != NULL) {
            printf("] Please enter your search term:\n>");
            readFromStdin(input, MAX_INPUT_LENGTH);
            printf("] Searching.. ");
            Element* ref = findElement(list, input);
            if(ref != NULL){
                printf(" Found!\n] Date: %s\n] Description: %s\n", ctime(&(ref->appointment->start)), ref->appointment->description);
            }else{
                printf(" Exhausted!\n  No appointment in the list matches your query.\n");
            }
            //printf("] You have chosen to search for an appointment.\n");
        } else if (strstr(input, "listtoday") != NULL) {
            displayListEpoch(list, time(NULL));
        } else if (strstr(input, "listday") != NULL) {
            displayListEpoch(list, inputTime(true));
        } else if (strstr(input, "list") != NULL) {
            displayList(list, 0, 0, 0);
        } else if (strstr(input, "quit") != NULL) {
            printf("] Exiting program.\n");
            return;
        } else if (strstr(input, "menu") != NULL) {
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
        }/* else {
            printf("] Unrecognized command. Try 'menu' for a list of supported commands\n");
        }*/
    }
}


int main(int argc, char** argv) {

  // Get the filename from the parameter
  char* filename;

  // Check if a filename was passed as a parameter
  if (argc < 2) {
    printf("] No filename provided, using 'termine.txt'\n");
    filename = "termine.txt";
  }else{
    filename = argv[1];
  }

  //printf("reading file..\n");
  List* l = readList(filename);
  //printf("done reading.\n");

  /*time_t nowEpoch = time(NULL);
  struct tm* now = localtime(&nowEpoch);
  displayList(l, now->tm_mday, now->tm_mon+1, now->tm_year+1900);*/
  displayListEpoch(l, time(NULL));
  menu(l);

  saveList(l, filename);
  clearList(l);

  //printf("-.-");
  return 0;
}

//Function which checks if every character in the provided string is a number(0-9) -> negative numbers will return false
bool isNumber(char* str){
    size_t n = 0;
    while (str[n] != '\0')
    {
        //0-9
        if(!isdigit(str[n])){
            printf("%c is not numeric\n", str[n]);
            return false;
        }
        n++;
    }
    return true;
}

//variadic function that returns true, should all arguments passed to the function be positive integers. false otherwise
bool containsNegative(int n, ...){
    // Declaring pointer to the
    // argument list
    va_list ptr;

    // Initializing argument to the
    // list pointer
    va_start(ptr, n);

    for (int i = 0; i < n; i++){
        // Accessing current variable
        // and pointing to next one
        int s = va_arg(ptr, int);
        if(s < 0){
            return true;
        }
    }

    // Ending argument list traversal
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

bool isValidDate(struct tm* ptr){
    struct tm check;
    memcpy(&check, ptr, sizeof(struct tm));
    bool parserOk = !containsNegative(6, ptr->tm_year, ptr->tm_mon, ptr->tm_mday, ptr->tm_hour, ptr->tm_min, ptr->tm_sec);
    bool successfulMktime = mktime(ptr) > -1;
    bool mktimeUnchanged = compareTm(ptr, &check);

    printf("validation result: [parser=%d, mktime1=%d, mktime2=%d]\n", parserOk, successfulMktime, mktimeUnchanged);

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
//returned pointer is allocated and needs to be freed
struct tm* parse_time(char* in, bool dateOnly){
    struct tm* time = (struct tm*) malloc(sizeof(struct tm));
    if(time == NULL){
        printf("] FATAL ERROR: memory exhausted, could not create time\n");
        return NULL;
    }
    int components[TIME_COMPONENTS];
    char* delimiter = "-: \n";
    components[0] = convertStrWithCheck(strtok(in, delimiter));

    for (int i = 1; i < TIME_COMPONENTS; ++i) {
        components[i] = convertStrWithCheck(strtok(NULL, delimiter));
        //printf("%d << %d\n", i, components[i]);
    }

    //perform sanity checks on entered Date:
    // Year: 2020-10000 (excluding)
    // Month: 0-13 (excluding)
    // Day: 0-32 (excluding)
    // Hour: 0-24 (including 0)
    // Minute: 0-60 (including 0)
    // Second: 0-60 (including 0)
    time->tm_year = (components[0] > 2020 && components[0] < 10000) ? components[0]-1900 : -1;
    time->tm_mon = (components[1] > 0 && components[1] < 13) ? components[1]-1 : -1;
    time->tm_mday = (components[2] > 0 && components[2] < 32) ? components[2] : -1;
    time->tm_hour = (components[3] >= 0 && components[3] < 24) ? components[3] : (dateOnly ? 0 : -1);
    time->tm_min = (components[4] >= 0 && components[4] < 60) ? components[4] : (dateOnly ? 0 : -1);
    time->tm_sec = (components[5] >= 0 && components[5] < 60) ? components[5] : (dateOnly ? 0 : -1);
    time->tm_isdst = -1; //-1 means we don't know whether the provided time has daylight savings or not

    return time;
}

//prompts the user to enter a time in a loop until a valid time has been entered
time_t inputTime(bool dateOnly){
    time_t curr_time = time(NULL);
    printf("] Time needs to be formatted according to ISO 8601: yyyy-mm-dd");printf(dateOnly ? "\n":" hh:mm:ss\n");
    char input[MAX_INPUT_LENGTH];
    struct tm *time = NULL;
    time_t unixtime;
    bool done = false;

    while(!done) {
        printf("] Please enter date");printf(dateOnly ? ":\n":" & time:\n");
        //fgets(input, 256, stdin);
        readFromStdin(input, MAX_INPUT_LENGTH);

        time = parse_time(input, dateOnly);

        //printf("mktime result: %ld\n", unixtime);
        //printf("tm: %d-%d-%d %d:%d:%d\n", time->tm_year, time->tm_mon, time->tm_mday, time->tm_hour, time->tm_min, time->tm_sec);
        //printf("dst: %d\n", time->tm_isdst);
        bool valid = isValidDate(time);
        if(time != NULL && valid && (mktime(time) > curr_time)){
            unixtime = mktime(time);
            //printf("tm: %d-%d-%d %d:%d:%d\n", time->tm_year, time->tm_mon, time->tm_mday, time->tm_hour, time->tm_min, time->tm_sec);
            //printf("dst: %d\n", time->tm_isdst);
            printf("ctime: %s\n", ctime(&unixtime));
            done = true;
        }else if(time != NULL && valid && (mktime(time) <= curr_time)){
            printf("] It is only possible to plan FUTURE appointments.\n");
        }else{
            printf("] Invalid time!\n");
        }

        free(time);
    }

    return unixtime;
}