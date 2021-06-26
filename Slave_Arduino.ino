#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>

#define  analogInPhoto A2
#define analogOutLEDLumi 9

uint8_t addr_slave  = 0x18 ; //found on the internet
LiquidCrystal_I2C lcd(0x27, 20, 4); // set the LCD address to 0x27 for a 16 chars and 2 line display

int valuePhotocell = 0;
byte nbPersonnes; //number people inside
int valueLight;
int temperature = 0; //temperature before digit
int temperature2 = 0; //temperature after digit
int chooseSend = 0;

volatile bool receivedone;  // use reception complete flag /

void setup()
{
  Serial.begin(9600);
  Wire.begin();
  Wire.beginTransmission(addr_slave);
  lcd.init();               // initialize the lcd
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Temperature:  .");
  lcd.setCursor(0, 1);
  lcd.print("Personnes:" );
  pinMode(analogInPhoto, INPUT);
  //SPI setup ------------------------------------------------
  SPCR |= _BV(SPE);         // Enable SPI /
  pinMode(MISO, OUTPUT);    // Make MISO pin as OUTPUT /
  receivedone = false;
  SPI.attachInterrupt();    // Attach SPI interrupt /
}


void loop()
{

  Display_Temp_LCD();
  delay(200); //same delay in slave (arduino) and in master (ESP) via spi

  // read the analog in value:
  valuePhotocell = analogRead(analogInPhoto);
  valueLight = 100 + valuePhotocell * 0.1; //map(valuePhotocell, 0, 600, 0, 255);

  // change the analog out value:
  if (nbPersonnes == 0) { //turn off the light and lcd if there is no one
    lcd.noBacklight();
    analogWrite(analogOutLEDLumi, LOW);
  } else {
    lcd.backlight();
    if (200 - ((valueLight - 100) * 2.5) > 0) { //just modify data from photo resisitor and analog write it in the LED
      analogWrite(analogOutLEDLumi, 200 - ((valueLight - 100) * 2.5));
    } else {
      analogWrite(analogOutLEDLumi, 10);
    }
  }

  if (receivedone) { //if we passed in the interrupt of SPI, we receive the number of people inside the house
    Serial.println("receive:");
    Serial.println(nbPersonnes);
    Serial.println("sent:");
    Serial.println(valueLight);
    Serial.print(temperature); Serial.print(","); Serial.println(temperature2);
    receivedone = false;
  }
}

// SPI interrupt routine
ISR(SPI_STC_vect) //ecrire dans le registre spr pour une reponse caractere par caractere
{
  if (receivedone == false) {
    uint8_t oldsrg = SREG;
    cli();
    nbPersonnes = SPDR;  //here we receive the byte of master, every time master send the same byte -> number of people inside but here, slave want to send back le valueLight, and temperature before and after digit
    //so we send 3 bytes (if we send a float, 1float = 4bytes, that's why i did "my own float send" in Display_Temp_LCD()
    if (chooseSend == 0) {
      SPDR = byte(valueLight); //to send from slave to master in spi, we write in the SPDR register, bytes by bytes
      chooseSend = 1;
    } else if (chooseSend == 1) {
      SPDR = byte(temperature); //to send from slave to master in spi, we write in the SPDR register, bytes by bytes
      chooseSend = 2;
    } else {
      SPDR = byte(temperature2); //to send from slave to master in spi, we write in the SPDR register, bytes by bytes
      chooseSend = 0;
    }
    receivedone = true;
    SREG = oldsrg;
  }
}

void Display_Temp_LCD() {
  byte temperatureMSB = 0;
  byte temperatureLSB = 0;
  Wire.beginTransmission(addr_slave);
  Wire.write(0x05);
  Wire.endTransmission(false);  // Condition RESTART
  Wire.requestFrom(0x18, 2); // two bytes needed
  Wire.endTransmission();
  if (2 <= Wire.available()) {
    temperatureMSB = Wire.read();  // Octet de poids fort
    temperatureLSB = Wire.read();  // Octet de poids faible
    temperature = (16 * (temperatureMSB & 0xF)) + 0.0625 * (temperatureLSB & 0xF0) ;//here my own compute
    temperature2 =  0.625 * (temperatureLSB & 0x0F); //here my own compute
  }
  lcd.setCursor(12, 0);
  lcd.print(temperature);
  lcd.setCursor(15, 0);
  lcd.print(temperature2);
  lcd.setCursor(12, 1);
  lcd.print(nbPersonnes);
}
