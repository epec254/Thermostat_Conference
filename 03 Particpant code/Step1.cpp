// This #include statement was automatically added by the Spark IDE.
//This will allow for future support of the LED matrixes
#include "Therm_LED_Library/Therm_LED_Library.h"
//These next 2 lines are required by spark
#include "application.h"
#include <math.h>

//now we will declare your team name
int core_id = 1; //change to your spark team ID (1 - 30)
char *teamName = "TeamName"; //max length 20 characters!!!

//DO NOT CHANGE THESE NEXT LINES
//These store the current state of the program
int currentTemperature = 0;
char logResult[200] = "";
bool thermostatStatus = false;
char statusString[100] = "";
char *contestStatus = "step1";

//run by the Spark Core upon start - configures all PINs & web interface
void setup()
{
    Wire.begin();
    
    //Variable that Google docs will read to track team progress
    Spark.variable("log_data", &logResult, STRING);
    
    Serial.begin(9600);
    
    pinMode(D7, OUTPUT);
    
    sprintf(statusString, "{\"id\":%d,\"team\":\"%s\",\"contest_status\":\"%s\"}", core_id, teamName, contestStatus);
    Spark.publish("contest-status",statusString);
}



void loop()
{
    //counter to allow us to only check the temp every so often
    static int wait = 0;
    
 
    digitalWrite(D7, HIGH);   // Turn ON the LED

    
    //logging result for Google
    //TODO: Check if Google doc can read this partial set of strings
    sprintf(logResult, "{\"id\":%d,\"team\":\"%s\",\"contest_status\":\"%s\",\"status\":\"%s\"}", core_id, teamName, contestStatus, thermostatStatus ? "on" : "off");
    
    --wait;
}

