    /*
    * Arduino Wireless Communication Tutorial
    *       Example 1 - Receiver Code
    *                
    * by Dejan Nedelkovski, www.HowToMechatronics.com
    * 
    * Library: TMRh20/RF24, https://github.com/tmrh20/RF24/
    */
    #include <SPI.h>
    #include <nRF24L01.h>
    #include <RF24.h>
    #define POWA_W 6
    
    RF24 radio(8, 7); // CNS, CE
    
    //const byte address[6] = "00001";
    const uint64_t address = 0xE8E8F0F0E1LL;
        
    void setup() {
      //Wireless lines UP!
      pinMode(POWA_W, OUTPUT);
      digitalWrite(POWA_W,HIGH);

      //POR ERROR DE CONEXIONADO O ALGO POR EL ESTILO EL DATALOGGER MARCADO NO ANDA SINO SETEAMOS EN LOW EL PIN DIGITAL 9 (DATALOGGER LINE!)
      pinMode(9, OUTPUT);
      digitalWrite(9,LOW);
      
      Serial.begin(9600);
      radio.begin();
      radio.openReadingPipe(0, address);
      radio.setPALevel(RF24_PA_MIN);
      
      radio.startListening();
    }
    
    void loop() {
      //Serial.println("estoy en eso");
      char text[25] = "NADA"; 
      if (radio.available()) {
        //Serial.println("entro");
        radio.read(&text, sizeof(text));
        //char text[25] = "NADA";
        //delay(100);
        //int entero = 0;
        //radio.read(&entero, sizeof(entero));
        //Serial.println(entero);
      }
      Serial.println(text);
      //delay(100);
    }
