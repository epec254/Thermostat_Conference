// This #include statement was automatically added by the Spark IDE.
//This will allow for future support of the LED matrixes
#include "Therm_LED_Library/Therm_LED_Library.h"
//These next 2 lines are required by spark
#include "application.h"
#include <math.h>

//now we will declare your team name
int core_id = 1; //change to your spark team ID (1 - 30)
char *teamName = "Demo Unit"; //max length 20 characters!!!

//now you need to ask your hardware team what PIN your different items are on 
//change XX in the code below to the pin number, e.g., D6, etc
//(if your hardware team tells you thermostat is connected to D6 code would be as follows)
//#define THERM_PIN   D6
//button toggles
#define COOL_BUTTON D2
#define HEAT_BUTTON D3
//LEDs within the buttons
#define COOL_LED D5
#define HEAT_LED D6
//Thermometer
#define THERM_PIN   A7
//Motion sensor
#define MOTION D4

//static definitions to identify which pin belongs to which device
//DO NOT CHANGE
#define MATRIX1_I2C 0x70
#define MATRIX2_I2C 0x71
#define MATRIX3_I2C 0x72
#define I2C_D D0
#define I2C_C D1
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
//variable to store desired temperature (the setting)
int desiredTemperature = 0;
//should the heat be on?
bool isHeatOn = false;
//should the AC be on?
bool isCoolOn = false;
//is motion currently detected?
bool motionDetected = false;
//last time motion detected
unsigned long lastMotionSensed = 0UL;
//UNIX timestamp of last motion sense
int lastMotionTimeStamp = 0;
//to log data via google apps
char logResult[200] = "";
//to allow motion to only be published to the cloud once every 15 seconds
unsigned long lastMotionPublish = 0UL;
char idForPublishMotion[2] = "";
//last state of cool / heat buttons
int lastHeatButtonState = 0;
int lastCoolButtonState = 0;
//to allow thermostat to turn on / off
//true = on; false = off
bool thermostatStatus = true;
//status variable
char statusString[100] = "";
char *contestStatus = "step5";

static const uint8_t on_motion[] = {
    0b11111111,
    0b00000000,
    0b11101001,
    0b10101101,
    0b10101011,
    0b11101001,
    0b00000000,
    0b11111111
};

static const uint8_t off_motion[] =
{   0b11111111,
    0b00000000,
    0b11101111,
    0b10101000,
    0b10101111,
    0b11101000,
    0b00000000,
    0b11111111 };

static const uint8_t on_nomotion[] =
{   0b00000000,
    0b00000000,
    0b11101001,
    0b10101101,
    0b10101011,
    0b11101001,
    0b00000000,
    0b00000000 };
static const uint8_t off_nomotion[] =
{   0b00000000,
    0b00000000,
    0b11101111,
    0b10101000,
    0b10101111,
    0b11101000,
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
    
    //allow temperature to be set via web interface
    Spark.function("set_temp", setTemperatureFromString);
    
    //allow thermostat to be turned on / off from web interface
    Spark.function("set_status", adjustThermostatStatus);
    
    //allow temperature (room & setting) to be read from web interface
    Spark.variable("current_temp", &currentTemperature, INT);
    Spark.variable("desired_temp", &desiredTemperature, INT);
    
    //allow status of heating & cooling & motion to be read from web interface
    Spark.variable("is_heat_on", &isHeatOn, BOOLEAN);
    Spark.variable("is_cool_on", &isCoolOn, BOOLEAN);
    Spark.variable("motion", &motionDetected, BOOLEAN);
    
    //Variable that Google docs will read
    Spark.variable("log_data", &logResult, STRING);
    
    Serial.begin(9600);
    
    loadTemperature();
    
    //LED pins send voltage (OUTPUT)
    pinMode(COOL_LED, OUTPUT);
    pinMode(HEAT_LED, OUTPUT);
    
    //themometer pin recieves voltage (INPUT)
    pinMode(THERM_PIN, INPUT);
    
    //temp setting buttons recieve voltage (INPUT)
    pinMode(COOL_BUTTON, INPUT);
    pinMode(HEAT_BUTTON, INPUT);
    
    //motion senior recieves voltage (INPUT)
    pinMode(MOTION, INPUT);
    
    //default LEDs to off
    digitalWrite(COOL_LED,LOW);
    digitalWrite(HEAT_LED,LOW);
    
    sprintf(statusString, "{\"id\":%d,\"team\":\"%s\",\"contest_status\":\"%s\"}", core_id, teamName, contestStatus);
    Spark.publish("contest-status",statusString);
}



void loop()
{
    //counter to allow us to only check the temp every so often
    static int wait = 0;
    
    static int firstTimeOn = 1;
    
    int tempReading = 0;
    double voltage = 0.0;
    
    int motionReading = 0;
    
    int heatButtonState = 0;
    int coolButtonState = 0;
    
    
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
    }
    
    //MOTION READING
    motionReading = digitalRead(MOTION);
    if (motionReading == HIGH) {
        motionDetected = true;
        //store last time sensed
        lastMotionSensed = millis();
        lastMotionTimeStamp = Time.now();
        
        //only publish every 15 seconds
        unsigned long now = millis();
        if ((now - lastMotionPublish)>15000UL) {
            sprintf(idForPublishMotion, "%d", core_id);
            Spark.publish("contest-motion",idForPublishMotion);
            lastMotionPublish = now;
        }
    } else {
        motionDetected = false;
    }
    
    
    //BUTTONS FOR TEMP ADJUSTMENT
    heatButtonState = digitalRead(HEAT_BUTTON);
    coolButtonState = digitalRead(COOL_BUTTON);
    
    if(heatButtonState == HIGH) { //button pressed
        
        if (lastHeatButtonState != heatButtonState) { //changed from previous state
            //this logic prevents erratic behavior from the user holding the button down
            //TODO: Implement logic to allow user to hold button down for quick changes
            
            //change the temperature
            if (desiredTemperature < MAX_TEMP) {
                setTemperature(desiredTemperature + 1);
            }
        }
        
        //save last button state
        lastHeatButtonState = heatButtonState;
    } else { //button not pressed
        //save last button state
        lastHeatButtonState = heatButtonState;
        
    }
    
    if(coolButtonState == HIGH) { //button pressed
        if (lastCoolButtonState != coolButtonState) { //changed from previous state
            //this logic prevents erratic behavior from the user holding the button down
            //TODO: Implement logic to allow user to hold button down for quick changes
            
            //change the temperature
            if (desiredTemperature > MIN_TEMP) {
                setTemperature(desiredTemperature - 1);
            }
        }
        
        //save last button state
        lastCoolButtonState = coolButtonState;
    } else { //button not pressed
        //save last button state
        lastCoolButtonState = coolButtonState;
        
    }
    
    //DISPLAY STATUS & MOTION
    matrix3.clear();
    if (motionDetected == true && thermostatStatus == true) {
        //on & motion
        matrix3.drawBitmap(0, 0, on_motion, 8, 8, LED_ON);
    } else if (motionDetected == true && thermostatStatus == false) {
        //off & motion
        matrix3.drawBitmap(0, 0, off_motion, 8, 8, LED_ON);
    } else if (motionDetected == false && thermostatStatus == true) {
        //on & NO motion
        matrix3.drawBitmap(0, 0, on_nomotion, 8, 8, LED_ON);
    } else if (motionDetected == false && thermostatStatus == false) {
        //off & NO motion
        matrix3.drawBitmap(0, 0, off_nomotion, 8, 8, LED_ON);
    }
    matrix3.writeDisplay();
    
    
    
    //ADJUST THE HEATING / COOLING ELEMENTS
    isHeatOn = desiredTemperature > currentTemperature;
    if(isHeatOn == true && thermostatStatus == true)
    {
        digitalWrite(HEAT_LED, HIGH);
        
    }
    else
    {
        digitalWrite(HEAT_LED, LOW);
    }
    
    
    isCoolOn = desiredTemperature < currentTemperature;
    
    if(isCoolOn == true && thermostatStatus == true)
    {
        digitalWrite(COOL_LED, HIGH);
        
    }
    else
    {
        digitalWrite(COOL_LED, LOW);
    }
    
    
    //logging result for Google
    sprintf(logResult, "{\"id\":%d,\"team\":\"%s\",\"c_tmp\":%d,\"d_tmp\":%d,\"motion\":\"%s\",\"heat\":\"%s\",\"cool\":\"%s\",\"status\":\"%s\",\"last_motion\":%d,\"contest_status\":\"%s\"}", core_id, teamName, currentTemperature, desiredTemperature, ((millis() - lastMotionSensed) < 5000UL)  ? "true" : "false", isHeatOn ? "true" : "false", isCoolOn ? "true" : "false", thermostatStatus ? "on" : "off", lastMotionTimeStamp, contestStatus);
    

    --wait;
}


//HELPER FUNCTIONS BELOW - DO NOT MODIFY

//function to turn thermostat on / off from web interface
int adjustThermostatStatus(String status)
{
    
    if (status == "on" && thermostatStatus == false)
    {
        //if off, turn on
        thermostatStatus = true;
        Serial.print("Turn thermostat to status on");
        return 1;
    } else if (status == "off" && thermostatStatus == true)
    {
        //if on, turn off
        thermostatStatus = false;
        Serial.print("Turn thermostat to status off");
        return 0;
    } else if (status == "on" || status == "off")
    {
        //valid command, but no need to adjust status
        return -1;
    } else
    {
        //invalid command, do nothing
        return -2;
    }
    
}


//function to display the room temperate on the LED boards
void displayTemperature(void)
{
    char ones = desiredTemperature % 10;
    char tens = (desiredTemperature / 10) % 10;
    
    matrix1.clear();
    matrix1.setCursor(0, 0);
    matrix1.write(tens + '0');
    matrix1.writeDisplay();
    
    matrix2.clear();
    matrix2.setCursor(0, 0);
    matrix2.write(ones + '0');
    matrix2.writeDisplay();
}

//helper function to save the desired temperate set by user
void saveTemperature()
{
    sFLASH_EraseSector(DESIRED_TEMP_FLASH_ADDRESS);
    Serial.println("Saving temperature to flash");
    uint8_t values[2] = { (uint8_t)desiredTemperature, 0 };
    sFLASH_WriteBuffer(values, DESIRED_TEMP_FLASH_ADDRESS, 2);
}

//helper function to load the desired temperate set by user
void loadTemperature()
{
    Serial.println("Loading and displaying temperature from flash");
    uint8_t values[2];
    sFLASH_ReadBuffer(values, DESIRED_TEMP_FLASH_ADDRESS, 2);
    desiredTemperature = values[0];
    displayTemperature();
}

//function for adjusting the temperate
int setTemperature(int t)
{
    desiredTemperature = t;
    displayTemperature();
    saveTemperature();
    return desiredTemperature;
}

//wrapper function for adjusting the temperate from the web interface
int setTemperatureFromString(String t)
{
    
    Serial.print("Setting desired temp from web to ");
    Serial.println(t);
    
    if (t.toInt() < MIN_TEMP || t.toInt() > MAX_TEMP) {
        //sanity check, if outside of given range, ignore it (will also catch strings)
        //TODO: More robust input validation!  (for those who understand security - you will immediatly recognize how insecure this unchecked input is!)
        return -1;
    } else {
        return setTemperature(t.toInt());
    }
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

