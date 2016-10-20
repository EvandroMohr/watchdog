/*
 	Arduino script to manage the basics of Watchdog server.
	For instructions, go to https://github.com/EvandroMohr/watchdog
	Created by Evandro Mohr, Feb 2015.
	Released into the public domain.
*/

#include "SD.h"
#include "ICMPPing.h"

byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED}; 
byte ip[] = {10, 0, 0, 9};
SOCKET pingSocket = 0;
char buffer [128];
ICMPPing ping(pingSocket, (uint16_t)random(0, 255));

File file;
EthernetServer server(9966);
#define REQ_BUF_SZ   100
#define IP_BUF_SZ   18

char HTTP_req[REQ_BUF_SZ] = {0}; 
char req_index = 0;
long previousMillis = 0;


char ip_buf_1[IP_BUF_SZ] = {0};
char ip_buf_2[IP_BUF_SZ] = {0};
char ip_buf_3[IP_BUF_SZ] = {0};
char ip_buf_4[IP_BUF_SZ] = {0};

// user settings
long interval = 90000; // 30 seconds
long deads_before_restart = 3;
long offTimeBeforeRestart = 5000;


int port[4];
int ips[4][4];
int stats[4];
int dead[4];

int reles[4]={6,7,8,9};

void setup() {

  for(int n=0; n<4; n++){
    pinMode(reles[n], OUTPUT);
    digitalWrite(reles[n], HIGH);
  }

  Ethernet.begin(mac, ip);
  Serial.begin(9600);
  if (!SD.begin(4)) {
    return;
  }

  //writeARP();
  readARP();

  
  
}
void loop() {
  pinga();
  Serial.begin(9600);
  EthernetClient client = server.available();  // try to get client

  if (client) {  // got client?
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        if (req_index < (REQ_BUF_SZ - 1)) {
          HTTP_req[req_index] = c;          // save HTTP request character
          req_index++;
        }
       
        else if (c == '\n' && currentLineIsBlank) {
          client.println(F("HTTP/1.1 200 OK\nAccess-Control-Allow-Origin: *\nContent-Type: application/json\nConnnection: close\n"));
          if (startsWith(HTTP_req, "GET / ")
              || startsWith(HTTP_req, "GET /index.html")) {
            // 26.516
            // 1610
            client.println(F("[{\"version\":\"1.0.1\"},{\"published\":\"2016-02-03\"},{\"status\":\"operational\"}]"));
          }
          else if (startsWith(HTTP_req, "GET /arp")){
            if (getARP(ip_buf_1, ip_buf_2,ip_buf_3, ip_buf_4, IP_BUF_SZ)) {
              writeARP();
              readARP();
            }
            client.println(F("{\"status\":\"updated\"}"));
          }
          else if (startsWith(HTTP_req, "GET /reset/")){
          	int p;
          	int n = sscanf(HTTP_req, "GET /reset/%d", &p);
          	if(n){ resetDevice(p); }
            client.println(F("{\"status\":\"reseted\"}"));
          }
          else if (startsWith(HTTP_req, "GET /reload")){
            readARP();
            client.println(F("{\"status\":\"reloaded\"}"));
          }
          
          else if (startsWith(HTTP_req, "GET /status")) {
            client.print(F("["));
            for (int x = 0; x < 4; x++) {
              client.print(F("{\"ip\": \""));
              client.print(ips[x][0]);
              client.print(F("."));
              client.print(ips[x][1]);
              client.print(F("."));
              client.print(ips[x][2]);
              client.print(F("."));
              client.print(ips[x][3]);
              client.print(F("\", \"port\": "));
              client.print(port[x]);
              client.print(F(", \"status\": "));
              client.print(stats[x]);
              client.print(F(", \"deads\": "));
              client.print(dead[x]);
              client.print(F("}"));
              if (x != 3)  client.print(",");
            }
            client.print(F("]"));
          } else if (startsWith(HTTP_req, "GET /favicon.ico")) {
            client.println(F("HTTP/1.1 200 OK\nConnnection: close"));
          }

          req_index = 0;
          StrClear(HTTP_req, REQ_BUF_SZ);
          break;
        }
        if (c == '\n') {
          currentLineIsBlank = true;
        }
        else if (c != '\r') {
          currentLineIsBlank = false;
        }
      }
    }
    delay(1);
    client.stop();
  }
}

void writeARP() {
  file = SD.open("ARP.txt", FILE_WRITE);
  file.seek(0);
  if (file) {
    file.println(ip_buf_1);
    file.println(ip_buf_2);
    file.println(ip_buf_3);
    file.println(ip_buf_4);
    file.close();
  }
}

void readARP(){
  file = SD.open("ARP.txt");
  int n = 0;
  char str[20];
  char *ptr;

  for (int i = 0; i < 4; i++) {
    n = readField(&file, str, sizeof(str), "\n");
    char* command = strtok(str, ";");

    while (command != 0)
    {
      char* separator = strchr(command, '.');
      if (separator != NULL)
      {
        char* octet = strtok(command, ".");
        for (int j = 0; j < 4; j++) {
          ips[i][j] = atoi(octet);
          octet = strtok(0, ".");
        }
      } else {
        port[i] = atoi(command);
      }
      command = strtok(0, ";");
    }
  }
  file.close();
}


size_t readField(File* file, char* str, size_t size, char* delim) {
  char ch;
  size_t n = 0;
  while ((n + 1) < size && file->read(&ch, 1) == 1) {
    // Delete CR.
    if (ch == '\r') {
      continue;
    }
    str[n++] = ch;
    if (strchr(delim, ch)) {
      break;
    }
  }
  str[n] = '\0';
  return n;
}

boolean getARP(char *line1, char *line2, char *line3, char *line4, int len)
{
  boolean got_text = false;    // text received flag
  char *str_begin;             // pointer to start of text
  char *str_end;               // pointer to end of text
  int str_len = 0;
  int txt_index = 0;
  char *current_line;

  current_line = line1;

  // get pointer to the beginning of the text
  str_begin = strstr(HTTP_req, "&P1=");

  for (int j = 0; j < 4; j++) { // do for 4 lines of text
    if (str_begin != NULL) {
      str_begin = strstr(str_begin, "=");  // skip to the =
      str_begin += 1;                      // skip over the =
      str_end = strstr(str_begin, "&");

      if (str_end != NULL) {
        str_end[0] = 0;  // terminate the string
        str_len = strlen(str_begin);

        // copy the string to the buffer
        for (int i = 0; i < str_len; i++) {
          if (str_begin[i] == 0) {
            // end of string
            break;
          }
          else {
            current_line[txt_index++] = str_begin[i];
            if (txt_index >= (len - 1)) {
              // keep the output string within bounds
              break;
            }
          }
        } // end for i loop
        // terminate the string
        current_line[txt_index] = 0;
        if (j == 0) {
          // got first line of text, now get second line
          str_begin = strstr(&str_end[1], "P2=");
          current_line = line2;
          txt_index = 0;
        }
        else if (j == 1) {
          // got first line of text, now get second line
          str_begin = strstr(&str_end[1], "P3=");
          current_line = line3;
          txt_index = 0;
        }
        else if (j == 2) {
          // got first line of text, now get second line
          str_begin = strstr(&str_end[1], "P4=");
          current_line = line4;
          txt_index = 0;
        }
        got_text = true;
      }
    }
  } // end for j loop

  return got_text;
}

bool startsWith(const char *str, const char *pre)
{
    size_t lenpre = strlen(pre),
           lenstr = strlen(str);
    return lenstr < lenpre ? false : strncmp(pre, str, lenpre) == 0;
}

void pinga() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis > interval) {
    previousMillis = currentMillis;

    for (int x = 0; x < 4; x++) {
      byte pingAddr[] = {ips[x][0], ips[x][1], ips[x][2], ips[x][3]};
      ICMPEchoReply echoReply = ping(pingAddr, 1);
      if (echoReply.status != 0) {
        dead[x] += 1;
      } else {
        dead[x] = 0;
      }
      stats[x] = echoReply.status;
    }
  }
  checkAndResetDevices();
}

void checkAndResetDevices() {
  for (int x = 0; x < 4; x++) {
    if (dead[x] > deads_before_restart){
      dead[x] = 0;
      resetDevice(x);  
    }
  }
}

void resetDevice(int port){
  dead[port] = 0;
  digitalWrite(reles[port],LOW);
  delay(offTimeBeforeRestart);
  digitalWrite(reles[port], HIGH);
  
}

void StrClear(char *str, char length)
{
  for (int i = 0; i < length; i++) {
    str[i] = 0;
  }
}
