#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <PubSubClient.h>

#define SS_PIN_ARDUINO D1
#define SS_PIN_RFID D8
#define RST_PIN D0
#define BUZZER D2
#define BOUTON D3
byte byteReceive;
// Init array that will store new NUID
MFRC522 rfid(SS_PIN_RFID, RST_PIN);
MFRC522::MIFARE_Key key;
byte nuidPICC[4];
byte MONNUID[4];
int flagFirstCard = 0;
/////////////////////////////
byte nbPersonnes = 0;
////////////////////////////
bool etatbouton = false;
bool etatboutonavant = true;
////////////////////////////
bool etatAlarm = false;
int compteurAlarm = 0;
////////////////////////////
bool etatServo = false;
int compteurServo = 0;
Servo myservo;  // create servo object to control a servo
int pos = 0;    // variable to store the servo position
////////////////////////////
int valueLight = 0;
float temperature = 0;
int temperature2 = 0;
bool recu1 = false;
bool recu2 = false;
bool recu3 = false;
/////////////////////////////////////////////////////////

//WiFi Connection configuration
char ssid[] = "WIFINAME";     //  wifiname
char password[] = "pswd12345677899";  // passord WIFI
//mqtt server
char mqtt_server[] = "192.168.8.114";  // IP adress mqtt broker (the raspi address)
#define MQTT_USER "" //i didn't used it but it's not securty friendly
#define MQTT_PASS ""

WiFiClient espClient;
PubSubClient MQTTclient(espClient);
/////////////////////////////////////////////////////////

void setup() {
  Serial.begin(9600);
  pinMode (SS_PIN_RFID, OUTPUT);
  pinMode (SS_PIN_ARDUINO, OUTPUT);
  pinMode (BUZZER, OUTPUT);
  pinMode (BOUTON, INPUT);
  digitalWrite(SS, LOW);              // enable Slave Select
  digitalWrite(SS_PIN_RFID, HIGH);    // disable Slave Select
  digitalWrite(SS_PIN_ARDUINO, HIGH); // disable Slave Select
  myservo.attach(2);                  //D4  GPIO 2  servo object
  myservo.write(0);                   //initialized

  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV8);//divide the clock by 8
  Serial.println("Master initialized");
  rfid.PCD_Init(); // Init MFRC522
  rfid.PCD_DumpVersionToSerial(); //not necessery
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  // Conexion WIFI
  WiFi.begin(ssid, password);
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected");
  MQTTclient.setServer(mqtt_server, 1883); //port 1883 is used by deffault
}

void loop() {
  delay (200);

  static uint32_t  lastTimeMqtt = 0;
  // connect serveur MQTT
  if (!MQTTclient.connected()) {
    MQTTconnect();
  }

  //-------------------------------------------------------------------------
  //Communication from master ESP to slave Arduino---------------------------
  //-------------------------------------------------------------------------
  digitalWrite(SS_PIN_ARDUINO, LOW); //enable slave arduino with the slave select
  byteReceive = SPI.transfer(nbPersonnes); //Send the number of people inside AND receive what arduino send to esp, one time it's value light, or first digit or after coma of temperature
  //Serial.println(int(byteReceive)); //just print what i receive and it's all good
  if (int(byteReceive) > 100 && int(byteReceive) < 200 ) {//my luminosity is from 100 to 200 so everytime
    valueLight = int(byteReceive);
    valueLight = valueLight - 100; //from 0 to 100
    recu1 = true;
  }
  else if (int(byteReceive) >= 0 && int(byteReceive) <= 9) {//receive after coma temperature
    temperature2 = int(byteReceive);
    recu2 = true;
  }
  else if ( int(byteReceive) < 50) {                        //reveive before coma tempereauter
    if (int(byteReceive) <= 35 && int(byteReceive) >= 20 ) {
      temperature = int(byteReceive);
      temperature = temperature + 0.1 * temperature2;
      recu3 = true;
    }
  }

  digitalWrite(SS_PIN_ARDUINO, HIGH); //dissable slave arduino
  //-------------------------------------------------------------------------
  //STOP Communication from master ESP to slave Arduino----------------------
  //-------------------------------------------------------------------------
  if (recu1 == true && recu2 == true && recu3 == true) { //here i verify if i have 3 good value receive, because i don't know why, but in my serial print, i receive only good data, but when i compute a bit values, there are some problems. So i have somes delays and i'm a bit sad
    MQTTsend();
    recu1 = false;
    recu2 = false;
    recu3 = false;
  }

  etatbouton = digitalRead(BOUTON); //check if someone want to go out with debouncing
  //Serial.println(etatbouton);
  if (etatbouton == 0 && etatboutonavant == 1) {
    if (nbPersonnes > 0) {
      nbPersonnes--;
      etatServo = true; //open the door
    } else {
      nbPersonnes = 0;
    }
  }
  etatboutonavant = etatbouton;

  //-------------------------------------------------------------------------
  //Communication from master ESP to slave RFID -----------------------------
  //-------------------------------------------------------------------------
  digitalWrite(SS_PIN_RFID, LOW); //enable slave rfid
  funcRFID();
  digitalWrite(SS_PIN_RFID, HIGH);//dissable slave rfid
  //-------------------------------------------------------------------------
  //STOP Communication from master ESP to slave RFID-------------------------
  //-------------------------------------------------------------------------

  MQTTclient.loop();
  doAlarm();
  servoControl();

}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
void doAlarm() {
  if (etatAlarm == true) {
    compteurAlarm++; //not a delay here
    if (compteurAlarm < 3) {
      tone(BUZZER, 523) ; //DO note 523 Hz
    }
    if (compteurAlarm > 3) {
      noTone(BUZZER) ;
    }
    if (compteurAlarm > 6) {
      compteurAlarm = 0;
    }
  } else { //to stop in case we stop buzzer when he do noise
    compteurAlarm = 0;
    noTone(BUZZER) ;
  }
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
void servoControl() {
  if (etatServo == true) {
    compteurServo++; //not a delay here
    //Serial.println(compteurServo);
    if (compteurServo < 10) {
      myservo.write(180);
    }
    if (compteurServo > 10) {
      myservo.write(0);
      compteurServo = 0;
      etatServo = false;
    }
  }
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//Fonctions mqtt
void MQTTsend() {

  char bufferr[10];
  for (int i = 0; i < 10; i++) { //just in case
    bufferr[i] = '\0';
  }

  sprintf(bufferr, "%.*f", 1, temperature); //creat a string
  MQTTclient.publish("Home/Temp", bufferr );//send temperature in this format 25.2 in string to mqtt topic /Temp

  sprintf(bufferr, "%d", valueLight);
  MQTTclient.publish("Home/Lumi", bufferr);//send valueLight

  sprintf(bufferr, "%d", nbPersonnes);
  MQTTclient.publish("Home/Pers", bufferr);//send the number of people inside

}

void MQTTconnect() {
  MQTTclient.setServer("192.168.8.121", 1883);
  Serial.println("Trying to connect to MQTT broker");
  while (!MQTTclient.connected()) {
    if (MQTTclient.connect("esp")) ;
    else delay(1000);
  }
  MQTTclient.subscribe("Home/AlarmExtern"); // Subscribe to a topic, this topic is used to see if from the app, we want to stop the alarm
  MQTTclient.setCallback(MQTTcallback); // Set the callback function
}

void MQTTcallback(char* topic, byte* payload, unsigned int length) {
  payload[length] = 0;
  String messageTemp((char *)payload);
  Serial.println(messageTemp);
  if (String(topic) == "Home/AlarmExtern") {
    etatAlarm = false;
    Serial.println("stop the alarm");
  }
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
void funcRFID() {
  // if no card
  if (!rfid.PICC_IsNewCardPresent()) {
    return;
  }
  // if card
  if (!rfid.PICC_ReadCardSerial()) {
    return;
  }
  // save uid of the card (4 bytes)
  for (byte i = 0; i < 4; i++)
  {
    nuidPICC[i] = rfid.uid.uidByte[i];
  }
  if (flagFirstCard == 0) { //FIRST CARD so we save it
    for (byte i = 0; i < 4; i++)
    {
      MONNUID[i] = nuidPICC[i];
    }
    flagFirstCard = 1;
    Serial.println("frist id");
  } else {
    Serial.println("card detected");
    Serial.println("UID is :");
    int countIfGood = 0;
    for (byte i = 0; i < 4; i++)
    {
      Serial.print(nuidPICC[i], HEX);
      Serial.print(" ");
      if (nuidPICC[i] == MONNUID[i]) {
        countIfGood ++;
      }
    }
    if (countIfGood == 4) { //GOOD CARD
      if (nbPersonnes < 9) { //if here are 9 personns inside you can't go in
        nbPersonnes++;
        etatServo = true;
        compteurServo = 0;
      }
    } else {             //BAD CARD
      etatAlarm = true;
      MQTTclient.publish("Home/Alarm", "Alarm"); //bad card, there is alarm
    }
    countIfGood = 0;
    Serial.println();
    // Re-Init RFID
    rfid.PICC_HaltA(); // Halt PICC
    rfid.PCD_StopCrypto1(); // Stop encryption on PCD
  }
}
