#include <vector>

#include <SPI.h>
#include <MFRC522.h>
#include <NfcAdapter.h>

#include "GWebAES.h"

                                                                                                                                                                                                                                                                                                                                                                                                                                       #include <ESPAsyncWebServer.h>
#include <ESPAsyncTCP.h>

#include <ESP8266WiFi.h>
#include <FS.h>

#include "dataPins.h"

#define SSID_LENGTH 32
#define PASS_LENGTH 64

#define SS_PIN D8
#define RST_PIN D0

// BUZZER.VARIABLES
String bvPath = "/buzzer.variables";
uint8_t lastIPDigit = 2;
unsigned long authUID = 0;
String wifiSSID = "";
String wifiPass = "";
// BUZZER.VARIABLES

// AP.DATA
String adPath = "/ap.data";
String apSSID = "Wifi";
String apPass = "";
// AP.DATA

MFRC522 rfid(SS_PIN, RST_PIN);
uint16_t rfidBuzzerNoteFailed = 1047;
uint16_t rfidBuzzerNoteSuccess = 2093;
bool authenticated = false;

String nfcData = "";
const int minimumPayloadData = 7;
uint8_t nahl = 16;
String nfcAuthHeader = "V0JBQzg5NzhENDU2QzBORg==";

NfcAdapter nfc = NfcAdapter(&rfid);

String toWrite = "";
String successStr = "þ";
String failedStr = "ý";
bool startReading = false;
bool startWriting = false;

File midiFile;

int buzzer = D1;

bool headerReceived = false;
int headerOffset = 12;
unsigned long fileDataHeader = 0;
unsigned long buzzerDataHeader = 0x0AD6D8FF;

int curNote = 0;

unsigned long songLen = 0;
uint16_t bpm = 0;
uint16_t ppq = 0;
uint16_t notePitch = 0;
uint16_t noteDuration = 0;
unsigned long noteTime = 0;

int fileLen = 0;
bool updateMidi = false;

unsigned long ms = 0;
unsigned long startMs = 0;
unsigned long endMs = 0;
unsigned long msSubtract = 0;
unsigned long msAdder = 0;
bool paused = false;
bool prevPaused = false;
bool songLooped = false;

float durationMultiplier = 1;
float bpmDivider = 1.0625;

String dataList = "";

String addReqId = "";
bool startOpened = false;
int addDataPos = 0;
size_t addDataLength = 0;
File bdWrite;

bool startUpdating = false;
bool serverStarted = false;

Dir root;

String durMulPath = "/duration.multiplier";

// Live Data Final
// Live Data Position
// Live Data Maximum

uint16_t ldf = 0;
uint8_t ldp = 0;
uint8_t ldm = 2;

IPAddress staticIp(10, 0, 1, 10);
IPAddress gateway(10, 0, 1, 1);
IPAddress subnet(255, 255, 255, 0);

WiFiClient client;
// static std::vector<AsyncClient*> clients;
WiFiServer liveServer(1280);
AsyncWebServer server(80);

String spaceify(String input, char toAdd, uint16_t maxLen) {
  String output = input;
  uint16_t diffLen = maxLen - (uint16_t)input.length();
  if (!(diffLen > 0)) diffLen = 0;
  if (diffLen > maxLen) diffLen = maxLen;
  for (uint16_t i = 0; i < diffLen; i++) output += toAdd;
  return output;
}

// BUZZER.VARIABLES
void loadBV() {
  File bv = SPIFFS.open(bvPath, "r");

  if (bv) {
    size_t bvLength = bv.size();
    
    if (bvLength == 101) {
      lastIPDigit = bv.read();
      authUID = (bv.read() << 24) + (bv.read() << 16) + (bv.read() << 8) + bv.read();

      wifiSSID = "";
      for (uint16_t a = 0; a < SSID_LENGTH; a++) {
        uint8_t bvByte = bv.read();
        if (bvByte != 0) wifiSSID += char(bvByte);
      }

      wifiPass = "";
      for (uint16_t a = 0; a < PASS_LENGTH; a++) {
        uint8_t bvByte = bv.read();
        if (bvByte != 0) wifiPass += char(bvByte);
      }
    }
    
    bv.close();
  }
}

void saveBV() {
  if (SPIFFS.exists(bvPath)) SPIFFS.remove(bvPath);
  File bv = SPIFFS.open(bvPath, "w");

  if (bv) {
    bv.write(lastIPDigit);
    bv.write((uint8_t)(authUID >> 24));
    bv.write((uint8_t)((authUID >> 16) & 0xFF));
    bv.write((uint8_t)((authUID >> 8) & 0xFF));
    bv.write((uint8_t)(authUID & 0xFF));

    /*
    uint16_t ssidLen = (uint16_t)wifiSSID.length();n
    uint16_t passLen = (uint16_t)wifiPass.length();
    
    for (uint16_t a = 0; a < SSID_LENGTH; a++) {
      if (a < ssidLen)
        bv.write((uint8_t)wifiSSID[a]);
      else
        bv.write(0);
    }

    for (uint16_t a = 0; a < PASS_LENGTH; a++) {
      if (a < passLen)
        bv.write((uint8_t)wifiPass[a]);
      else
        bv.write(0);
    }
    */

    bv.print(spaceify(wifiSSID, '\0', SSID_LENGTH));
    bv.print(spaceify(wifiPass, '\0', PASS_LENGTH));
    
    bv.close();
  }
}
// BUZZER.VARIABLES

// AP.DATA
void loadAD() {
  File ad = SPIFFS.open(adPath, "r");

  if (ad) {
    size_t adLength = ad.size();
    
    if (adLength == 96) {
      apSSID = "";
      for (uint8_t a = 0; a < SSID_LENGTH; a++) {
        uint8_t adByte = ad.read();
        if (adByte != 0) apSSID += char(adByte);
      }

      apPass = "";
      for (uint8_t a = 0; a < PASS_LENGTH; a++) {
        uint8_t adByte = ad.read();
        if (adByte != 0) apPass += char(adByte);
      }
    }
    
    ad.close();
  }
}
// AP.DATA

void getHeader() {
  midiFile.seek(0);
  
  uint8_t* dataArr = (uint8_t*)malloc(4);
  midiFile.read(dataArr, 4);

  fileDataHeader = (dataArr[0] << 24) + (dataArr[1] << 16) + (dataArr[2] << 8) + dataArr[3];

  free(dataArr);
}

void getLength() {
  midiFile.seek(4);

  uint8_t* dataArr = (uint8_t*)malloc(4);
  midiFile.read(dataArr, 4);

  songLen = (dataArr[0] << 24) + (dataArr[1] << 16) + (dataArr[2] << 8) + dataArr[3];

  free(dataArr);
}

void getBPM() {
  midiFile.seek(8);

  uint8_t* dataArr = (uint8_t*)malloc(2);
  midiFile.read(dataArr, 2);

  bpm = (dataArr[0] << 8) + dataArr[1];

  free(dataArr);
}

void getPPQ() {
  midiFile.seek(10);

  uint8_t* dataArr = (uint8_t*)malloc(2);
  midiFile.read(dataArr, 2);

  ppq = (dataArr[0] << 8) + dataArr[1];

  free(dataArr);
}

uint16_t millisecondsFromTicks16(uint16_t ticks) {
  return ticks * (60000 / ((uint16_t)(bpm / bpmDivider) * ppq));
}

unsigned long millisecondsFromTicks32(uint32_t ticks) {
  return ticks * (60000 / ((uint16_t)(bpm / bpmDivider) * ppq));
}

uint16_t getNotePitch(int noteNum) {
  int fileOffset = headerOffset + (noteNum * 2);
  midiFile.seek(fileOffset);

  uint8_t* dataArr = (uint8_t*)malloc(2);
  midiFile.read(dataArr, 2);

  uint16_t finalBytes = (dataArr[0] << 8) + dataArr[1];

  free(dataArr);

  return finalBytes;
}

uint16_t getNoteDuration(int noteNum) {
  int fileOffset = headerOffset + (songLen * 2) + (noteNum * 2);
  midiFile.seek(fileOffset);

  uint8_t* dataArr = (uint8_t*)malloc(2);
  midiFile.read(dataArr, 2);

  uint16_t finalBytes = (dataArr[0] << 8) + dataArr[1];

  free(dataArr);

  return finalBytes;
}

uint32_t getNoteTime(int noteNum) {
  int fileOffset = headerOffset + (songLen * 4) + (noteNum * 4);
  midiFile.seek(fileOffset);

  uint8_t* dataArr = (uint8_t*)malloc(4);
  midiFile.read(dataArr, 4);

  uint32_t finalBytes = (dataArr[0] << 24) + (dataArr[1] << 16) + (dataArr[2] << 8) + dataArr[3];

  free(dataArr);

  return finalBytes;
}

void getNote() {
  notePitch = getNotePitch(curNote);
  noteDuration = getNoteDuration(curNote);
  noteTime = getNoteTime(curNote);
}

void loadSong(String midiPath) {
  if (SPIFFS.exists(midiPath)) {
    if (midiFile) {
      midiFile.close();
    }
  
    headerReceived = false;
    updateMidi = false;
  
    fileLen = 0;
    curNote = 0;
    fileDataHeader = 0;
  
    midiFile = SPIFFS.open(midiPath, "r");
    if (!midiFile) {
        Serial.println("Failed to open file");
    } else {
        Serial.printf("File Size: %u bytes\n", midiFile.size());
    }

    uint8_t header[4];
    midiFile.read(header, 4);
    Serial.printf("Header Bytes: %02X %02X %02X %02X\n", header[0], header[1], header[2], header[3]);
    
    uint8_t firstNote[8];
    midiFile.read(firstNote, 8);
    Serial.printf("First Note Raw: %02X %02X %02X %02X  %02X %02X %02X %02X\n",
        firstNote[0], firstNote[1], firstNote[2], firstNote[3],
        firstNote[4], firstNote[5], firstNote[6], firstNote[7]);
      
    if (!midiFile) {
      Serial.println("MIDI file cannot be found!");
    } else {
      fileLen = midiFile.size();
    }
  }
}

void updateDataList() {
  bool succeeded = root.next();
  
  if (succeeded) {
    String toAppend = root.fileName() + "\n";
    if (!(dataList.indexOf(toAppend) >= 0)) {
      dataList += toAppend;
    }
  } else {
    startUpdating = true;
  }
}

String processFilesForSongNames() {
  String finalStr = "";
  String curStr = "";
  int dataListLength = dataList.length();
  for (int i = 0; i < dataListLength; i++) {
    char listChar = dataList[i];
    if (listChar == '\n') {
      if (curStr.endsWith(".name")) {
        String songTitle = "";
        if (SPIFFS.exists(curStr)) {
          File nameFile = SPIFFS.open(curStr, "r");
          if (nameFile) {
            while (nameFile.available()) {
              songTitle += char(nameFile.read());
            }
  
            finalStr += char(songTitle.length());
            finalStr += songTitle + curStr.substring(0, curStr.length() - 5);
            if (i + 1 < dataListLength) finalStr += "\n";
          }
        }
      }
      curStr = "";
    } else {
      curStr += listChar;
    }
  }
  return finalStr;
}

void sendErr(AsyncWebServerRequest* request) {
  request->send(400);
}

void resetFileAddValues() {
  startOpened = false;
  addDataPos = 0;
  addDataLength = 0;
  if (bdWrite) bdWrite.close();
  addReqId = "";
}

/*
int getPercent() {
  int percent = ((ms - startMs) * 100) / endMs;
  if (percent > 100) percent = 100;
  if (!(percent >= 0)) percent = 0;
  return percent;
}
*/

void restartSong() {
  curNote = 0;
  updateMidi = true;
  startMs = ms;
}

void playSong() {
  long diffMs = ms - startMs;
  if (!updateMidi || (diffMs > endMs)) restartSong();
  paused = false;
}

void pauseSong() {
  paused = true;
}

void saveDurMul() {
  if (SPIFFS.exists(durMulPath)) SPIFFS.remove(durMulPath);
  File durMulFile = SPIFFS.open(durMulPath, "w");
  
  if (durMulFile) {
    durMulFile.write(char((uint8_t)(durationMultiplier * 100)));
    durMulFile.close();
  }
}

void loadDurMul() {
  if (SPIFFS.exists(durMulPath)) {
    File durMulFile = SPIFFS.open(durMulPath, "r");
    
    if (durMulFile) {
      if (durMulFile.available() > 0) {
        float durMulFileData = durMulFile.read();
        if (durMulFileData > 100.0) durMulFileData = 100.0;
        durationMultiplier = durMulFileData / 100.0;
      }
      
      durMulFile.close();
    }
  }
}

void setupServer() {
  server.on("/add", HTTP_POST, [](AsyncWebServerRequest* request){}, NULL, [](AsyncWebServerRequest* request, uint8_t* data, size_t dataLength, size_t dataIndex, size_t totalData) {
    String addRequestId = "";
    int cLength = 0;
    int dataPos = -1;

    bool s1 = false;
    bool s2 = false;
    bool s3 = false;
    
    int headers = request->headers();
    
    for (int i = 0; i < headers; i++) {
      AsyncWebHeader* h = request->getHeader(i); 
      
      String hName = h->name().c_str();
      String hValue = h->value().c_str();
      
      if (hName == "Request-Id") {
        addRequestId = hValue;
        if (!(addReqId.length() > 0)) addReqId = addRequestId;
        s1 = true;
      } else if (hName == "Buzzer-Data-Length") {
        cLength = hValue.toInt();
        s2 = true;
      } else if (hName == "Buzzer-Data-Position") {
        dataPos = hValue.toInt();
        s3 = true;
      }

      Serial.println(hName + " " + hValue);
    }

    if (s1 && s2 && s3) {
      if (addRequestId == addReqId && dataPos == addDataPos) {
        if (!startOpened) {
          startOpened = true;
          String reqData = "";
        
          for (int i = 0; i < dataLength; i++) {
            reqData += char(data[i]);
          }
  
          if (dataLength >= 2) {
            size_t l1 = reqData[0];
            size_t l2 = reqData[1];
        
            int offset1 = 2;
            int offset2 = offset1 + l1 + l2;
            
            if (dataLength >= offset2) {
              reqData = reqData.substring(offset1, dataLength - offset1);
              String songName = reqData.substring(0, l1);
              reqData = reqData.substring(l1, reqData.length() - l1);
              String songFile = reqData.substring(0, l2);
  
              reqData = "";
              
              String nameFilePath = "/" + songFile + ".name";
              String binFilePath = "/" + songFile + ".bin";
  
              if (bdWrite) bdWrite.close();
              if (SPIFFS.exists(nameFilePath)) SPIFFS.remove(nameFilePath);
              File nameFile = SPIFFS.open(nameFilePath, "w");
              if (nameFile) {
                nameFile.print(songName);
                nameFile.close();
                if (SPIFFS.exists(binFilePath)) SPIFFS.remove(binFilePath); 
                bdWrite = SPIFFS.open(binFilePath, "w");
                for (int i = offset2; i < dataLength; i++) {
                  bdWrite.write(char(data[i]));
                }
                bdWrite.flush();
                request->send(200);
              } else {
                sendErr(request);
              }
            } else {
              sendErr(request);
            }
          } else {
            sendErr(request);
          }
        } else {
          for (int i = 0; i < dataLength; i++) {
            bdWrite.write(char(data[i]));
          }
          bdWrite.flush();
          request->send(200);
        }
  
        addDataLength += dataLength;
        addDataPos++;
        
        if (addDataLength >= cLength) resetFileAddValues();
      } else {
        resetFileAddValues();
        sendErr(request);
      }
    } else {
      resetFileAddValues();
      sendErr(request);
    }
  });

  server.on("/load", HTTP_POST, [](AsyncWebServerRequest* request){}, NULL, [](AsyncWebServerRequest* request, uint8_t* data, size_t dataLength, size_t dataIndex, size_t totalData) {
    bool failed = true;
    
    if (dataLength > 0) {
      String songFilePath = "";

      for (int i = 0; i < dataLength; i++) {
        songFilePath += char(data[i]);
      }

      songFilePath += ".bin";

      if (SPIFFS.exists(songFilePath)) {
        loadSong(songFilePath);
        failed = false;
      }
    }

    if (failed)
      sendErr(request);
    else
      request->send(200);
  });

  server.on("/loop", HTTP_POST, [](AsyncWebServerRequest* request){}, NULL, [](AsyncWebServerRequest* request, uint8_t* data, size_t dataLength, size_t dataIndex, size_t totalData) {
    if (dataLength > 0) {
      songLooped = data[0] > 0;
      request->send(200);
    } else {
      sendErr(request);
    }
  });

  server.on("/skipBack5", HTTP_POST, [](AsyncWebServerRequest* request){
    curNote = 0;
    ms -= 5000;
    msSubtract = millis() - ms;
    request->send(200);
  });

  server.on("/skipForward5", HTTP_POST, [](AsyncWebServerRequest* request){
    ms += 5000;
    msSubtract = millis() - ms;
    request->send(200);
  });

  server.on("/restart", HTTP_POST, [](AsyncWebServerRequest* request){
    restartSong();
    request->send(200);
  });

  server.on("/play", HTTP_POST, [](AsyncWebServerRequest* request){
    playSong();
    request->send(200);
  });

  server.on("/pause", HTTP_POST, [](AsyncWebServerRequest* request){
    pauseSong();
    request->send(200);
  });

  server.on("/delete", HTTP_POST, [](AsyncWebServerRequest* request){}, NULL, [](AsyncWebServerRequest* request, uint8_t* data, size_t dataLength, size_t dataIndex, size_t totalData) {
    bool failed = true;
    
    if (dataLength > 0) {
      String songFilePath = "";

      for (int i = 0; i < dataLength; i++) {
        songFilePath += char(data[i]);
      }

      pauseSong();
      updateMidi = false;
      headerReceived = false;
      fileDataHeader = 0;
      if (midiFile) midiFile.close();
      if (SPIFFS.exists(songFilePath + ".name")) SPIFFS.remove(songFilePath + ".name");
      if (SPIFFS.exists(songFilePath + ".bin")) SPIFFS.remove(songFilePath + ".bin");

      request->send(200);
    } else {
      sendErr(request);
    }
  });

  server.on("/setTime", HTTP_POST, [](AsyncWebServerRequest* request){}, NULL, [](AsyncWebServerRequest* request, uint8_t* data, size_t dataLength, size_t dataIndex, size_t totalData) {
    if (dataLength == 4) {
      curNote = 0;
      unsigned long songOffset = (data[0] << 24) + (data[1] << 16) + (data[2] << 8) + data[3];
      long mil = millis();
      startMs = mil;
      msAdder = songOffset;
      ms = startMs + msAdder;
      msSubtract = 0;
      request->send(200);
    } else {
      sendErr(request);
    }
  });

  server.on("/setDurationMultiplier", HTTP_POST, [](AsyncWebServerRequest* request){}, NULL, [](AsyncWebServerRequest* request, uint8_t* data, size_t dataLength, size_t dataIndex, size_t totalData) {
    if (dataLength > 0) {
      float reqDurMul = data[0];
      if (reqDurMul > 100.0) reqDurMul = 100.0;
      durationMultiplier = reqDurMul / 100.0;
      saveDurMul();
    } else {
      sendErr(request);
    }
  });
  
  server.on("/bootstrap/bootstrap.min.css", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(SPIFFS, "/bootstrap/bootstrap.min.css", "text/css");
  });

  server.on("/midi/Midi.js", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(SPIFFS, "/midi/Midi.js", "text/javascript");
  });

  server.on("/converter", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(SPIFFS, "/converter/index.html", "text/html");
  });

  server.on("/admin", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(SPIFFS, "/admin/index.html", "text/html");
  });
  
  server.on("/add", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(SPIFFS, "/add/index.html", "text/html");
  });

  server.on("/list", HTTP_GET, [](AsyncWebServerRequest* request) {
    updateDataList();
    String toSend = processFilesForSongNames();
    request->send(200, "text/plain", toSend);
  });

  server.on("/time", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "text/plain", String(ms - startMs) + " " + String(endMs));
  });

  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(SPIFFS, "/index.html", "text/html");
  });

  server.begin();
}

void updateSong() {
  if (updateMidi && midiFile && midiFile.available() && fileLen >= 12) {
    if (!headerReceived) {
      getHeader();
      getLength();
      getBPM();
      getPPQ();
      headerReceived = true;
    } else if (fileDataHeader == buzzerDataHeader) {
      if (!paused) {
        if (curNote + 1 >= songLen) {
          curNote = 0;
          
          if (!songLooped) {
            pauseSong();
          } else {
            restartSong();
          }

          return;
        }

        endMs = millisecondsFromTicks32(getNoteTime(songLen - 1));

        Serial.println("PPQ: " + String(ppq));
        Serial.println("BPM: " + String(bpm));
        Serial.println("Song Len: " + String(songLen));
        Serial.println("End MS: " + String(endMs));
        Serial.println("Start MS: " + String(startMs));
        for (int z = 0; z < songLen; z++) {
          Serial.println("Note " + String(z) + " Time: " + String(getNoteTime(z)));
        }
        Serial.println("-----");
        
        getNote();
  
        unsigned long noteMs = millisecondsFromTicks32(noteTime);
        long msDiff = ms - startMs;
        if (msDiff >= noteMs) {
          uint16_t durationMultiplied = millisecondsFromTicks16(noteDuration) * durationMultiplier;
          if (msDiff < noteMs + durationMultiplied) {
            tone(buzzer, notePitch, durationMultiplied);
          } else if (msDiff >= noteMs + durationMultiplied) {
            noTone(buzzer);
            curNote++;
          }
        }
      }
    }
  }
}

/*
String b64ify(String inputStr) {
  String outputStr = "";
  
  for (uint16_t i = 0; i < (uint16_t)inputStr.length(); i++) {
    bool stopAlphabet = false;
    for (uint16_t j = 0; j < Base64AlphabetSize; j++) {
      if (!stopAlphabet) {
        if ((byte)inputStr[i] == pgm_read_byte(&_Base64AlphabetTable[j])) {
          stopAlphabet = true;
          outputStr += inputStr[i];
        }
      }
    }
  }
  
  return outputStr;
}
*/

String decodeNfcAuthHeader() {
  char* headerCharArray = (char*)nfcAuthHeader.c_str();
  int headerLength = nfcAuthHeader.length();
  uint8_t decodedHeaderLength = (uint8_t)Base64.decodedLength(headerCharArray, headerLength);
  if (decodedHeaderLength == nahl) {
    char decodedHeader[nahl];
    Base64.decode(decodedHeader, headerCharArray, headerLength);
    return String(decodedHeader);
  }
  return "";
}

void startServer() {
  setupServer();
  serverStarted = true;
}

void setupAes() {
  key = "V0JBQzkyMTBEMzQxRzAxNQ==";
  iv = "SU5JVFZFQzU2ODEyOSRRUg==";

  setupKeys();

  uint8_t setCredType[4] = { 0x57, 0x69, 0x46, 0x69 };
  memcpy(credType, setCredType, sizeof(setCredType));
}

void setup() {
  setupAes();
  Serial.begin(115200);
  while (!Serial) {};
  bool fsOpen = SPIFFS.begin();
  if (!fsOpen) {
    Serial.println("SPIFFS cannot be started!");
    ESP.reset();
    return;
  }
  root = SPIFFS.openDir("/");
  loadDurMul();
  loadBV();
  loadAD();
  SPI.begin();
  rfid.PCD_Init();
  nfc.begin();
  pinMode(LED_BUILTIN, OUTPUT);
}

void playSuccess() {
  tone(buzzer, rfidBuzzerNoteSuccess, 250);
  delay(250);
  noTone(buzzer);
  delay(250);
}

void playFail() {
  tone(buzzer, rfidBuzzerNoteFailed, 500);
  delay(500);
  noTone(buzzer);
  delay(500);
}

bool clearNfc() {
  String s = spaceify("", ' ', 255);
  int l = s.length();
  byte p[l + 1];
  s.getBytes(p, l);

  NdefRecord r = NdefRecord();
  r.setTnf(NdefRecord::TNF::TNF_WELL_KNOWN);
  r.setType(credType, 4);
  r.setPayload(p, l);

  NdefMessage m = NdefMessage();
  bool a = m.addRecord(r);

  if (a)
    return nfc.write(m);
  else
    return false;
}

void loop() {
  // SERIAL COMMUNICATION CODE BETWEEN NODE.JS AND BOARD
  if (!startWriting) {
    if (Serial.available() > 0) {
      char character = Serial.read();
      if (startReading) {
        if (character == 0) {
          startReading = false;
          toWrite = decodeNfcAuthHeader() + "," + toWrite;
          toWrite = aesEncrypt(toWrite);
          startWriting = true;
        } else if (toWrite.length() < 110) {
          toWrite += character;
          startWriting = false;
        }
      } else if (character == '|') {
        startReading = true;
        startWriting = false;
      }
    }
  } else {
    if (nfc.tagPresent()) {
      if (clearNfc()) {
        int strLength = toWrite.length() + 1;
        byte nfcPayload[strLength];
        toWrite.getBytes(nfcPayload, sizeof(nfcPayload));
  
        NdefRecord record = NdefRecord();
        record.setTnf(NdefRecord::TNF::TNF_WELL_KNOWN);
        record.setType(credType, 4);
        record.setPayload(nfcPayload, strLength);
  
        NdefMessage message = NdefMessage();
        bool added = message.addRecord(record);
  
        if (added) {
          bool written = nfc.write(message);
  
          if (written) {
            Serial.println(successStr + toWrite + "\n");
            Serial.flush();
            toWrite = "";
            if (serverStarted) {
              ESP.reset();
              return;
            }
            Serial.println("Rechecking NFCs After 3 Seconds...");
            playSuccess();
            delay(2900);
            Serial.println("Back To Checking!");
            startWriting = false;
          } else {
            Serial.print(failedStr);
            Serial.flush();
          }
        } else {
          Serial.print(failedStr);
          Serial.flush();
        }
      } else {
        Serial.print(failedStr);
        Serial.flush();
      }
    }
  }
  // SERIAL COMMUNICATION CODE BETWEEN NODE.JS AND BOARD

  if (!authenticated) {
    if (!startReading && !startWriting) {
      bool tagDetected = nfc.tagPresent();
      if (tagDetected || (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial())) {
        unsigned long uid = (rfid.uid.uidByte[0] << 24) + (rfid.uid.uidByte[1] << 16) + (rfid.uid.uidByte[2] << 8) + rfid.uid.uidByte[3];
        Serial.println("UID: " + String(uid, HEX));
  
        noTone(buzzer);
        
        if (uid == authUID && tagDetected) {
          Serial.println("NFC Detected");
          NfcTag tag = nfc.read();
          if (tag.hasNdefMessage()) {
            Serial.println("Message Detected");
            NdefMessage message = tag.getNdefMessage();
            unsigned int recordCount = message.getRecordCount();
            bool stopRecordLoop = false;
            String authHeader = decodeNfcAuthHeader();
            int dataLength = 0;
            String nfcData = "";
            for (int a = 0; a < recordCount; a++) {
              if (!stopRecordLoop) {
                Serial.println("Record Detected");
                NdefRecord record = message.getRecord(a);
                if (record.getTnf() == NdefRecord::TNF::TNF_WELL_KNOWN) {
                  Serial.println("Valid TNF");
                  unsigned int recordTypeLength = record.getTypeLength();
                  byte* recordTypeBytes = (byte*)record.getType();
                  String recordType = "";
                  for (int b = 0; b < recordTypeLength; b++) {
                    recordType += char(recordTypeBytes[b]);
                  }
                  if (recordType.indexOf(String((char*)credType)) != -1) {
                    Serial.println("Valid Type");
                    uint16_t payloadLength = (uint16_t)record.getPayloadLength();
                    
                    if (payloadLength > 0) {
                      String payloadText = String((char*)record.getPayload());
                      payloadText.trim();
                      Serial.println(payloadText);
                      Serial.println(aesEncrypt(payloadText));
                      nfcData = aesDecrypt(payloadText);
                      Serial.println(nfcData);
                      Serial.println("ok");
                      payloadText = "";
                      if (nfcData.length() > 0) stopRecordLoop = true;
                    }
                  }
                }
              }
            }
            if (stopRecordLoop) {
              if (nfcData.length() == nahl + 8) {
                if (nfcData.substring(0, nahl) == authHeader.substring(0, nahl)) {
                  nfcData = nfcData.substring(nahl + 1);
                  nfcData.trim();
                  if (nfcData == "USEFILE") {
                    playSuccess();
                    authenticated = true;
                  } else {
                    playFail();
                  }
                } else {
                  playFail();
                }
              } else if (nfcData.length() >= nahl + minimumPayloadData) {
                if (nfcData.substring(0, nahl) == authHeader.substring(0, nahl)) {
                  nfcData = nfcData.substring(nahl + 1);
                  uint8_t ssidLen = (uint8_t)nfcData.substring(0, 2).toInt();
                  nfcData = nfcData.substring(3);
                  if (nfcData.length() >= ssidLen + 2) {
                    if (clearNfc()) {
                      wifiSSID = nfcData.substring(0, ssidLen);
                      nfcData = nfcData.substring(ssidLen + 1);
                      wifiPass = nfcData;
                      
                      String newPayloadStr = authHeader + ",USEFILE";
                      newPayloadStr = aesEncrypt(newPayloadStr);
                      
                      int strLength = newPayloadStr.length() + 1;

                      byte newPayload[strLength];
                      newPayloadStr.getBytes(newPayload, sizeof(newPayload));
    
                      NdefRecord newRecord = NdefRecord();
                      newRecord.setTnf(NdefRecord::TNF::TNF_WELL_KNOWN);
                      newRecord.setType(credType, 4);
                      newRecord.setPayload(newPayload, strLength);
    
                      NdefMessage newMessage = NdefMessage();
                      bool added = newMessage.addRecord(newRecord);
                      if (added) {
                        bool written = nfc.write(newMessage);
    
                        if (written) {
                          saveBV();
                          playSuccess();
                          authenticated = true;
                        } else {
                          playFail();
                        }
                      } else {
                        playFail();
                      }
                    } else {
                      playFail();
                    }
                  } else {
                    playFail();
                  }
                } else {
                  playFail();
                }
              } else {
                playFail();
              }
            } else {
              playFail();
            }
          } else {
            playFail();
          }
        } else {
          playFail();
        }
      }
    }
    digitalWrite(LED_BUILTIN, HIGH);
  } else {
    if (startUpdating) {
      if (!serverStarted) {

        WiFi.mode(WIFI_STA);
        // String ssidStr = wifiSSID;
        // String passStr = wifiPass;
        WiFi.begin(wifiSSID, wifiPass);

        uint8_t retries = 0;
        while (WiFi.status() != WL_CONNECTED && retries < 100) {
          if (retries % 5 == 0) Serial.print(".");
          delay(100);
          retries++;
        }
        Serial.println("");
        
        if (WiFi.status() != WL_CONNECTED) {
          wifiSSID = "";
          wifiPass = "";

          saveBV();
          
          Serial.println("Failed!");
          Serial.println("I'll Start The AP Fallback...");

          WiFi.mode(WIFI_AP);
          WiFi.softAPConfig(staticIp, gateway, subnet);
          WiFi.softAP(apSSID, apPass);
          Serial.println("IP: " + String(WiFi.softAPIP().toString()));

          liveServer.begin();
          startServer();
          playFail();
        } else {
          IPAddress wifiIP = WiFi.localIP();
          IPAddress staStaticIp = wifiIP;
          IPAddress staGateway = staStaticIp;
          staGateway[3] = 1;
          staStaticIp[3] = 210;
          WiFi.config(staStaticIp, staGateway, subnet);
          wifiIP = WiFi.localIP();
          Serial.println("IP: "+ wifiIP.toString());

          liveServer.begin();
          startServer();
          playSuccess();
        }
      } else {
        bool loadUpdate = true;
        
        client = liveServer.available();

        if (client)
        {
          if (client.connected())
          {
            paused = true;
            Serial.println("checking");
            if (client.available() > 0)
            {
              uint8_t cd = client.read();
              Serial.println(cd);
              switch (ldp)
              {
                case 0:
                  ldf += cd << 8;
                  break;
                case 1:
                  ldf += cd;
                  break;
              }
              
              ldp++;
              if (ldp >= ldm) ldp = 0;

              if (ldp == 0)
              {
                if (ldf == 0)
                  noTone(buzzer);
                else
                  tone(buzzer, ldf);
                ldf = 0;
              }
            }

            loadUpdate = false;
          }
          else
          {
            client.stop();
            // loadUpdate = true;
            // paused = prevPaused;
            // prevPaused = false;
            paused = false;
          }
        }
        else
          updateSong();

        if (paused) {
          msSubtract = millis() - (ms - msAdder);
        } else {
          long mil = millis();
          
          if (!(mil > 0)) {
            msSubtract = 0;
            msAdder = 0;
          }
          
          ms = (mil - msSubtract) + msAdder;
        }
    
        digitalWrite(LED_BUILTIN, LOW);
      }
    } else {
      updateDataList();
    }
  }
}
