/*
  Smart belysning
  Kort: Arduino Uno WiFi Rev.2
  - LDR (spenningsdeler) leses på A0
  - D9 (PWM) styrer lampen (LED) via MOSFET
  - Webserver over WiFi: viser LDR-verdi og har knapper for lampen

  Bibliotek: WiFiNINA (Verktøy > Administrer biblioteker > "WiFiNINA")
  Kort i Arduino IDE: Arduino Uno WiFi Rev2 (pakken "Arduino megaAVR Boards")
*/

#include <SPI.h>
#include <WiFiNINA.h>

// ---- FYLL INN DINE EGNE VERDIER ----
const char SSID[] = "DITT_WIFI_NAVN";
const char PASS[] = "DITT_WIFI_PASSORD";
// ------------------------------------

const int LDR_PIN = A0;   // lyssensor
const int LED_PIN = 9;    // PWM til MOSFET-gate (via 220 ohm)

// Hvor mørkt/lyst det er (0-1023) før auto-modus justerer lampen.
// Juster etter testing/målinger i rommet ditt.
const int MORKT = 200;    // under dette: full lysstyrke
const int LYST  = 800;    // over dette: lampen av

enum Modus { AUTO, PA, AV };
Modus modus = AUTO;

WiFiServer server(80);

int lesLdr() {
  return analogRead(LDR_PIN);   // høyere verdi = mer lys (se koblingsskjema)
}

// Auto: mørkere rom gir sterkere lampe
int autoLysstyrke(int ldr) {
  ldr = constrain(ldr, MORKT, LYST);
  return map(ldr, MORKT, LYST, 255, 0);
}

int beregnLysstyrke(int ldr) {
  switch (modus) {
    case PA:  return 255;
    case AV:  return 0;
    default:  return autoLysstyrke(ldr);
  }
}

void sendSide(WiFiClient &client, int ldr, int lysstyrke) {
  const char *modusTekst = (modus == AUTO) ? "Auto" : (modus == PA) ? "På" : "Av";
  int prosent = map(lysstyrke, 0, 255, 0, 100);

  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html; charset=utf-8");
  client.println("Connection: close");
  client.println();
  client.println("<!DOCTYPE html><html><head>");
  client.println("<meta name='viewport' content='width=device-width, initial-scale=1'>");
  client.println("<meta http-equiv='refresh' content='3'>");   // oppdaterer hvert 3. sekund
  client.println("<title>Smart belysning</title>");
  client.println("<style>body{font-family:sans-serif;text-align:center;background:#0b1424;color:#eaf0f7}");
  client.println("a{display:inline-block;margin:8px;padding:16px 28px;border-radius:12px;background:#2dd4bf;color:#0b1424;text-decoration:none;font-weight:bold}");
  client.println("</style></head><body>");
  client.println("<h1>Smart belysning</h1>");
  client.print("<p>LDR-verdi: <b>");
  client.print(ldr);
  client.println("</b> (0 = mørkt, 1023 = lyst)</p>");
  client.print("<p>Lysstyrke: <b>");
  client.print(prosent);
  client.println(" %</b></p>");
  client.print("<p>Modus: <b>");
  client.print(modusTekst);
  client.println("</b></p>");
  client.println("<a href='/auto'>Auto</a><a href='/on'>På</a><a href='/off'>Av</a>");
  client.println("</body></html>");
}

void kobleTilWifi() {
  Serial.print("Kobler til ");
  Serial.println(SSID);
  while (WiFi.begin(SSID, PASS) != WL_CONNECTED) {
    Serial.println("Prøver igjen om 5 sekunder...");
    delay(5000);
  }
  Serial.print("Tilkoblet! Åpne i nettleseren: http://");
  Serial.println(WiFi.localIP());
}

void setup() {
  pinMode(LED_PIN, OUTPUT);
  Serial.begin(9600);

  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Fant ikke WiFi-modulen.");
    while (true);
  }

  kobleTilWifi();
  server.begin();
}

void loop() {
  int ldr = lesLdr();
  int lysstyrke = beregnLysstyrke(ldr);
  analogWrite(LED_PIN, lysstyrke);   // PWM 0-255

  WiFiClient client = server.available();
  if (client) {
    String forsteLinje = "";
    bool lest = false;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        if (!lest) {
          if (c == '\n') lest = true;
          else forsteLinje += c;
        }
        if (c == '\n' && forsteLinje.length() > 0 && lest) {
          // Tom linje avslutter forespørselen; vi har det vi trenger
          if (forsteLinje.indexOf("GET /on") >= 0)        modus = PA;
          else if (forsteLinje.indexOf("GET /off") >= 0)  modus = AV;
          else if (forsteLinje.indexOf("GET /auto") >= 0) modus = AUTO;

          lysstyrke = beregnLysstyrke(ldr);
          analogWrite(LED_PIN, lysstyrke);
          sendSide(client, ldr, lysstyrke);
          break;
        }
      }
    }
    delay(1);
    client.stop();
  }

  delay(50);
}
