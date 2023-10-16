#define RST_PIN D3
#define SS_PIN  D4


MFRC522 mfrc522(SS_PIN, RST_PIN);
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
