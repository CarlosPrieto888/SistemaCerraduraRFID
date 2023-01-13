//Librerias utilizadas
#include <SPI.h>                // Serial Peripheral Interface. Es una libreria regula diversos protocolos de comunicacion de varios dispositivos integrados
#include <MFRC522.h>            // Libreria del RFID
#include <LiquidCrystal_I2C.h>  // Libreria para el LCD
#include <Servo.h>              // Libreria para el uso del Servomotor
#include <SD.h>                 // Libreria para tarjetas SD
#include <TimeLib.h>            // Libreria de cronometraje de tiempo

#define SSpin 4  // Slave Select en pin digital 10

File archivo;  // Instancia archivo del tipo File

//Librerias de la conexión de Internet
//#include <Ethernet.h>
//#include "ThingSpeak.h"  // always include thingspeak header file after other header files and custom macros

/*Secrets.h*/
// Use this file to store all of the private credentials
// and connection details

// Enter a MAC address for your controller below.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
//70-4D-7B-3F-6B-C3
/*#define SECRET_MAC \
  { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }

#define SECRET_CH_ID 1958760                    // replace 0000000 with your channel number
#define SECRET_WRITE_APIKEY "OH1MFS713B4GOMW5"  // replace XYZ with your channel write API Key    */

//Variables de la conexion a Internet
//byte mac[] = SECRET_MAC;
// Set the static IP address to use if the DHCP fails to assign
//193.146.97.141    10.130.52.207
/*IPAddress ip(10, 130, 52, 207);
IPAddress myDns(193, 146, 97, 141);*/
/*IPAddress ip(192, 168, 1, 52);
IPAddress myDns(192, 168, 1, 1);*/

/*EthernetClient client;

unsigned long myChannelNumber = SECRET_CH_ID;
const char* myWriteAPIKey = SECRET_WRITE_APIKEY;*/

// Pin del Buzzer
const byte pinBuzzer = 6;

#define NR_OF_READERS 2

//Pines de los RFIDS
#define RST_PIN 9        // RST -> Reset
#define SS_PIN_rfid1 10  // SS -> Slave Select es una variable sirve para el intercambio de datos
#define SS_PIN_rfid2 7

byte ssPins[] = { SS_PIN_rfid1, SS_PIN_rfid2 }; // Vector donde almacenamos los pines de cada RFID
byte LecturaUID[4];  // Array para almacenar el UID leido
byte uidLlaveroRegistrado[4] = { 0x6A, 0x5B, 0x35, 0x16 };  // Diferentes uids de los llaveros y tarjetas

// Instancia MFRC522 :
MFRC522 mfrc522[NR_OF_READERS];

// Pines para el LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Variable que comprueba el estado de la puerta
// 0 = cerrada
// 1 = abierta
int estadoPuerta = 0;

// Servomotor
Servo servomotor;  // Instancia del servomotor

void setup() {
  Serial.begin(9600);  // Initialize serial communications with the PC
  while (!Serial)
    ;           // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
  SPI.begin();  // Init SPI bus*/

  // Iniciamos los diferentes RFIDS
  for (uint8_t reader = 0; reader < NR_OF_READERS; reader++) {
    mfrc522[reader].PCD_Init(ssPins[reader], RST_PIN);  // Init MFRC522
    mfrc522[reader].PCD_DumpVersionToSerial();          // Show details of PCD - MFRC522 Card Reader details
  }
  // COSA IMP
  // PCD -> Proximity Coupling Device, en otras palabras, es el lector del RFID 
  // PICC -> Proximity Integrated Circuit Chip, son las tarjetas blancas

  // Iniciamos el LCD
  lcd.init();
  lcd.backlight();

  // Iniciamos el servomotor
  servomotor.attach(5);

  /*  
  //Parte del código de la conexión a Internet
  Ethernet.init(10);   // Most Arduino Ethernet hardware
  while (!Serial) {
    ;  // wait for serial port to connect. Needed for Leonardo native USB port only
  }

  // start the Ethernet connection:
  Serial.println("Initialize Ethernet with DHCP:");
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // Check for Ethernet hardware present
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
      Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
      while (true) {
        delay(1);  // do nothing, no point running without Ethernet hardware
      }
    }
    if (Ethernet.linkStatus() == LinkOFF) {
      Serial.println("Ethernet cable is not connected.");
    }
    // try to congifure using IP address instead of DHCP:
    Ethernet.begin(mac, ip, myDns);
  } else {
    Serial.print("  DHCP assigned IP ");
    Serial.println(Ethernet.localIP());
  }
  // give the Ethernet shield a second to initialize:
  delay(1000);

  ThingSpeak.begin(client);  // Initialize ThingSpeak
  */

  Serial.println("Inicializando tarjeta ...");    // Texto en ventana de monitor
  if (!SD.begin(SSpin)) {                         // Inicializacion de tarjeta SD
    Serial.println("fallo en inicializacion !");  // Si falla se muestra texto correspondiente y sale del setup() para finalizar el programa
  }
  // Controlar el tiempo
  setTime(20, 0, 0, 19, 3, 2022);  //hr, mm, s, d, m, y   
}

void loop() {
  // Apertura para lectura/escritura de archivo prueba.txt
  archivo = SD.open("prueba.txt", FILE_WRITE);  
  // archivo.println("Entrando al sistema " + reloj());

  if(archivo){
    archivo.println("prueba sistema " + reloj());   
    archivo.close(); 
  }

  // Primero limpiamos lo que haya en el LCD
  lcd.clear();
  for (uint8_t reader = 0; reader < NR_OF_READERS; reader++) {
    // El LCD imprime "Pase la tarjeta", esperando a que se pase un uid por encima
    lcd.setCursor(0, 0);
    lcd.print("Pase la tarjeta");

    // Aqui buscaremos si hay tarjetas comunicando desde el propio lector -> PICC_IsNewCardPresent()
    if (mfrc522[reader].PICC_IsNewCardPresent() && mfrc522[reader].PICC_ReadCardSerial()) {  // Si la tarjeta contiene algún tipo de dato -> PICC_ReadCardSerial()
      // Dump debug info about the card; PICC_HaltA() is automatically called
      //mfrc522.PICC_DumpToSerial(&(mfrc522.uid));
      mostrarUid(reader);

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Leyendo tarjeta");

      if (comparaUID(LecturaUID, uidLlaveroRegistrado)) {  // En caso que este registrado
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Acceso");
        lcd.setCursor(0, 1);
        lcd.print("Permitido");
        delay(2000);
        if (estadoPuerta == 0) {  // Si la puerta esta cerrada se abre
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Abriendo puerta");
          // El servomotor abre la puerta
          servomotor.write(0);
          estadoPuerta = 1;
          if(archivo){
            archivo.println("Puerta Abierta " + reloj());            
          }
        } else {        // Si la puerta está abierta se cierra
          lcd.clear();  // Borramos el texto y no mostramos nada en la pantalla
          // El servomotor cierra la puerta
          servomotor.write(90);
          estadoPuerta = 0;
          if(archivo){
            archivo.println("Puerta Cerrada " + reloj());            
          }
        }

      } else {  // En caso que no este registrado
        buzzer();
        // Escribimos en el LCD
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Acceso");
        lcd.setCursor(0, 1);
        lcd.print("Denegado");
        // Falta otro log aqui
        delay(2000);
      }
      // Paramos la comunicacion con las tarjetas
      mfrc522[reader].PICC_HaltA();  // Detiene la comunicacion con la tarjeta 
    }
  }

  /*//Para la vuelta de la conexion al servidor de ThingSpeak
  delay(15000);*/
}

// En esta funcion del codigo nos mostrara por la terminal de Arduno el UID de la tarjeta
void mostrarUid(uint8_t reader) {
  Serial.print("UID");  // Muestra texto UID:

  /*if(archivo){
      if (reader == 0) {
        Serial.print(" detectado por el RFID 1 -> ");
        archivo.println("Detectado por el RFID 1 " + reloj());
      } else if (reader == 1) {
        archivo.println("Detectado por el RFID 2 " + reloj());
        Serial.print(" detectado por el RFID 2 -> ");
      } else {
        archivo.println("Error en los RFIDs" + reloj());
        Serial.print("Se ha producido un ERROR en la lectura");
      }
  }*/

  for (byte i = 0; i < mfrc522[reader].uid.size; i++) {  // Recorre de a un byte por vez el UID
    if (mfrc522[reader].uid.uidByte[i] < 0x10) {         // Si el byte leido es menor a 0x10
      Serial.print(" 0");                                // Imprime espacio en blanco y numero cero
    } else {                                             // Sino
      Serial.print(" ");                                 // Imprime un espacio en blanco
    }
    Serial.print(mfrc522[reader].uid.uidByte[i], HEX);   // Imprime el byte del UID leido en hexadecimal
    LecturaUID[i] = mfrc522[reader].uid.uidByte[i];      // Almacena en array el byte del UID leido
  }
  Serial.print("\n");

  /*if(archivo){
    archivo.println("Detectado por " + LecturaUID[]);    
  }*/

  //Parte del codigo que sube a la base de datos ThingSpeak
  //long long_numero = buffer[3]*256^3 + buffer[2]*256^2 + buffer[1]*256^1 + buffer[0]*256^0;
  /*int num = LecturaUID[0]*256^3 + LecturaUID[1]*256^2 + LecturaUID[2]*256^1 + LecturaUID[3]*256^0;
  Serial.print(num);    
  ThingSpeak.writeField(myChannelNumber, 1, num, myWriteAPIKey); */
}

//Esta funcion compara el uid extraido con el registrado dentro del sistema
//Devuelve: TRUE si esta dentro del sistema
//          FALSE en caso contrario
boolean comparaUID(byte lectura[], byte usuario[]) {
  for (uint8_t reader = 0; reader < NR_OF_READERS; reader++) {  // Accedemos a todas las tarjetas
    for (byte i = 0; i < mfrc522[reader].uid.size; i++) {       // Buscamos el uid de la tarjeta
      if (lectura[i] != usuario[i]) {
        return (false);
      }
    }
  }
  return (true);  // Si los 4 bytes coinciden retorna verdadero
}

// Función que hace sonar el buzzer durante 2sg
void buzzer() {
  analogWrite(pinBuzzer, OUTPUT);
  delay(2000);
  analogWrite(pinBuzzer, INPUT);
}

String dato(int digit) {
  String dt = String("0") + digit;
  return dt.substring(dt.length() - 2);
}

String reloj() {
  String tiempo = String(hour()) + ":" + dato(minute()) + ":" + dato(second());
  String fecha = String(year()) + "-" + dato(month()) + "-" + dato(day());
  return tiempo + fecha;
}