// Quick hardware test.
//
#include <SPI.h>
#include <SdFat.h>
//
// Set DISABLE_CHIP_SELECT to disable a second SPI device.
// For example, with the Ethernet shield, set DISABLE_CHIP_SELECT
// to 10 to disable the Ethernet controller.
const int8_t DISABLE_CHIP_SELECT = -1;
//
// Test with reduced SPI speed for breadboards.
// Change spiSpeed to SPI_FULL_SPEED for better performance
// Use SPI_QUARTER_SPEED for even slower SPI bus speed
const uint8_t spiSpeed = SPI_HALF_SPEED;
//------------------------------------------------------------------------------
// File system object.
SdFat sd;

// Serial streams
ArduinoOutStream cout(Serial);

// input buffer for line
char cinBuf[40];
ArduinoInStream cin(Serial, cinBuf, sizeof(cinBuf));

// SD card chip select
int chipSelect;

void cardOrSpeed() {
  cout << F("Try another SD card or reduce the SPI bus speed.\n");
  cout << F("Edit spiSpeed in this program to change it.\n");
}

void reformatMsg() {
  cout << F("Try reformatting the card.  For best results use\n");
  cout << F("the SdFormatter program in SdFat/examples or download\n");
  cout << F("and use SDFormatter from www.sdcard.org/downloads.\n");
}

void setup() {
  Serial.begin(9600);
  while (!Serial) {}  // Wait for Leonardo.

  cout << F("\nSPI pins:\n");
  cout << F("MISO: ") << int(MISO) << endl;
  cout << F("MOSI: ") << int(MOSI) << endl;
  cout << F("SCK:  ") << int(SCK) << endl;
  cout << F("SS:   ") << int(SS) << endl;

  if (DISABLE_CHIP_SELECT < 0) {
    cout << F(
           "\nBe sure to edit DISABLE_CHIP_SELECT if you have\n"
           "a second SPI device.  For example, with the Ethernet\n"
           "shield, DISABLE_CHIP_SELECT should be set to 10\n"
           "to disable the Ethernet controller.\n");
  }
  cout << F(
         "\nSD chip select is the key hardware option.\n"
         "Common values are:\n"
         "Arduino Ethernet shield, pin 4\n"
         "Sparkfun SD shield, pin 8\n"
         "Adafruit SD shields and modules, pin 10\n");
}

bool firstTry = true;
void loop() {
  // read any existing Serial data
  while (Serial.read() >= 0) {}

  if (!firstTry) {
    cout << F("\nRestarting\n");
  }
  firstTry = false;

  cout << F("\nEnter the chip select pin number: ");
  while (!Serial.available()) {}
  delay(400);  // catch Due restart problem

  cin.readline();
  if (cin >> chipSelect) {
    cout << chipSelect << endl;
  } else {
    cout << F("\nInvalid pin number\n");
    return;
  }
  if (DISABLE_CHIP_SELECT < 0) {
    cout << F(
           "\nAssuming the SD is the only SPI device.\n"
           "Edit DISABLE_CHIP_SELECT to disable another device.\n");
  } else {
    cout << F("\nDisabling SPI device on pin ");
    cout << int(DISABLE_CHIP_SELECT) << endl;
    pinMode(DISABLE_CHIP_SELECT, OUTPUT);
    digitalWrite(DISABLE_CHIP_SELECT, HIGH);
  }
  if (!sd.begin(chipSelect, spiSpeed)) {
    if (sd.card()->errorCode()) {
      cout << F(
             "\nSD initialization failed.\n"
             "Do not reformat the card!\n"
             "Is the card correctly inserted?\n"
             "Is chipSelect set to the correct value?\n"
             "Does another SPI device need to be disabled?\n"
             "Is there a wiring/soldering problem?\n");
      cout << F("\nerrorCode: ") << hex << showbase;
      cout << int(sd.card()->errorCode());
      cout << F(", errorData: ") << int(sd.card()->errorData());
      cout << dec << noshowbase << endl;
      return;
    }
    cout << F("\nCard successfully initialized.\n");
    if (sd.vol()->fatType() == 0) {
      cout << F("Can't find a valid FAT16/FAT32 partition.\n");
      reformatMsg();
      return;
    }
    if (!sd.vwd()->isOpen()) {
      cout << F("Can't open root directory.\n");
      reformatMsg();
      return;
    }
    cout << F("Can't determine error type\n");
    return;
  }
  cout << F("\nCard successfully initialized.\n");
  cout << endl;

  uint32_t size = sd.card()->cardSize();
  if (size == 0) {
    cout << F("Can't determine the card size.\n");
    cardOrSpeed();
    return;
  }
  uint32_t sizeMB = 0.000512 * size + 0.5;
  cout << F("Card size: ") << sizeMB;
  cout << F(" MB (MB = 1,000,000 bytes)\n");
  cout << endl;
  cout << F("Volume is FAT") << int(sd.vol()->fatType());
  cout << F(", Cluster size (bytes): ") << 512L * sd.vol()->blocksPerCluster();
  cout << endl << endl;

  cout << F("Files found (date time size name):\n");
  sd.ls(LS_R | LS_DATE | LS_SIZE);

  if ((sizeMB > 1100 && sd.vol()->blocksPerCluster() < 64)
      || (sizeMB < 2200 && sd.vol()->fatType() == 32)) {
    cout << F("\nThis card should be reformatted for best performance.\n");
    cout << F("Use a cluster size of 32 KB for cards larger than 1 GB.\n");
    cout << F("Only cards larger than 2 GB should be formatted FAT32.\n");
    reformatMsg();
    return;
  }
  // read any existing Serial data
  while (Serial.read() >= 0) {}
  cout << F("\nSuccess!  Type any character to restart.\n");
  while (Serial.read() < 0) {}
}