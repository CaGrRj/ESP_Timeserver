#ifndef Timeserver_h
#define Timeserver_h

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

#define LOCAL_PORT 2390 // Local port for Time requests
#define NTP_PACKET_SIZE 48 // NTP time stamp is in the first 48 bytes of the message
#define NTP_SERVER_NAME "time.nist.gov"
#define NTP_TIMEOUT 5000 // 5secs

class TimeServer
{
	
	public:
		TimeServer(int timeZone) {
			tOffset = 0;
			tZone = timeZone;
			startTime = 0;
			lastUpdate = 0;
			initiated = false;
		}
		
		bool initiate() {
			if (WiFi.status() == WL_CONNECTED) {
				udp.begin(LOCAL_PORT);
				WiFi.hostByName(ntpServerName, timeServerIP);
				initiated = true;
				return true;
			}
			return false;
		}
		
		void startTimeUpdate() {
			if (!initiated) return;
			sendNTPpacket();
			startTime = millis();
		}
		
		int tryReceiveTime() {
			if (!initiated) return 3;
			if (startTime == 0) return 3;
			
			int cb = udp.parsePacket();
			if (cb) {
				udp.read(packetBuffer, NTP_PACKET_SIZE);
				unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
				unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
				unsigned long realTime = highWord << 16 | lowWord;
				realTime -= 2208988800UL; // ntp (since 1900) => unix (since 1970)
				tOffset = realTime - millis() / 1000;
				
				startTime = 0;
				lastUpdate = millis();
				return 0;
			}
			
			if (startTime + NTP_TIMEOUT < millis()) {
				startTime = 0;
				return 2;
			}
			return 1;
		}
		
		unsigned long getLastUpdate() {
			return lastUpdate;
		}
		
		unsigned long getTimestamp() {
			return millis() / 1000 + tOffset + tZone * 3600;
		}
		
		void getDateTimestamp(unsigned long seconds, int *year, int *month, int *day) {
			unsigned long days = seconds / 86400L;
			// epoch = 4 years = 1461 days
			int epoch = days / 1461;
			days -= epoch * 1461; // days of epoch
			int yearOffset = 0;
			bool schalt = false;
			if (days >= 365) {
				days -= 365;
				yearOffset ++;
			}
			if (days >= 365) {
				schalt = true;
				days -= 365;
				yearOffset ++;
			}
			if (days >= 366) {
				schalt = false;
				days -= 366;
				yearOffset ++;
			}
			*year = epoch * 4 + 1970 + yearOffset;
			
			// days = days of year
			int monthDays[12];
			monthDays[ 0] = 31; // jan
			if (schalt) {
				monthDays[ 1] = 29; // feb
			} else {
				monthDays[ 1] = 28; // feb
			}
			monthDays[ 2] = 31; // mar
			monthDays[ 3] = 30; // apr
			monthDays[ 4] = 31; // mai
			monthDays[ 5] = 30; // jun
			monthDays[ 6] = 31; // jul
			monthDays[ 7] = 31; // aug
			monthDays[ 8] = 30; // sep
			monthDays[ 9] = 31; // okt
			monthDays[10] = 30; // nov
			monthDays[11] = 31; // dez
			
			int m = 0;
			while (m < 12) {
				if (days >= monthDays[m]) {
				days -= monthDays[m];
				m ++;
				} else {
				break;
				}
			}
			*month = m + 1;
			*day = days + 1; // 0 based (1.1.70 = 0 days)
		}
		
		void getDate(int *year, int *month, int *day) {
			getDateTimestamp(getTimestamp(), year, month, day);
		}
		
		void getTimeTimestamp(unsigned long seconds, int *hour, int *minute, int *second) {
			*hour = (seconds % 86400L) / 3600;
			*minute = (seconds % 3600) / 60;
			*second = seconds % 60;
		}
		
		void getTime(int *hour, int *minute, int *second) {
			getTimeTimestamp(getTimestamp(), hour, minute, second);
		}
		
	private:
		bool initiated;
		IPAddress timeServerIP;
		const char* ntpServerName = NTP_SERVER_NAME;
		byte packetBuffer[NTP_PACKET_SIZE];
		WiFiUDP udp;
		unsigned long tOffset;
		int tZone;
		unsigned long startTime;
		unsigned long lastUpdate;
		
		void sendNTPpacket() {
			// set all bytes in the buffer to 0
			memset(packetBuffer, 0, NTP_PACKET_SIZE);
			// Initialize values needed to form NTP request
			// (see URL above for details on the packets)
			packetBuffer[0] = 0b11100011;   // LI, Version, Mode
			packetBuffer[1] = 0;     // Stratum, or type of clock
			packetBuffer[2] = 6;     // Polling Interval
			packetBuffer[3] = 0xEC;  // Peer Clock Precision
			// 8 bytes of zero for Root Delay & Root Dispersion
			packetBuffer[12]  = 49;
			packetBuffer[13]  = 0x4E;
			packetBuffer[14]  = 49;
			packetBuffer[15]  = 52;
			
			// all NTP fields have been given values, now
			// you can send a packet requesting a timestamp:
			udp.beginPacket(timeServerIP, 123); //NTP requests are to port 123
			udp.write(packetBuffer, NTP_PACKET_SIZE);
			udp.endPacket();
		}
		
};

#endif