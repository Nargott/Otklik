#include <RFM69.h>
#include <toneAC.h>

//define the used pins
#define SND_0 10
#define SND_1 9
#define LAMP_0 3
#define RFM_MOSI 11
#define RFM_MISO 12
#define RFM_SCK 13
#define RFM_NSS 8
#define RFM_INT 2 //INT 0
#define BTN_1 4
#define BTN_0 5

//RFM radio configuration
#define NODEID        2
#define NETWORKID     100
#define FREQUENCY     RF69_433MHZ //we work on 433MHz

#define MAX_LAST_RSSI_TIME 2500 //when to stop pulsing after last packet received

struct Sensitivity {
  unsigned char minRSSI;
  unsigned char maxRSSI;
  unsigned int  minFreq;
  unsigned int  maxFreq;
} curSens;

Sensitivity shortDistance = { //short distance params
  30,  //minRSSI
  80,  //maxRSSI
  20,  //minFreq
  1500 //maxFreq
};

Sensitivity longDistance = {  //long distance params
  70,  //minRSSI
  200, //maxRSSI
  50,  //minFreq
  2000 //maxFreq
};

//global vars
unsigned long pulseInterval = 50; //how long pulse is (ms)
unsigned int lampBright = 80; //brightness of the lamp
const unsigned int toneFreq = 4000; //frequency of buzzer tone

//time-managment
unsigned long pulseLastTime = 0; //when we pulsed
unsigned int indicatorsLastChanged = 0; //when we starts pulsing on frequency
unsigned int packetLastTime = 0;

int freq = 0;
int16_t lastRSSI, prevRSSI;


RFM69 radio(RFM_NSS, RFM_INT, true); //RADIO object

void setup() {
  //set pins
  pinMode(LAMP_0, OUTPUT);
  pinMode(BTN_0, INPUT);
  pinMode(BTN_1, INPUT);
  
  Serial.begin(9600); //init Serial for debug

  if (!radio.initialize(FREQUENCY, NODEID, NETWORKID)) { //init radio
    Serial.println("radio is not initialised");
    while(1); //stop execution
  }

  Serial.println("radio is OK!");
}

void pulse(bool isOn) {
  if (isOn) { //we want to start a pulse
    lampBright = ((digitalRead(BTN_0) == HIGH)) ? 100 : 20; //brightness regulation
    analogWrite(LAMP_0, lampBright);

    ((digitalRead(BTN_0) == HIGH)) ? toneAC(toneFreq) : noToneAC(); //make sound only if needed
    pulseLastTime = millis();
  }

  if ((!isOn) && (millis() - pulseLastTime) > pulseInterval) { //turn off light and sound
    noToneAC();
    analogWrite(LAMP_0, 0);
  }
}

void RSSIFreq() {
  if ((lastRSSI > curSens.maxRSSI) || ((millis() - packetLastTime) > MAX_LAST_RSSI_TIME)) freq = 0; //shut off on big distance
    
  curSens = ((digitalRead(BTN_1) == HIGH)) ? shortDistance : longDistance; //sensivity regulation
  
  if ((lastRSSI != prevRSSI)) { //RSSI changed
    prevRSSI = lastRSSI;
    Serial.print("lastRSSI: ");  Serial.println(lastRSSI);
    freq = map((-1*lastRSSI), curSens.minRSSI, curSens.maxRSSI, curSens.minFreq, curSens.maxFreq);
    if (freq < (int) curSens.minFreq) freq = curSens.minFreq; //fix freq if it stay too small

    Serial.print("freq: ");  Serial.println(freq);
  }

  if ((freq > 0) && ((millis() - indicatorsLastChanged) > freq)) { //time to pulse
      Serial.print(millis()); Serial.print(" pulse on freq: "); Serial.println(freq);
      pulse(true); //start pulse
      indicatorsLastChanged = millis();
  }
}

void loop() {
  pulse(false); //process unlockeble pulse (to shut off)
  RSSIFreq(); //process RSSI to requency
  
  if (radio.receiveDone()) { //packet received
    lastRSSI = radio.RSSI;
    packetLastTime = millis();
    if (radio.ACKRequested()) { //check if sender wanted an ACK
      radio.sendACK();
      Serial.println("ACK sent");
    }
  }
}
