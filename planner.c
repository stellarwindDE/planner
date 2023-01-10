#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>

#define MAX_INPUT_LENGTH 100

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


// Function to create a new appointment
Appointment* newAppointment(time_t start, const char *description)
{
    Appointment *appointment = (Appointment*) malloc(sizeof(Appointment));
    if(appointment == NULL){
        printf("FATAL ERROR: memory exhausted, could not create a new appointment.");
        return NULL;
    }
    appointment->start = start;
    appointment->description = description;

    return appointment;
}

// Function to create a new list
List* newList()
{
    List *list = (List*) malloc(sizeof(List));
    if(list == NULL){
        printf("FATAL ERROR: memory exhausted, could not create a new list.");
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
        printf("FATAL ERROR: memory exhausted, could not append appointment to the list");
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
        printf("FATAL ERROR: memory exhausted, could not insert the appointment into the list");
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

    // Read the file line by line
    char line[256];
    while (fgets(line, sizeof(line), file) != NULL)
    {
        // Parse the start time and description from the line
        long start;
        char description[256];

        // Check if sscanf was able to read 2 parameters from the line, ignore the line otherwise
        if(sscanf(line, "%ld,%[^\n]", &start, description) == 2){
            printf("read appointment %d - %s\n", start, description);
            if(start > curr_time){
                // Create a new appointment using the parsed data
                char * desc = malloc(sizeof(description));
                if(desc == NULL){
                    printf("FATAL ERROR: memory exhausted, could not read the full list.");
                    return list;
                }
                //*desc = description;
                strncpy(desc, description, sizeof(description));
                Appointment *appointment = newAppointment(start, desc);

                // Add the appointment to the list
                addAppointment(list, appointment);
            }else{
                appointments_skipped++;
            }
        }else{
            file_damaged = true;
        }

    }

    // Close the file
    fclose(file);

    // Display some status information
    if (appointments_skipped > 0)
        printf("Skipped %d appointments because they were too old.\n", appointments_skipped);
    if (file_damaged)
        printf("The file %s seems to be damaged, some data might not be available as expected.\n", filename);

    return list;
}

// Function to clear the list from memory and release allocated memory
void clearList(List *list)
{
    Element *current = list->head;
    while (current != NULL)
    {
        Element *next = current->next;

        // Free the memory allocated for the appointment
        free(current->appointment);

        // Free the memory allocated for the element
        free(current);

        current = next;
    }

    // Set the head and tail of the list to NULL
    list->head = list->tail = NULL;
}


// Function to display the appointments in the list
void displayList(List* list, int day, int month, int year)
{
    Element *current = list->head;
    while (current != NULL)
    {
        Appointment *appointment = current->appointment;

        // Check if the appointment should be printed
        int print = 0;
        if (day == 0 && month == 0 && year == 0)
        {
            // If all the parameters are 0, print the entire list
            print = 1;
        }
        else
        {
            // Get the start time of the appointment
            struct tm *start = localtime(&appointment->start);

            // Check if the start time matches the specified day, month, and year
            if (start->tm_mday == day && start->tm_mon + 1 == month && start->tm_year + 1900 == year)
            {
                print = 1;
            }
        }

        if (print)
        {
            // Print the start time and description of the appointment to the console
            printf("----\n%s -> %s\n", ctime(&appointment->start), appointment->description);
        }

        current = current->next;
    }
    printf("----\n");
}


/*int main(){
    List* l = newList();
    insertAppointment(l, newAppointment(4758547572578, "droelfzig"));
    insertAppointment(l, newAppointment(47585475725784678, "andere beschreibung..."));
    insertAppointment(l, newAppointment(8547572578, "drog kleinste"));

    displayList(l, 0, 0, 0);

    saveList(l, "tst.csv");

    clearList(l);


    return 0;
}*/

void menu(List* list){
    char input[MAX_INPUT_LENGTH];

    // Loop until the user quits
    while (1) {
        printf("Enter a command: \n");

        // Read the user's input
        scanf("%s", input);

        // Check for the different commands and perform the appropriate action
        if (strcmp(input, "create") == 0) {
            addAppointment(list, newAppointment(0, "sdddddddd"));
            printf("You have chosen to create a new appointment.\n");
        } else if (strcmp(input, "delete") == 0) {
            printf("You have chosen to delete an appointment.\n");
        } else if (strcmp(input, "search") == 0) {
            printf("You have chosen to search for an appointment.\n");
        } else if (strcmp(input, "list") == 0) {
            displayList(list, 0, 0, 0);
            printf("You have chosen to list all appointments.\n");
        } else if (strcmp(input, "quit") == 0) {
            printf("Exiting program.\n");
            break;
        } else if (strcmp(input, "menu") == 0) {
            printf("Available commands: \n");
            printf("  create - create a new appointment  \n");
            printf("  delete - delete a appointment from file \n");
            printf("  search - search for a appointment  \n");
            printf("  list - list all appointments  \n");
            printf("  quit - exit the program\n");
            printf("  menu - show this menu\n");
        } else {
            printf("Unrecognized command. Try 'menu' for a list of supported commands\n");
        }
    }
}


int main(int argc, char** argv) {

  // Get the filename from the parameter
  char* filename;

  // Check if a filename was passed as a parameter
  if (argc < 2) {
    printf("No filename provided, using 'termine.txt'\n");
    filename = "termine.txt";
  }else{
    filename = argv[1];
  }

  List* l = readList(filename);

  menu(l);

  saveList(l, filename);
  clearList(l);

  return 0;
}