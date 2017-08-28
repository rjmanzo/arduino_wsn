
/*  Based on rRF24Network/examples/meshping from James Coliz, Jr. <maniacbug@ymail.com> 
    and Updated in 2014 - TMRh20
    
    Wireless sensor network (WSN) - coordinator node code

    Coder.
    Manzo Renato José - Ceneha (UNL), Santa Fe, Argentina, 2017
    
*/

/*
 sen_id | sen_descrip  | type_sen_id | type_sen_id | type_sen_descrip 
--------+--------------+-------------+-------------+------------------
      1 | DHT22        |           1 |           1 | Temperatura
      2 | DHT22        |           2 |           2 | Humedad
      3 | BMP183       |           4 |           4 | Presion
      4 | BMP183       |           1 |           1 | Temperatura
      5 | Tensión      |           5 |           5 | Batería
     99 | send data    |           - |           - | -
    100 | node_config  |           - |           - | -
    101 | go to sleep  |           - |           - | -
    102 | Sinc network |           - |           - | -

  (+) 99,100 & 101 doesnt exist. We use as flags
  
  nodo_id | nod_descrip 
---------+-------------
       1 | Nodo_0
       2 | Nodo_1
       3 | Nodo_2
       4 | Nodo_3
       5 | Nodo_4

 */
/*********************************LIBRARY*********************************/
#include <RF24Network.h>
#include <RF24.h>
#include <SPI.h>
#include <EEPROM.h>
#include <Wire.h>
#include <LowPower.h> //LowPower - Sleep mode
//#include <SD.h> //SD card
#include <ds3231.h> //DS3231
#include "WiFiEsp.h"
// Emulate Serial1 on pins 3/2 if not present
#ifndef HAVE_HWSERIAL1
#include "SoftwareSerial.h"
SoftwareSerial Serial1(3, 2); // RX, TX
#endif

/*********************************STATIC VARIABLES************************/
#define POWA_W 6 //switch pin of wireless shield
#define POWA_D 9 //switch pin of datalogger shield
//#define SDcsPin 10 // pin 10 is CS pin for MicroSD breakout

/*********************************SENSOR OBJECT CREATION******************/

RF24 radio(7, 8);                   // nRF24L01(+) radio attached using Getting Started board
RF24Network network(radio);          // Network uses that radio
// Initialize the Ethernet client object
WiFiEspClient client;


/***********************************************************************
************* Set the Node Address *************************************
  /***********************************************************************/
// These are the Octal addresses that will be assigned
const uint16_t node_address_set[6] = { 00, 01, 011, 0111, 02};

//tree topology
// 0 = Master
// 1 y 4 (01 - 02)   = Children of Master(00)
// 2 (011) = Children of (01)
// 3 (0111) = Children of (011)

uint8_t NODE_ADDRESS = 0;  // This is the number we have to change for every new node
//uint8_t MASTER_ADDRESS = 0;  // Use numbers 0 through to select an address from the array

/*********************************SYSTEM VARIABLES ASSIGNATION************************/
const uint16_t this_node = node_address_set[NODE_ADDRESS];        // Address of our node in Octal format
//const uint16_t other_node = node_address_set[MASTER_ADDRESS];       // Address of the other node in Octal format

const unsigned long interval = 30000; //ms  // How long will seek for package ?
//const unsigned long interval = 00; //ms  // How long will seek for package ?
const unsigned long interval_s = 5000; //ms  // How long will seek for package ?
//const unsigned long interval_s = 500; //ms  // How long will seek for package ?

unsigned long last_time_sent;

const short max_active_nodes = 5;            // Array of nodes we are aware of
uint16_t active_nodes[max_active_nodes];
short num_active_nodes = 0;
short next_ping_node_index = 0;

// Structure of our payload
struct payload_t {
  float data;
  int timestamp[6];
  unsigned int node_id;
  unsigned int sen_id;
  byte node_config[12]; 
};

char ssid[] = "Minga!";            // your network SSID (name)
char pass[] = "hj?*ara4";        // your network password
int status = WL_IDLE_STATUS;     // the Wifi radio's status
const char server[] = "192.168.1.117";

/*********************************PROTOTYPES FUNCTIONS************************/
bool send_T(uint16_t to, payload_t payload); // Prototypes for functions to send & handle messages
bool send_N(uint16_t to);
void handle_T(RF24NetworkHeader& header);
void handle_N(RF24NetworkHeader& header);
void add_node(uint16_t node);

void battery_voltage(float& voltage);  //Prototypes functions of sensor

void activate_wireless();   //other Prototypes functions
void activate_datalogger();
void init_node();
void init_radio();
void init_sensor();
void init_datalogger();
void sleep_mode (int sleep_minutes);
//void save_data(struct ts t, float payload, uint8_t node_id, uint8_t sen_id);
//bool send_data(struct ts t, float payload, uint8_t node_id, uint8_t sen_id,uint16_t to);
void send_action(uint8_t sen_id, uint16_t to, uint8_t node_config[12]);
void connect_esp8266();
void post_request_esp8266(int timestamp[6], float data, unsigned int node_id, unsigned int sen_id);
//void post_request_esp8266(String timestamp, float data, unsigned int node_id, unsigned int sen_id);
//void get_request_esp8266(byte node_config[12]);

void setup(void){

  Serial.begin(9600);  
  //Serial.println("\n\rRF24Network - Coordinator node \n\r");
  //This function set the init function, port and serial comunications of the node
  init_node();
  
}

void loop(void) {

  //Start sending package
  //EEPROM.write(5,1); //TESTING! THIS DOESNT GO HERE!
  
  if(EEPROM.read(0)){                 // After first boot

    connect_esp8266(); //connect to the wifi to send data !
        
    unsigned long now = millis(); 
    last_time_sent = now;                     
    while (status == WL_CONNECTED & now - last_time_sent <= interval){ //  We have any package available to send ?
                
      now = millis();
      
      //Serial.println(now - last_time_sent);
    
      network.update();                                      // Pump the network regularly
      
      while ( network.available() ) {                      // Is there anything ready for us?
  
      RF24NetworkHeader header;                            // If so, take a look at it
      network.peek(header);
  
      switch (header.type) {                             // Dispatch the message to the correct handler.
        case 'T': handle_T(header); break;
        case 'N': handle_N(header); break;
        default:  printf_P(PSTR("*** WARNING *** Unknown message type %c\n\r"), header.type);
          network.read(header, 0, 0);
          break; };      
      }

      delay(100);

      for (int i=1; i<=4; i++){

        uint16_t to = node_address_set[i];                                   // Who should we send to? By default, send to base
        if ( num_active_nodes ) {                           // Or if we have active nodes,
          to = active_nodes[next_ping_node_index++];      // Send to the next active node
          if ( next_ping_node_index > num_active_nodes ) { // Have we rolled over?
            next_ping_node_index = 0;                   // Next time start at the beginning
            to = 00;                                    // This time, send to node 00.
           }
        }
        //SEND START SENDING PACKAGE !
        uint8_t node_config[12] = {0,0,0,0,0,0,0,0,0,0,0,0};         
        send_action(to, 99, node_config[12]); 
      }
    }
  }

  status = WL_IDLE_STATUS; //set to default
          
  if (DS3231_triggered_a1()) {   //check trigger alarm. First boot incomming!
    // INT has been pulled low
    DS3231_clear_a1f();   // clear a1 alarm flag and let INT go into hi-z
    EEPROM.write(0,1); //set first boot flags UP! (Time to sleep) 
  }
 
  //If the alarm has triggered Pro Mini goes to sleep
  if(EEPROM.read(0)){ //check the setting flags 
    int sleep_minutes = EEPROM.read(5); //get the sleeptime from the flag
    sleep_mode(sleep_minutes);   //go to sleep for a while
  }
}        

/**--------------------------------------------------------------------------
   Sensor & modules: init, activate and deactivate functions for sensors & modules
   --------------------------------------------------------------------------*/

void init_radio () { // nrfl24l01 radio Init

  radio.begin(); //nrfl24
  radio.setPALevel(RF24_PA_MAX); // Establesco la potencia de la señal
  network.begin(/*channel*/ 90, /*node address*/ this_node ); // todos los nodos deben estar en el mismo channel
}

void init_datalogger () { // DS3231 & Catalex MicroSD Init

  //SD.begin(SDcsPin);   //SD init on Pin SDcsPin CS (10)
  DS3231_init(DS3231_INTCN);  //clock init

}

void activate_datalogger () { //switch ON datalogger shield
  pinMode(POWA_D, OUTPUT);
  digitalWrite(POWA_D, HIGH); //Datalogger lines UP!
}

void deactivate_datalogger () { //switch OFF datalogger shield
  digitalWrite(POWA_D, LOW); //Datalogger lines UP!
}

void activate_wireless () { //switch ON wireless shield
  pinMode(POWA_W, OUTPUT);
  digitalWrite(POWA_W, HIGH); //Wireless lines UP!
}

void deactivate_wireless () { //switch OFF wireless shield
  digitalWrite(POWA_W, LOW); //Wireless lines UP!
}

//This function trigger initilization of all protocols, modules ,sensor, etc. 
void init_node(){

  //protocol initialization
  SPI.begin(); //spi
  Wire.begin(); //i2c
  
  pinMode(10, OUTPUT);    // Even we dont use it, we've to setup Pin 10 as output. It's the reference for SPI channel.

  activate_wireless(); //wireless shield switch ON

  activate_datalogger(); //Datalogger shield switch ON

  init_radio();   // Init radio and assign the radio to the network  

  init_datalogger(); // set initial parameters for ds3231 & Catalex Micro SD

  Serial1.begin(9600);    // initialize serial for ESP module
}

/**--------------------------------------------------------------------------
   Sleep mode & Data manipulation: sleep mode function, datetime manipulation, send and save data
   --------------------------------------------------------------------------*/

void sleep_mode (int sleep_minutes) { // 

  //setting sleep time period
  /* the microcontroler only allow max sleeptime of 8 seconds. So, if you
  *  want to sleep for ,i.e 1 minute, we need to make a loop of sleeping time cicle.
  *  In our case we use the 4seconds sleeptime. We get better result in the division.  
  *  c_loop = (sleep_minutes*60)/4;
  */  
  if (sleep_minutes != 0){  // Only if sleep minutes >= 1 min the node goes to sleep

    //deactivate all the modules
    deactivate_datalogger();
    deactivate_wireless();
    
    int c_cloop=(sleep_minutes*60)/4;

    //sleep for c_loop times
    for (int i = 0; i < c_cloop; i++) { 
        LowPower.powerDown(SLEEP_4S, ADC_OFF, BOD_OFF); 
    }
     
    init_node();    //after sleep mode we need to reinicializar the node 
         
  } //endif
 }

//send data to destination
void send_action(uint8_t sen_id, uint16_t to, uint8_t node_config[12]){
  /*
  send_id   Action                  Description        
   99     | start sendding data   | we send the start sending package to the node 'to'
  100     | node_config           | we send the new node_config
  101     | go to sleep           | Its time to sleep!
  102     | sinc WSN              | Sinc the WSN. Set clock and sleep time alarm of the network (?) 
   */
  bool ok;  

  struct ts t;
  DS3231_get(&t); //Get time  
  
  //payload_t payload_t = {0.0, {t.year, t.mon, t.mday, t.hour, t.min, t.sec}, 0, sen_id, node_config[12]};  // sending package to coordinator
  payload_t payload_t = {0.0, {t.year, t.mon, t.mday, t.hour, t.min, t.sec}, 0, sen_id, {0,0,0,0,0,0,0,0,0,0,0,0}};  // sending package to coordinator
  ok = send_T(to, payload_t);  

}

/**--------------------------------------------------------------------------
   ESP8266: Initialization, WiFi Conection and Send/Post Request
  --------------------------------------------------------------------------*/
void connect_esp8266(){
 
  // initialize ESP module
  WiFi.init(&Serial1);  
    
  // check for the presence of the shield
  if (WiFi.status() == WL_NO_SHIELD) {
    //Serial.println("WiFi shield not present");
    // don't continue
    while (true);
  }
  // attempt to connect to WiFi network
  while (status != WL_CONNECTED) {
    //Serial.print("Attempting to connect to WPA SSID: ");
    //Serial.println(ssid);
    // Connect to WPA/WPA2 network
    status = WiFi.begin(ssid, pass);
  }
  // you're connected now, so print out the data
  //Serial.println("You're connected to the network");  
  
}
void post_request_esp8266(int timestamp[6], float data, unsigned int node_id, unsigned int sen_id){

  bool next = false; int c = 1;
  if(next==false || c < 3){      
  c += 1;
  if (client.connect(server,80)) {
    next = true;
    Serial.println("Connected to server");
    // Make HTTP POST request
    client.println("POST /api-reg/ HTTP/1.1");
    client.println("Host: 192.168.1.117");
    client.println("Authorization: Token 2b29d7ed651de2bc3d6bff6bab7efd4a3e8c542a");
    //client.println("User-Agent: Arduino/1.0\n");
    client.println("Content-Type: application/json");
    client.println("Connection: close");
    
    //String json = "{\"data\":5.5,\"fechahora\":\"2017-08-12 22:16:00\",\"nod_red_id\":2,\"sen_id\":1}";
    //String json = "{\"data\":" + String(data) + ",\"fechahora\":"+ "\""+ String("2017-08-12 22:16:00") + "\"" +",\"nod_red_id\":" + String(node_id) +",\"sen_id\":"+ String(sen_id) +"}";
    String line = " ";
    String json = "{\"data\":";
    line = String(data); json += line;
    json += ",\"fechahora\":"; json += "\"";
    line = String(timestamp[0]); json += line; json += "-";
    line = String(timestamp[1]); json += line; json += "-";
    line = String(timestamp[2]); json += line; json += " ";
    line = String(timestamp[3]); json += line; json += ":";
    line = String(timestamp[4]); json += line; json += ":";
    line = String(timestamp[5]); json += line; json += "\"";
    json += ",\"nod_red_id\":";
    line = String(node_id); json += line;
    json += ",\"sen_id\":";
    line = String(sen_id); json += line; 
    json += "}";
    line = " "; //we clean the string
    
    //add content length and data
    client.print("Content-Length: "); client.println(json.length());
    client.println();
    client.print(json);
    // if there are incoming bytes available
    // from the server, read them and print them
    while (client.available()) {
      char c = client.read();
      //Serial.write(c);
    }
    //delay(50);
    client.stop(); //disconnect TCP socket
    // if the server's disconnected, stop the client
    //if (!client.connected()) {
    //Serial.println();
    //Serial.println("Disconnecting from server...");
    //client.stop();  
    //}
  }
  }
}
void get_request_esp8266(byte node_config[12]){

  if (client.connect(server, 80)) {
    Serial.println("Connected to server");
    // Make a HTTP request
    client.println("GET /api-reg/ HTTP/1.1");
    client.println("Host: 192.168.1.117");
    client.println("Authorization: Token 2b29d7ed651de2bc3d6bff6bab7efd4a3e8c542a");
    client.println("Content-Type: application/json;");
    //client.println("User-Agent: Arduino/1.0\n");
    client.println("Connection: close");    
    client.println();  
    // if there are incoming bytes available
    // from the server, read them and print them
    while (client.available()) {
    char c = client.read();
    //Serial.write(c);
    }
    // if the server's disconnected, stop the client
    if (!client.connected()) {
    //Serial.println();
    //Serial.println("Disconnecting from server...");
    client.stop();
    }
  }
}

/**--------------------------------------------------------------------------
   Sensor measure funtions zone
  --------------------------------------------------------------------------*/

//Input battery voltage
void battery_voltage(float& voltage) {
  // the loop routine runs over and over again forever
  // read the input on analog pin 0:
  analogReference(INTERNAL);
  int sensorValue = analogRead(A0);
  voltage = 0.0;
  for (int i = 0; i < 10; i++) {
    // Convert the analog reading (which goes from 0 - 1023) to a voltage (0 - 5V):
    voltage += (sensorValue * 4.675) / 1024.0;
    delay(1);
  }
  voltage = voltage / 10.0;   // average sample
}

/**--------------------------------------------------------------------------
   Functions for send and recive menssages from the wsn
   --------------------------------------------------------------------------/

/*
   Send a 'T' message, the current time
*/

bool send_T(uint16_t to, payload_t payload)
{
  RF24NetworkHeader header(/*to node*/ to, /*type*/ 'T' /*Time*/);
  return network.write(header, &payload, sizeof(payload));
}

/**
   Send an 'N' message, the active node list
*/
bool send_N(uint16_t to)
{
  RF24NetworkHeader header(/*to node*/ to, /*type*/ 'N' /*Time*/);
  return network.write(header, active_nodes, sizeof(active_nodes));
}

/**
   Handle a 'T' message
   Add the node to the list of active nodes
*/
void handle_T(RF24NetworkHeader& header) {

  payload_t payload;
  //testing----!! : Print the inconming package
  network.read(header,&payload,sizeof(payload));
  Serial.print("Received packet from Node ");
  Serial.println(header.from_node);
  /*Serial.print(" Data: ");
  Serial.print(payload.data);
  Serial.print(" - timestamp: ");
  Serial.print(String(payload.timestamp[0]));
  Serial.print("-");
  Serial.print(String(payload.timestamp[1]));
  Serial.print("-");
  Serial.print(String(payload.timestamp[2]));
  Serial.print(" ");
  Serial.print(String(payload.timestamp[3]));
  Serial.print(":");
  Serial.print(String(payload.timestamp[4]));
  Serial.print(":");
  Serial.print(String(payload.timestamp[5]));
  Serial.print(" - node_id: ");
  Serial.print(payload.node_id);
  Serial.print(" sen_id: ");
  Serial.println(payload.sen_id);
  delay(50);*/
  //String full_timestamp = String(payload.timestamp[0]) +"-"+ String(payload.timestamp[1]) +"-"+ String(payload.timestamp[2])+" "+ String(payload.timestamp[3]) +":"+ String(payload.timestamp[4]) +":"+String(payload.timestamp[5]);
  
  //- send data to the server
  post_request_esp8266(payload.timestamp, payload.data, payload.node_id, payload.sen_id);  
  //post_request_esp8266(full_timestamp, payload.data, payload.node_id, payload.sen_id);  

  if ( header.from_node != this_node || header.from_node > 00 )                                // If this message is from ourselves or the base, don't bother adding it to the active nodes.
    add_node(header.from_node);
}

/**
   Handle an 'N' message, the active node list
*/
void handle_N(RF24NetworkHeader& header)
{
  static uint16_t incoming_nodes[max_active_nodes];

  network.read(header, &incoming_nodes, sizeof(incoming_nodes));
  //printf_P(PSTR("%lu: APP Received nodes from 0%o\n\r"), millis(), header.from_node);

  int i = 0;
  while ( i < max_active_nodes && incoming_nodes[i] > 00 )
    add_node(incoming_nodes[i++]);
}

/**
   Add a particular node to the current list of active nodes
*/
void add_node(uint16_t node) {

  short i = num_active_nodes;                                    // Do we already know about this node?
  while (i--)  {
    if ( active_nodes[i] == node )
      break;
  }

  if ( i == -1 && num_active_nodes < max_active_nodes ) {        // If not, add it to the table
    active_nodes[num_active_nodes++] = node;
    //printf_P(PSTR("%lu: APP Added 0%o to list of active nodes.\n\r"), millis(), node);
  }
}
