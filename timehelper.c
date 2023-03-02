//
// Created by MoritzGeffert on 10.01.2023.
//
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdarg.h>
#include "timehelper.h"
#define TIME_COMPONENTS 6

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
struct tm* parse_time(char* in){
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
    time->tm_hour = (components[3] >= 0 && components[3] < 24) ? components[3] : -1;
    time->tm_min = (components[4] >= 0 && components[4] < 60) ? components[4] : -1;
    time->tm_sec = (components[5] >= 0 && components[5] < 60) ? components[5] : -1;
    time->tm_isdst = -1; //-1 means we don't know whether the provided time has daylight savings or not

    return time;
}

//prompts the user to enter a time in a loop until a valid time has been entered
time_t inputTime(){
    time_t curr_time = time(NULL);
    printf("] Time needs to be formatted according to ISO 8601: yyyy-mm-dd hh:mm:ss\n");
    char input[256];
    struct tm *time = NULL;
    time_t unixtime;
    bool done = false;

    while(!done) {
        printf("] Please enter date & time:\n");
        fgets(input, 256, stdin);

        time = parse_time(input);

        //printf("mktime result: %ld\n", unixtime);
        printf("tm: %d-%d-%d %d:%d:%d\n", time->tm_year, time->tm_mon, time->tm_mday, time->tm_hour, time->tm_min, time->tm_sec);
        printf("dst: %d\n", time->tm_isdst);
        if(time != NULL && isValidDate(time) && (mktime(time) > curr_time)){
            unixtime = mktime(time);
            printf("tm: %d-%d-%d %d:%d:%d\n", time->tm_year, time->tm_mon, time->tm_mday, time->tm_hour, time->tm_min, time->tm_sec);
            printf("dst: %d\n", time->tm_isdst);
            printf("ctime: %s\n", ctime(&unixtime));
            done = true;
        }else if((mktime(time) <= curr_time)){
            printf("] It is only possible to plan FUTURE appointments.\n");
        }else{
            printf("] Invalid time!\n");
        }

        free(time);
    }

    return unixtime;
}

/*int main(){
    //printf("unix-time: %d\n", curr_time);
    //printf("entspricht %s", ctime(&curr_time));

    printf("%ld\n", inputTime());
    return 0;
}*/
