// Date and time functions using a RX8025 RTC connected via I2C and Wire lib

#include <Wire.h>
#include "Sodaq_DS3231.h"

/*********************************STATIC VARIABLES************************/
#define POWA_D 9 //switch pin of datalogger shield
#define SDcsPin 10 // pin 10 is CS pin for MicroSD breakout


char weekDay[][4] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };

//year, month, date, hour, min, sec and week-day(starts from 0 and goes to 6)
//writing any non-existent time-data may interfere with normal operation of the RTC.
//Take care of week-day also.
DateTime dt(2011, 11, 10, 15, 18, 0, 5);

void setup () 
{
    pinMode(POWA_D, OUTPUT);
    digitalWrite(POWA_D, HIGH); //Datalogger lines UP!
    Serial.begin(9600);
    Wire.begin();
    rtc.begin();
    rtc.setDateTime(DateTime(__DATE__, __TIME__));
    //rtc.setDateTime(dt); //Adjust date-time as defined 'dt' above 
}

void loop (){
  
    DateTime now = rtc.now(); //get the current date-time
    Serial.print(now.year(), DEC);
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.date(), DEC);
    Serial.print(' ');
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);
    Serial.println();
    Serial.print(weekDay[now.dayOfWeek()]);
    Serial.println();
    delay(1);
}
