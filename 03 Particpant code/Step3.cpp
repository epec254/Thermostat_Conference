// This #include statement was automatically added by the Spark IDE.
//This will allow for future support of the LED matrixes
#include "Therm_LED_Library/Therm_LED_Library.h"
//These next 2 lines are required by spark
#include "application.h"
#include <math.h>

//now we will declare your team name
int core_id = 1; //change to your spark team ID (1 - 30)
char *teamName = "TeamName"; //max length 20 characters!!!

//now you need to ask your hardware team what PIN your thermostat is on 
//change XX in the code below to the pin number, e.g., D6, etc
//(if your hardware team tells you thermostat is connected to D6 code would be as follows)
//#define THERM_PIN   D6
#define THERM_PIN   A7
#define I2C_D xx
#define I2C_C xx


//static definitions to identify which pin belongs to which device
//DO NOT CHANGE
#define MATRIX1_I2C 0x70
#define MATRIX2_I2C 0x71
#define MATRIX3_I2C 0x72

#define MAX_TEMP 40
#define MIN_TEMP 0
#define DESIRED_TEMP_FLASH_ADDRESS 0x80000
Adafruit_8x8matrix matrix1;
Adafruit_8x8matrix matrix2;
Adafruit_8x8matrix matrix3;

//DO NOT CHANGE THESE NEXT LINES
//These store the current state of the program
//variable to store current temperature (the room)
int currentTemperature = 0;
char logResult[200] = "";
bool thermostatStatus = false;
char statusString[100] = "";
char *contestStatus = "step3";

static const uint8_t off_nomotion[] =
{   0b00000000,
    0b00000000,
    0b11101111,
    0b10101000,
    0b10101111,
    0b10101000,
    0b11101000,
    0b00000000 };
    
static const uint8_t on_nomotion[] =
{   0b00000000,
    0b00000000,
    0b11101001,
    0b10101101,
    0b10101011,
    0b11101001,
    0b00000000,
    0b00000000 };

void displayTemperature(void);
void saveTemperature();
void loadTemperature();
int setTemperature(int t);
int setTemperatureFromString(String t);
void setupMatrix(Adafruit_8x8matrix m);
int adjustThermostatStatus(String status);



//run by the Spark Core upon start - configures all PINs & web interface
void setup()
{
    Wire.begin();
    
    //initial setup of the LED matrixes
    matrix1.begin(MATRIX1_I2C);
    matrix2.begin(MATRIX2_I2C);
    matrix3.begin(MATRIX3_I2C);
    
    setupMatrix(matrix1);
    setupMatrix(matrix2);
    setupMatrix(matrix3);
    
  
    //allow temperature (room & setting) to be read from web interface
    Spark.variable("current_temp", &currentTemperature, INT);

    //Variable that Google docs will read
    Spark.variable("log_data", &logResult, STRING);
    
    Serial.begin(9600);
    
    //themometer pin recieves voltage (INPUT)
    pinMode(THERM_PIN, INPUT);
    
    sprintf(statusString, "{\"id\":%d,\"team\":\"%s\",\"contest_status\":\"%s\"}", core_id, teamName, contestStatus);
    Spark.publish("contest-status",statusString);
    
}



void loop()
{
    //counter to allow us to only check the temp every so often
    static int wait = 0;
    
    int tempReading = 0;
    double voltage = 0.0;
  
    //only check & update temp every so often
    if (!wait)
    {
        wait = 1000;
        
        //TEMPERATE READING
        //read temp raw data
        tempReading = analogRead(THERM_PIN);
        voltage = (tempReading * 3.3) / 4095;
        //get the temp as degrees C
        double doubleTemp = (voltage - 0.5) * 100;
        //convert c to f
        //doubleTemp = ((doubleTemp * 9) / 5) + 32;
        // cast to int (e.g., change 75.44 to 75)
        currentTemperature = round(doubleTemp);
        
         char ones = currentTemperature % 10;
    	char tens = (currentTemperature / 10) % 10;
    
    	matrix1.clear();
    	matrix1.setCursor(0, 0);
    	matrix1.write(tens + '0');
    	matrix1.writeDisplay();
    
    	matrix2.clear();
    	matrix2.setCursor(0, 0);
    	matrix2.write(ones + '0');
    	matrix2.writeDisplay();
        
    }
    
    
     //DISPLAY STATUS 
    matrix3.clear();
    if (thermostatStatus == true) {
        //on & motion
        matrix3.drawBitmap(0, 0, on_nomotion, 8, 8, LED_ON);
    } else if (thermostatStatus == false) {
        //off & motion
        matrix3.drawBitmap(0, 0, off_nomotion, 8, 8, LED_ON);
    } 
    matrix3.writeDisplay();
        
    //logging result for Google
    sprintf(logResult, "{\"id\":%d,\"team\":\"%s\",\"c_tmp\":%d,\"contest_status\":\"%s\",\"status\":\"%s\"}", core_id, teamName, currentTemperature, contestStatus, thermostatStatus ? "on" : "off");
    
    --wait;
}



//helper function to set up the matrixes
void setupMatrix(Adafruit_8x8matrix m)
{
    m.clear();
    m.writeDisplay();
    m.setTextSize(1);
    m.setTextWrap(false);
    m.setTextColor(LED_ON);
    m.setRotation(0);
    m.setCursor(0, 0);
}

