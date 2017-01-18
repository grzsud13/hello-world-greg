

/* Data powstania v02 2.11.2016
 * v02 wyremowano nie uzywane wpisy
 * v02 zmodyfikowano obsluge PINu kontrolera pieca 2.11.2016
 * v03 dodano opoznienie przelaczenia pieca jesli przyjdzie z OpenHab 11.11.2016
 * v03 wgrano 11.11.2016
 * v04 dodano debuging do obslugi sterownika
 * v04 wydluzono opoznienie z 70 do 500 ms przy detekcji wlaczenia sterownika
 * v04 wgrano 8.01.2017
 */

//#include <SPI.h>
//#include <Ethernet.h>
//#include <UIPEthernet.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
//#include <Metro.h>

//Metro CheckInterval = Metro(60000);

const String SWversion(" WERSJA v04 09.01.2017");


//const long interval = 1000;
unsigned long previousMillis = 0;
unsigned long ViessmanStateChangedByHabStarted = 0;
unsigned long DelaySwitchViessmanState = 20000;
unsigned long TemperatureReadingInterval = 60000;


unsigned long milis = 0;
unsigned long milis2 = 0;


int l1=0;
int l2=0;
int l3=0;
String l1str="0";
String l2str="0";
String l3str="0";


#define AP_SSID     "GREG"
#define AP_PASSWORD "Kamiko2!1a"

//#define AP_SSID     "GREG24G"
//#define AP_PASSWORD "kamiko2!1a"


const int ONE_WIRE_BUS = 4;
const int VIESSMAN_RELAY_PIN = 12;
const int CONTROLLER_PIN = 14;
const int LAN_STATUS_LED = 16;


boolean ControllerPinStatus = HIGH; //informuje o stanie przekaznika na sciennym kontrolerze pieca
boolean ViessmanStateChangeNotExecuted = false ;

int currentViessmanStatefromHAB = LOW; //Aktualny stan Pieca z openHAB

String TempStr;
String IntToStr;
String ViessmanStatefromHAB, LastViessmanStateFromHab;


const String VIESSMAN_ON_COMMAND("viessman#1");
const String VIESSMAN_OFF_COMMAND("viessman#0");

const int AFTER_CHANGE_DELAY = 400;

// NOWE:

int buttonState;             // the current reading from the input pin // aktualny stan Pieca ze Sterownika
int lastButtonState = LOW;   // the previous reading from the input pin

// the following variables are long's because the time, measured in miliseconds,
// will quickly become a bigger number than can be stored in an int.
long lastDebounceTime = 0;  // the last time the output pin was toggled
long debounceDelay = 50;    // the debounce time; increase if the output flickers





OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);



//byte mac[]    = {  0xDE, 0xED, 0xBA, 0xFE, 0xFE, 0xED };
//IPAddress arduino_ip(192, 168, 1, 124);
IPAddress mqtt_server(192, 168, 1, 125);



void callback(char* topic, byte* payload, unsigned int length) 
 {
  payload[length] = '\0';
  String strPayload = String((char*)payload);
  Serial.print("IN: ");
  Serial.println(strPayload);
  //executeRemoteCommand(strPayload);  
  ViessmanStatefromHAB = strPayload;  //MQTT command przekazywana za pomoca tej zmiennej dalej do procedury obslugi
  Serial.println("ViessmanStateFromHab callbackIN: " + ViessmanStatefromHAB);
 }




//EthernetClient ethClient;
WiFiClient espClient;
PubSubClient mqtt_client(mqtt_server, 1883, callback, espClient);


// *************************** PROCEDURES DECLARATIONS ********************************************

void sendMqttState(int ItemToChange, String state) 
{
  //Serial.print("STATE: ");
  //Serial.print(state);
  boolean publishState;
/*
konieczna weryfikacja czy mamy aktywne połączenie
jeśli nie, trzeba je odnowić
*/
  boolean connected = mqtt_client.connected();
  if (!connected)
  {
     initializeMqtt();
  }
  char ssid[state.length()+1];        
  state.toCharArray(ssid, state.length()+1);
  
  if (ItemToChange == 1)
  {
   publishState = mqtt_client.publish("/ViessmanHAB", ssid);
   delay(500);
   publishState = mqtt_client.publish("/ViessmanARD", ssid); 
  }
  if (ItemToChange == 3)
  {
   publishState = mqtt_client.publish("/TempHAB/lazienka", ssid);
  } 
 
  if (ItemToChange == 7)
  {
   publishState = mqtt_client.publish("/Sterownik/stopien1/", ssid);
  }
  if (ItemToChange == 8)
  {
   publishState = mqtt_client.publish("/Sterownik/stopien2/", ssid);
  }
  if (ItemToChange == 9)
  {
   publishState = mqtt_client.publish("/Sterownik/stopien3/", ssid);
  }

  
 // Serial.print(" publishedEND: ");
 // Serial.println(publishState);
}




void setItemState(int number,int state) 
{

    if (number == 1)
    {  
     digitalWrite(VIESSMAN_RELAY_PIN, state);
   //  digitalWrite(HEATING_LED_PIN,!(state)); //zmiana zeby dioda gasla gdy wylacznony - to jeszcze nie wgrane
      //RadiatorState = state;
      if (state == HIGH) {
        sendMqttState(1,VIESSMAN_ON_COMMAND);
      } 
      else 
          {
           sendMqttState(1,VIESSMAN_OFF_COMMAND);
          }
    }
   
}





void executeRemoteCommand(String command) 
{
  //Serial.println(command);
  if (command == VIESSMAN_ON_COMMAND) {
    setItemState(1,HIGH);
  }
  if (command == VIESSMAN_OFF_COMMAND) {
    setItemState(1,LOW);
  }
  
}










void initializeMqtt()
{
  if (mqtt_client.connect("ESP8266Client", "greg", "kaczorek")) {
    mqtt_client.subscribe("/ViessmanARD");
    delay(200);
    digitalWrite(LAN_STATUS_LED,LOW);
   Serial.println("MQTT connect OK");
  } else {
    digitalWrite(LAN_STATUS_LED,HIGH);
   Serial.println("MQTT connect failed");
  }
}




/*
void checkMqtt()
{
  mqtt_client.loop();
}
*/


// **************************************  SETUP BEGIN *******************************************


void setup() {
  // put your setup code here, to run once:
  pinMode(LAN_STATUS_LED, OUTPUT); //LED for LAN status
  pinMode(VIESSMAN_RELAY_PIN, OUTPUT);
  pinMode(CONTROLLER_PIN, INPUT);

  //Initial state of LAN LED
  digitalWrite(LAN_STATUS_LED,HIGH); //LAN status indication LED
  digitalWrite(VIESSMAN_RELAY_PIN, LOW);
  
  
  Serial.begin(115200);
//  Ethernet.begin(mac,arduino_ip);
  /*if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // no point in carrying on, so do nothing forevermore:
    // try to congifure using IP address instead of DHCP:
    Ethernet.begin(mac, ip);
  }
  */
  Serial.println(" Sterownik Pieca - " + SWversion);
  Serial.println("");
  Serial.println("");

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(AP_SSID);
 

  WiFi.begin(AP_SSID, AP_PASSWORD);
  
  
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LAN_STATUS_LED,HIGH);
    delay(250);
    digitalWrite(LAN_STATUS_LED,LOW);
    Serial.print(".");
    delay(250);
  }
  

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  //digitalWrite(LAN_STATUS_LED,LOW);

  
  DS18B20.begin();

 /* 
 Serial.print("Locating devices...");
  Serial.print("Found ");
  Serial.print(DS18B20.getDeviceCount(), DEC);
  Serial.println(" devices.");
  
  Serial.print("My IP address: ");
  for (byte thisByte = 0; thisByte < 4; thisByte++) {
  // print the value of each byte of the IP address:
  Serial.print(Ethernet.localIP()[thisByte], DEC);
  Serial.print(".");}
  */

ViessmanStatefromHAB = "viessman#0";
LastViessmanStateFromHab = "viessman#0";
ViessmanStateChangedByHabStarted = millis();
    
  initializeMqtt();
  //initializeRadiator();
//  initializeRadiatiorState();


//mqtt_client.publish("/RestartHAB", "1");
Serial.println("ViessmanStatefromHAB: " + ViessmanStatefromHAB);
Serial.println("LastViessmanStateFromHab: " + LastViessmanStateFromHab);
sendMqttState(7,l1str);
sendMqttState(8,l1str);
sendMqttState(9,l1str);
Serial.println("END of SETUP");
Serial.println("");

}  //END of SETUP



//******************************** LOOP BEGIN **************************************


void loop() {
  // put your main code here, to run repeatedly:


if (millis() - previousMillis >= TemperatureReadingInterval)
 {
  previousMillis = millis();
  
float Temp;
  do {
    DS18B20.requestTemperatures(); 
    Temp = DS18B20.getTempCByIndex(0);
   Serial.println(" Sterownik Pieca - " + SWversion); 
   Serial.print(" Aktualna Temperatura: ");
   Serial.println(Temp);
   Serial.println("  C ");
     } while (Temp == 85.0 || Temp == (-127.0));

     TempStr = String(Temp);
   Serial.print("TempStr: ");
   Serial.println(TempStr);
 
  sendMqttState(3,TempStr);
 // if (temp != oldTemp)
 // {
    //sendTeperature(Temp);
  //}
  //sendMqttState(2,TempStr);
 }



//Obsluga Sterownika Sciennego 


 // read the state of the switch into a local variable:
  int reading = digitalRead(CONTROLLER_PIN);

  // check to see if you just pressed the button 
  // (i.e. the input went from LOW to HIGH),  and you've waited 
  // long enough since the last press to ignore any noise:  

  // If the switch changed, due to noise or pressing:
  if (reading != lastButtonState) {
    // reset the debouncing timer
    lastDebounceTime = millis();
  } 
  
  if ((millis() - lastDebounceTime) > debounceDelay) {
    // whatever the reading is at, it's been there for longer
    // than the debounce delay, so take it as the actual current state:
    buttonState = reading;  // ten status na interesuje !!!!
    
    digitalWrite(VIESSMAN_RELAY_PIN,buttonState);
    if (buttonState == HIGH)
      {
        //sendMqttState(1,VIESSMAN_OFF_COMMAND);
        Serial.println("VIESMAN OFF");
      } 
      else 
          {
           //sendMqttState(1,VIESSMAN_ON_COMMAND);
           Serial.println("VIESMAN ON");
          }
  }
  
  // set the LED using the state of the button:
  //digitalWrite(ledPin, buttonState);

  // save the reading.  Next time through the loop,
  // it'll be the lastButtonState:
  lastButtonState = reading;



//********** OBSLUGA REQUESTU Z OPENHAB z OPOZNIENIEM POCZATEK****************

//Sprawdza czy zmienil sie status Viessmana w OpenHAB, jesli tak to wlacza licznik
if (ViessmanStatefromHAB != LastViessmanStateFromHab) 
    {
     Serial.println("Wykryto zmiane z "+ LastViessmanStateFromHab + " na " + ViessmanStatefromHAB);
     Serial.println("");
     
     ViessmanStateChangedByHabStarted = millis();
     LastViessmanStateFromHab = ViessmanStatefromHAB;
     ViessmanStateChangeNotExecuted = !ViessmanStateChangeNotExecuted;
       
    }
//Jesli po okreslonym czasie status sie nie zmienil to zmienia status na zadany
//milis2 = ViessmanStateChangedByHabStarted - millis();
//Serial.println("Przed petla ViessmanStateChangedByHabStarted - millis " + String(milis2));

if (( millis()- ViessmanStateChangedByHabStarted ) >= DelaySwitchViessmanState) 
  {
 if (ViessmanStateChangeNotExecuted)
    {
     milis2 = millis()- ViessmanStateChangedByHabStarted ;
     Serial.println("Od zmiany uplynelo: " + String(milis2));
     //milis = millis();
     //Serial.println("millis : " + String(milis)); 
     Serial.println("Minal timer zmieniam status na : " + ViessmanStatefromHAB); 
     Serial.println("");
    // ViessmanStateChangedByHabStarted = millis();  //WYDAJE SIE NIEPOTRZEBNE
     executeRemoteCommand (ViessmanStatefromHAB); // ***************************

     
     if (ViessmanStatefromHAB == VIESSMAN_ON_COMMAND)
     {
      currentViessmanStatefromHAB=LOW;
     }
     if (ViessmanStatefromHAB == VIESSMAN_OFF_COMMAND)
     {
      currentViessmanStatefromHAB=HIGH;
     }
     
     ViessmanStateChangeNotExecuted = false;
    }
  }
//********** OBSLUGA REQUESTU Z OPENHAB z OPOZNIENIEM  KONIEC****************


if (currentViessmanStatefromHAB != buttonState)  //jesli zmienil sie status Pieca na openHAB lub Sterowniku to wykonaj:
{
  
}


mqtt_client.loop();


} // END of LOOP
