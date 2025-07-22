#include "BluetoothSerial.h"
#include <Adafruit_NeoPixel.h>

// Configurazione LED WS2812B
#define LED_PIN 4       // Pin per il DI del WS2812B
#define NUM_LEDS 12      // Numero di LED nella striscia
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

BluetoothSerial SerialBT;
const int ledPin = 2;   // Pin del LED
const int reel = 15;     // Pin di ingresso per lo stato dell'ombrello
const int vccReel = 19;     // Pin che funge da vcc per reel
const int gnd = 21;     // Pin che funge da gnd
const int vccLed = 5;   // Pin che funge da vcc per led
int val = -1;
unsigned long previousMillisReel = 0;  // Memorizza il tempo dell'ultimo aggiornamento
const long intervalReel = 1000;        // Intervallo di aggiornamento (1 secondo)

unsigned long previousMillisTimer = 0;        
const long intervalTimer_on = 120000;    // Tempo di lampeggio totale
const long intervalTimer_off = 1320000;   // Tempo di spegnimento

unsigned long previousMillisLed = 0; 
const long intervalLed = 20;  // Intervallo di aggiornamento della luminosità (30 ms)
bool isIncreasing = true;
int brightness = 0;

// Variabili per il colore corrente
uint8_t targetRed = 0;
uint8_t targetGreen = 0;
uint8_t targetBlue = 0;

// Variabili per la gestione dello stato
bool isBlinking = false;  // Indica se il LED sta lampeggiando
int maxBrightness = 255;  // Luminosità massima per la previsione corrente
int blinkingFrequency = 5;

// Funzione per impostare il colore di destinazione
void setTargetColor(uint8_t red, uint8_t green, uint8_t blue) {
  targetRed = red;
  targetGreen = green;
  targetBlue = blue;
}

void setup() {
  Serial.begin(115200);
  SerialBT.begin("Umbrella_Smart_1");  // Nome del dispositivo Bluetooth
  Serial.println("Bluetooth avviato!");
  strip.begin();
  strip.show();
  pinMode(ledPin, OUTPUT);  // Imposta il pin del LED come uscita
  pinMode(reel, INPUT);      // Imposta il pin del sensore come ingresso
  pinMode(vccReel, OUTPUT);
  pinMode(gnd, OUTPUT);
  pinMode(vccLed, OUTPUT);
}

void loop() {
  // Controlla se il telefono è connesso
  if (SerialBT.hasClient()) {
    // Accendi il LED se connesso
    digitalWrite(ledPin, HIGH);   // Led blu si accende quando connesso al bluetooth
    digitalWrite(gnd, LOW);       // Massa da dare al reel
    digitalWrite(vccReel, HIGH);


    // Controlla se è passato l'intervallo di 1 secondo
    unsigned long currentMillisReel = millis();   
    if (currentMillisReel - previousMillisReel >= intervalReel) {
      previousMillisReel = currentMillisReel; // Aggiorna il tempo dell'ultimo aggiornamento

      int newVal = digitalRead(reel);  // Read the last status

      if (newVal == 0 && val != newVal) {
        val = newVal;
        SerialBT.print("close");
        Serial.println(newVal);      // Print status for serial monitor
      } else if (newVal == 1 && val != newVal){
        val = newVal;
        SerialBT.print("open");
        Serial.println(newVal);      // Print status for serial monitor
      }


      // Timer per spegnimento led dopo 2 minuti di lampeggio - accensione dopo 20 minuti
      unsigned long currentMillisTimer = millis();
      if(currentMillisTimer - previousMillisTimer <= intervalTimer_on){
        digitalWrite(vccLed, HIGH);

      }else{
        digitalWrite(vccLed, LOW);
        if(currentMillisTimer - previousMillisTimer >= intervalTimer_off){
          previousMillisTimer = currentMillisTimer;
        }
      }

    }
    if (SerialBT.available()) {
      String data = SerialBT.readString();
      int delimiterIndex = data.indexOf(',');
      String weather = data.substring(0, delimiterIndex);
      String otherUmbrellas = data.substring(delimiterIndex + 1);
      weather.trim();  // It removes any spaces or tabulations

      if(otherUmbrellas == "true"){
        blinkingFrequency = 15;
      } else {
        blinkingFrequency = 5;
      }
      
      // Assegna un colore di base per ogni tipo di condizione meteo
      if (weather.equalsIgnoreCase("clear sky")) {
        setTargetColor(0, 255, 0);  // Verde acceso per cielo sereno
      } else if (weather.indexOf("clouds") != -1) {
        setTargetColor(255, 116, 0);  // Arancione per nuvole
      } else if (weather.indexOf("rain") != -1) {
        if (weather.equalsIgnoreCase("light rain") || weather.equalsIgnoreCase("light intensity shower rain") || weather.equalsIgnoreCase("light intensity drizzle rain")) {
          setTargetColor(80, 224, 255); // Blu chiaro per pioggia leggera
        } else if (weather.indexOf("moderate rain") != -1 || weather.indexOf("shower rain") != -1 || weather.indexOf("drizzle rain") != -1) {
          setTargetColor(80, 224, 255);  // Blu per pioggia moderata
        } else {
          setTargetColor(0, 0, 255);  // Blu scuro per pioggia pesante
        }
      } else if (weather.indexOf("thunderstorm") != -1) {
        setTargetColor(125, 0, 187);  // Viola per temporale
      } else if (weather.indexOf("snow") != -1) {
        setTargetColor(255, 255, 255);  // Bianco per neve
      } else if (weather.indexOf("mist") != -1 || weather.indexOf("fog") != -1 || weather.indexOf("haze") != -1) {
        setTargetColor(255, 255, 0);  // Giallo chiaro per nebbia
      } else {
        setTargetColor(255, 0, 0);  // Rosso per input non riconosciuto
      }
      isBlinking = true; // Inizia il lampeggio
      brightness = 0; // Ripristina la luminosità per iniziare il lampeggio
    }

    // Gestisce il lampeggio lento
    unsigned long currentMillisLed = millis();
    if (isBlinking) {
      // Modifica la luminosità con un effetto pulsante
      if (currentMillisLed - previousMillisLed >= intervalLed) {
        previousMillisLed = currentMillisLed;

        if (isIncreasing) {
          brightness += blinkingFrequency;
          if (brightness >= maxBrightness) { // Limita la luminosità alla massima impostata
            brightness = maxBrightness; // Fermo alla luminosità massima
            isIncreasing = false; // Cambia direzione
          }
        } else {
          brightness -= blinkingFrequency;
          if (brightness <= 0) {
            brightness = 0; // Spento
            isIncreasing = true; // Cambia direzione
          }
        }

        // Imposta il colore del LED con la luminosità attuale
        for (int i = 0; i < NUM_LEDS; i++) {
          strip.setPixelColor(i, strip.Color((targetRed * brightness) / 255, (targetGreen * brightness) / 255, (targetBlue * brightness) / 255));
        }
        strip.show();
      } 
    }   
  } else {
    // Spegni il LED se non connesso
    digitalWrite(ledPin, LOW);
    digitalWrite(vccReel, LOW);
    digitalWrite(vccLed, LOW);
    val = -1;
  }
}

