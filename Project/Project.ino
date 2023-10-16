/* wiring the MFRC522 to ESP8266 (ESP-12)
RST     = GPIO5    =  D3
SDA(SS) = GPIO4    =  D4
MOSI    = GPIO13   =  D7
MISO    = GPIO12   =  D6
SCK     = GPIO14   =  D5
GND     = GND
3.3V    = 3.3V
*/
/* wiring the SD-Card to ESP8266 (ESP-12)
SDA(SS) = GPI15    =  D8
MOSI    = GPIO13   =  D7
MISO    = GPIO12   =  D6
SCK     = GPIO14   =  D5
GND     = GND
3.3V    = 3.3V
*/

#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
#include <SPI.h>
#include <SD.h>
#include <MFRC522.h>
#include <Wire.h>
#include <RTClib.h>
#include <EEPROM.h>
//#include <Config.h>
#include <SoftwareSerial.h>
#include <PN532_SWHSU.h>
#include <PN532.h>

SoftwareSerial SWSerial( 3, 2 ); // RX, TX
 
PN532_SWHSU pn532swhsu( SWSerial );
PN532 nfc( pn532swhsu );
String tagId = "None", dispTag = "None";
byte nuidPICC[4];


FirebaseData firebaseData;

File LogFile;
File UIDFile;

RTC_DS1307 rtc;
const int chipSelect = D8;
String State = "IN";
String Date="";
String Time="";

// Log file name
#define LOG_FILE_NAME "Loggs.txt"

#define ssid  "Vodafone_VDSL"
#define password  "2yNG5uhSDCFcPfeS"
#define firebaseHost  "reid-4aba0-default-rtdb.firebaseio.com"
#define firebaseAuth  "Tv4Griyb753ndlwi54sFFRPBnLGU3CJBk4TgfOCE"
#define FIREBASE_PATH "/RFID_IDs/"
String rfid = "";
String GATR_NUMBER= "Gate_1";

int BUTTON_UPDATE_IDs=D10;
int Relay = D9;
static int ref=0;
String IDs[]={""};


void Init_WiFi(){
	// Init WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi!");

  Firebase.begin(firebaseHost, firebaseAuth);
}
//========================================================
 void Init_RFID(){
  nfc.begin();
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata)
  {
    Serial.print("Didn't Find PN53x Module");
    while (1); // Halt
  }
  // Got valid data, print it out!
  Serial.print("Found chip PN5");
  Serial.println((versiondata >> 24) & 0xFF, HEX);
  Serial.print("Firmware ver. ");
  Serial.print((versiondata >> 16) & 0xFF, DEC);
  Serial.print('.'); 
  Serial.println((versiondata >> 8) & 0xFF, DEC);
  // Configure board to read RFID tags
  nfc.SAMConfig();
  //Serial.println("Waiting for an ISO14443A Card ...");
 }

//===================================================================
void Load_ID_From_SD(){

    UIDFile = SD.open("IDs.txt",FILE_READ);
  if (!UIDFile) {
    Serial.println("Error opening IDs file to Find");
    return ;
  }
  Serial.println("DONE Opening IDs file to Find");

   // Read 
  while(UIDFile.available()){
      String line;
 for(int i=0; i <= EEPROM.read(116); i++){
      line = UIDFile.readStringUntil('\n');
      Serial.println(line);
      IDs[i]=line;
     
    }
    UIDFile.close();

  }

}


//===================================================================


void Init_SD(){
	 //Setup for the SD card
  Serial.print("Initializing SD card...");
  if(!SD.begin(chipSelect)) {
    Serial.println("initialization failed!");
    return;
  }
  Serial.println("initialization done.");

      UIDFile = SD.open("IDs.txt",FILE_WRITE);
  if (!UIDFile) {
    Serial.println("Error opening IDs file to Find");
    return ;
  }
  Serial.println("DONE Opening IDs file to Find");
  UIDFile.close();
}
//======================================================================================

void Init_RTC(){
	   // Init RTC
  Wire.begin(D2, D1); // SDA, SCL pins for RTC module
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC module");
    while (1);
  }
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
}
//======================================================================================
void setup() {
  pinMode(BUTTON_UPDATE_IDs,INPUT);
  pinMode(Relay,OUTPUT);

 
  Serial.begin(115200);
  EEPROM.begin(1024);
  SPI.begin();
  ref=EEPROM.read(116);
  // EEPROM.write(108,0);
  // EEPROM.commit();
  
  Init_WiFi();
  Init_RFID();
  Init_SD();
  Init_RTC();
  Load_ID_From_SD();
  
}

//======================================================================================

void loop() {
  int Sens_Val = digitalRead(BUTTON_UPDATE_IDs);
  if(Sens_Val == HIGH){
    Update_IDs();
    }else
    {
      //readNFC();
      if(readNFC()){
      if(Find_ID(dispTag))
      {
        digitalWrite(Relay,HIGH);
        // delay(20000);
        digitalWrite(Relay,LOW);
        gettime();
        Send_Loggs(dispTag);
        Store_Loggs_SD(dispTag);
        //Upload_Loggs();
      }else
      {
          Update_IDs();
      }
      }
          delay(1000);
     }
}
//============================================================================================================

boolean readNFC()
{
  boolean success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;                       // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength);
  if (success)
  {
    Serial.print("UID Length: ");
    Serial.print(uidLength, DEC);
    Serial.println(" bytes");
    Serial.print("UID Value: ");
    for (uint8_t i = 0; i < uidLength; i++)
    {
      nuidPICC[i] = uid[i];
      Serial.print(" "); 
      Serial.print(uid[i], HEX);
    }
    Serial.println();
    
    tagId = tagToString(nuidPICC);
    dispTag = tagId;
    Serial.print(F("tagId is : "));
    Serial.println(dispTag);
    Serial.println("");
    delay(1000);  // 1 second halt
    return true;
  }
  else
  {
    // PN532 probably timed out waiting for a card
    Serial.println("Timed out! Waiting for a card...");
    tagId=" ";
    return false;
  }
}

//========================================================

String tagToString(byte id[4])
{
  String tag = "";
  for (byte i = 0; i < 4; i++)
  {
    if (i < 3){
      tag += String(id[i]) + " ";
      }else{
        tag += String(id[i]);
      } 
  }
  return tag;
}

//====================================================================================
void Update_IDs() {
 int num = get_Count_IDs();
 if(( num>0 )  &&   (ref != num)){
  
   // Read all IDs from Firebase and store them on SD card
 if (Firebase.getString(firebaseData, FIREBASE_PATH)== false) {
    
       Serial.println("DB NOT CONNECTED ");
   //delay(5);
 }else {
   Serial.println("DB CONNECTED SUCCSS");
   SD.remove("IDs.txt");
    UIDFile = SD.open("IDs.txt", FILE_WRITE);
   
    
     if (UIDFile) {
      Serial.println("file opend");
      UIDFile.println(num);
       IDs[0]=num;
       for (int i = 1; i <= num; i++) {
         String id = getLine(i);
         UIDFile.println(id);
         IDs[i]=id;
         //delay(100);
       }
       ref=EEPROM.read(116);
       UIDFile.close();
     
       Serial.println("IDs Updated & stored on SD card!");
     } else {
       Serial.println("Error opening Updating file!");
     }
   }
 
 }
 //delay(1000);
}
//======================================================================================
bool Find_ID(const String& logData){
int lenth = IDs[0].toInt();
for(int i=1; i < lenth; i++){
if(logData.toInt() == IDs[i].toInt()){
  return true;
}

}
return false;
}


//     UIDFile = SD.open("IDs.txt",FILE_READ);
//   if (!UIDFile) {
//     Serial.println("Error opening IDs file to Find");
//     return false;
//   }
//   Serial.println("DONE Opening IDs file to Find");
  
//   // Read 
//   while(UIDFile.available()){
//     Serial.println(logData);
//     String line;
//  for(int i=0; i <= EEPROM.read(116); i++){
//       line = UIDFile.readStringUntil('\n');
//       Serial.println(line);
//       if(logData.toInt() == line.toInt()){
//       Serial.println("ID is valid");
//       UIDFile.close();
//       return true;
//       }
//     }
//       Serial.println("ID NOT valid");
//        UIDFile.close();
//   }
//        return false;


//====================================================================================
void Upload_Loggs(){

   // Read log file
  LogFile = SD.open("Loggs.txt",FILE_READ);
  if (!LogFile) {
    Serial.println("Error opening loggs file");
    return;
  }
     Serial.println("DONE opening loggs file");

  // Read and send log data to Firebase
  while (LogFile.available()) {
    String line = LogFile.readStringUntil('\n');
    Serial.println(line);
    sendLogData(line);
  }
Serial.println("Uploading loggs Done");
  // Close log file
  LogFile.close();
}

//====================================================================================
void sendLogData(const String& logData) {
  // Parse log data
  String count = getValue(logData, ':', 0);
  String id    = getValue(logData, ',', 1);
  String date  = getValue(logData, ',', 2);
  String time  = getValue(logData, ',', 3);
  String state = getValue(logData, ',', 4);

  Firebase.setString(firebaseData, "/Loggs/" + GATR_NUMBER + "/" + count + "/ID", id);
  Firebase.setString(firebaseData, "/Loggs/" + GATR_NUMBER + "/" + count + "/date", date);
  Firebase.setString(firebaseData, "/Loggs/" + GATR_NUMBER + "/" + count + "/time", time);
  Firebase.setString(firebaseData, "/Loggs/" + GATR_NUMBER + "/" + count + "/state",state);

  // Send data to Firebase
   if (firebaseData.dataAvailable()) {
    Serial.println("Data sent to Firebase");
  } else {
    Serial.println("Error sending data to Firebase");
  }
}
//====================================================================================
String getValue(const String& data, char separator, int index) {
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }

  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}
//====================================================================================

String getLine(int i)
{
    String line;
    String count = "";
    count += i;
    if (Firebase.getString(firebaseData, FIREBASE_PATH + count))
    {
        
        Serial.println("GETTING LINE --> SUCCSSES");
    // Get the string
    line= firebaseData.stringData();

    // Print the string to the serial port
    Serial.println(line);
  // Wait for 1 second
  //delay(100);
  
    }else{
       Serial.println("GETTING LINE --> Failed");
    }
   
    return line;

}
//======================================================================================
int get_Count_IDs()
{
   
   if(Firebase.getInt(firebaseData, "/ID_Number")){
            Serial.println("GETTING COUNT IDs --> successfully");

    // Get the integer
    int integerData = firebaseData.intData();

    // Print the integer to the serial port
    Serial.println(integerData);

  EEPROM.write(116,integerData);
  EEPROM.commit();
    //delay(100);

      return integerData;

   }else{
    Serial.println("GETTING COUNT IDs --> Failed");
      //delay(100);

    return 0;
   }

}
//======================================================================================
void gettime(){
    DateTime now = rtc.now();
  Date = now.timestamp(DateTime::TIMESTAMP_DATE);
  Time = now.timestamp(DateTime::TIMESTAMP_TIME);
  }
//======================================================================================
void Send_Loggs(String ID){
  String count = "";
int Line =EEPROM.read(100);

count="";
 count += Line;
    
      // Store data in Firebase
  Firebase.setString(firebaseData, "/Loggs/" + GATR_NUMBER + "/" + count + "/ID", ID);
  Firebase.setString(firebaseData, "/Loggs/" + GATR_NUMBER + "/" + count + "/date", Date);
  Firebase.setString(firebaseData, "/Loggs/" + GATR_NUMBER + "/" + count + "/time", Time);
  Firebase.setString(firebaseData, "/Loggs/" + GATR_NUMBER + "/" + count + "/state",State);


  if (firebaseData.dataAvailable()) {
    Line++;
  EEPROM.write(100,Line);
  EEPROM.commit();
    Serial.println("Data sent to Firebase successfully!");
  } else {
    Serial.println("Failed to send data to Firebase");
  }
   //delay(1000); // Delay between each iteration
  }
//======================================================================================

void Store_Loggs_SD(String rfidID){

int Line =EEPROM.read(108);

  LogFile = SD.open("Loggs.txt", FILE_WRITE);
  
  if (LogFile) {
    Line++;
  EEPROM.write(108,Line);
  EEPROM.commit();
    LogFile.print(Line);
    LogFile.print(":");
    LogFile.print(rfidID);
    LogFile.print(",");
    LogFile.print(Date);
    LogFile.print(",");
    LogFile.print(Time);
    LogFile.print(",");
    LogFile.println(State);
    LogFile.close();
    Serial.println("Data stored in the file.");
  } else {
    Serial.println("Failed to open Loggs for writing.");
  }

  }
//======================================================================================
