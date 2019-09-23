#include <ESP8266WiFi.h>
#include "TimeServer.h"

TimeServer timeServer(2); // TimeZone UTC = 0, MEZ = 1, MESZ = 2, ...

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFi.begin("SSID", "Password");
  Serial.print("Connecting to network");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());

  if (!timeServer.initiate()) {
    Serial.println("Error. Can not initiate timeServer");
    while(true){delay(1000);}
  }

  timeServer.startTimeUpdate(); // send UDP packet with time request
  
  Serial.print("Trying to receive time");
}

void loop() {
  if (timeServer.getLastUpdate() == 0) { // last update in millis since boot - 0 when never set
    int err = timeServer.tryReceiveTime(); // check if UDP response available / timeout reached (5secs)
	// returns:
	// 0 = Packet received, time set
	// 1 = No packet available, waiting...
	// 2 = Timeout reached -> send new request (or exit)
	// 3 = No time update requested -> send request first (never executed startTimeUpdate / timeout reached and not re-executed startTimeUpdate)
    if (err == 2) {
      Serial.print("\nTIMEOUT. Sending new request");
      timeServer.startTimeUpdate();
    } else {
      switch (err) {
        case 0: Serial.println("\nTIME SET."); break;
        case 1: Serial.print("."); break;
        case 3: Serial.print("\nError. No request sent!"); while(true){delay(1000);} break;
        default: Serial.print("\nUnknown error."); while(true){delay(1000);} break; // should never happen...
      }
    }
    delay(100);
  } else {
    int day, month, year, hour, minute, second;
    timeServer.getDate(&year, &month, &day);
    timeServer.getTime(&hour, &minute, &second);
    Serial.printf("%02d.%02d.%04d %02d:%02d:%02d\n", day, month, year, hour, minute, second);
    delay(1000);
  }
}
