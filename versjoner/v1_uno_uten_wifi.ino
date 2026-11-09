/*
  Smart belysning - Arduino Uno (uten WiFi)
  - LDR (spenningsdeler) leses på A0
  - D9 (PWM) styrer lampen (LED) via MOSFET
  - Serial Monitor (9600 baud) viser verdier og tar imot kommandoer:
      a = auto, p = på, v = av
*/

const int LDR_PIN = A0;   // lyssensor
const int LED_PIN = 9;    // PWM til MOSFET-gate (via 220 ohm)

// Juster etter målinger i rommet ditt (0-1023)
const int MORKT = 200;    // under dette: full lysstyrke
const int LYST  = 800;    // over dette: lampen av

enum Modus { AUTO, PA, AV };
Modus modus = AUTO;

unsigned long sistePrint = 0;

int autoLysstyrke(int ldr) {
  ldr = constrain(ldr, MORKT, LYST);
  return map(ldr, MORKT, LYST, 255, 0);   // mørkere = sterkere lampe
}

void setup() {
  pinMode(LED_PIN, OUTPUT);
  Serial.begin(9600);
  Serial.println("Smart belysning klar.");
  Serial.println("Skriv: a = auto, p = på, v = av");
}

void loop() {
  // Les kommando fra Serial Monitor
  if (Serial.available() > 0) {
    char c = Serial.read();
    if (c == 'a') { modus = AUTO; Serial.println("Modus: Auto"); }
    if (c == 'p') { modus = PA;   Serial.println("Modus: På"); }
    if (c == 'v') { modus = AV;   Serial.println("Modus: Av"); }
  }

  int ldr = analogRead(LDR_PIN);   // høyere verdi = mer lys
  int lysstyrke;

  if (modus == PA)       lysstyrke = 255;
  else if (modus == AV)  lysstyrke = 0;
  else                   lysstyrke = autoLysstyrke(ldr);

  analogWrite(LED_PIN, lysstyrke);   // PWM 0-255

  // Skriv verdier en gang i sekundet
  if (millis() - sistePrint >= 1000) {
    sistePrint = millis();
    Serial.print("LDR: ");
    Serial.print(ldr);
    Serial.print("  Lysstyrke: ");
    Serial.print(map(lysstyrke, 0, 255, 0, 100));
    Serial.println(" %");
  }
}
