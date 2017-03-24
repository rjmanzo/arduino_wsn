// Date and time functions using a RX8025 RTC connected via I2C and Wire lib

#include <Wire.h>
#include "Sodaq_DS3231.h"

#define POWA 9 // pin 9 power suplite line

char weekDay[][4] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };

//year, month, date, hour, min, sec and week-day(starts from 0 and goes to 6)
//writing any non-existent time-data may interfere with normal operation of the RTC.
//Take care of week-day also.
//DateTime dt(2011, 11, 10, 15, 18, 0, 5);

void setup () 
{
    Serial.begin(9600);
    Wire.begin();
    
    //POWER LINE CONFIG 
    pinMode(POWA, OUTPUT);
    digitalWrite(POWA, HIGH); //HABILITO LA LINEA POWER ENVIANDO UN HIGH A LA BASE DEL TRANSISTOR
    delay(50);
    
    //RTC CONFIG
    rtc.begin();
    rtc.setDateTime(DateTime(__DATE__, __TIME__));
    //rtc.setDateTime(dt); //Adjust date-time as defined 'dt' above 
}

void loop () 
{
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
    delay(1000);
}