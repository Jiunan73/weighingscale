#include <SoftwareSerial.h>
#include <Ethernet.h>
#include <avr/wdt.h>
#include "HX711.h"
#include <EEPROM.h>
#define rxPin 2
#define txPin 3
const int DT_PIN = 6;
const int SCK_PIN = 5;
const uint16_t CRC16_POLY = 0xA001;
const uint16_t CRC16_INIT = 0xFFFF;


//============================使用者設定======================
String SiteName1 = "L8A6F";
String SiteName2 = "L8A6F";
byte dhcp = 1;
byte mac[] = { 0xFA, 0xF0, 0xF0, 0x00, 0x00, 0x05 };
//L5C 
//L1F 0x8A, 0x01, 0x1A, 0x01, 0x1F, 0xC1
//L4F 0x8A, 0x04, 0x4A, 0x04, 0x40, 0xC4
//L6F 0x8A, 0x06, 0x6C, 0x06, 0x6F, 0xC6
//K3F 0xDE, 0xDE, 0xBE, 0xEF, 0xF0, 0x10
//
//MySQL_Connection conn((Client *)&client);
IPAddress ip(10, 97, 238, 146);
IPAddress mask(255, 255, 254, 0);
IPAddress gateway(10, 97, 239, 254);
IPAddress dns(0, 0, 0, 0);
//IPAddress proxy(10, 31, 10, 188);
IPAddress proxy(10, 97, 4, 1);
int ProxyPort = 8080;  //0為不使用
IPAddress serverOA(10, 96, 192, 93);
//===========================================================
float scale_factor = 1.66;  //比例參數，從校正程式中取得
int scale_eeprom = 0;       //比例參數，從校正程式中取得
//const float scale_factor = 101;  //比例參數，從校正程式中取得
HX711 scale;
EthernetClient client;
char endstr[4];
String bagtype[] = {
  "",               // 索引0
  "OrganicWiper",   // 索引1
  "GeneralWaste",   // 索引2
  "RecyclePlastic"  // 索引3
};

//IPAddress serverOA(10, 96, 148, 79);
// Set up a new SoftwareSerial object
SoftwareSerial mySerial = SoftwareSerial(rxPin, txPin);
unsigned long previousMillis = 0;  // 保存上一次发送数据的时间
const long interval = 1000;        // 间隔时间，单位为毫秒
int page = 0;
void setup() {
  int Val1;
  int Val2;
  
  // Define pin modes for TX and RX
  sprintf(endstr, "\xff\xff\xff");
  pinMode(rxPin, INPUT);
  pinMode(txPin, OUTPUT);
  mySerial.begin(9600);
  Serial.begin(9600);
  mySerial.print(F("page page0"));
  mySerial.print(endstr);
  mySerial.print(F("page0.t0.txt=\"Init\""));
  mySerial.print(endstr);
  Val1 = EEPROM.read(0);
  Val2 = EEPROM.read(1);
  if (Val1 == 0 and Val2 == 0) {
    Val1 = 166;
    Val2 = 0;
  }
  ReadNet();
  scale_eeprom = Val1 + Val2 * 256;
  Serial.println(scale_eeprom);
  if (scale_eeprom <= 0) {
    scale_eeprom = 166;
  }

  scale_factor = float(scale_eeprom) / 100.0;

  scale.begin(DT_PIN, SCK_PIN);

  //Serial.println(scale.get_units(5), 0);  //未設定比例參數前的數值
  scale.set_scale(scale_factor);  // 設定比例參數
  scale.tare();                   // 歸零
  Serial.println(F("After setting up the scale:"));
  Serial.println(scale.get_units(5), 0);  //設定比例參數後的數值
       //在這個訊息之前都不要放東西在電子稱上

  if (dhcp == 1) {
    Serial.println(F("DHCP"));    
    mySerial.print(F("page0.t0.txt=\"Get IP\""));
    mySerial.print(endstr);
    Serial.println(mac[0]);
    Serial.println(mac[1]);
    Serial.println(mac[2]);
    Serial.println(mac[3]);
    Serial.println(mac[4]);
    Serial.println(mac[5]);
    Ethernet.begin(mac);

    ip = Ethernet.localIP();
    gateway = Ethernet.gatewayIP();
    mask = Ethernet.subnetMask();
    if (ip[0] == 0) {
      mySerial.print(F("page0.t0.txt=\"IP Err\""));
      mySerial.print(endstr);
      delay(5000);
      wdt_enable(100);
    }

  } else {
    Ethernet.begin(mac, ip, dns, gateway, mask);
  }
  mySerial.print(F("page0.t0.txt=\"0.0\""));
  mySerial.print(endstr);
  mySerial.print(F("page1.x0.val="));
  mySerial.print(scale_eeprom);
  mySerial.print(endstr);
  Serial.print(F("page1.x0.val="));
  Serial.println(scale_eeprom);
  Serial.print(F("My IP address: "));
  Serial.println(Ethernet.localIP());
  Serial.println(mac[0]);
  Serial.println(mac[1]);
  Serial.println(mac[2]);
  Serial.println(mac[3]);
  Serial.println(Ethernet.gatewayIP());
  Serial.println(Ethernet.subnetMask());

  delay(1000);
  SendNet();
}

void loop() {
  float weight = scale.get_units(10) / 1000;

  if (page == 0) {
    mySerial.print(F("page0.t0.txt=\""));
    mySerial.print(weight, 1);
    mySerial.print(F("\""));
    mySerial.print(endstr);


    //Serial.print(weight, 1);
    //Serial.println(weight, 3);
  } else {
  }

  if (mySerial.available()) {
    Serial.print(F("R:"));
    Serial.println(mySerial.available());
    byte readbyte = mySerial.read();
    Serial.println(readbyte, HEX);
    if (readbyte == 0x55) {
      String strValue = mySerial.readStringUntil(0xFF);
      String CheckType = strValue.substring(0, 1);
      strValue = strValue.substring(1, strValue.length() - 3);
      Serial.println(readbyte);
      Serial.println(SiteName1);
      Serial.println(strValue);
      Serial.println(weight);
      Serial.print(F("KG"));
      sendJsonData(SiteName1, bagtype[CheckType.toInt()], strValue.c_str(), "KG");
    } else if (readbyte == 0x56) {
      String strValue = mySerial.readStringUntil(0xFF);
      String CheckType = strValue.substring(0, 1);
      strValue = strValue.substring(1, strValue.length() - 3);
      Serial.println(readbyte);
      Serial.println(SiteName2);
      Serial.println(strValue);
      Serial.println(weight);
      Serial.print(F("KG"));
      sendJsonData(SiteName2, bagtype[CheckType.toInt()], strValue.c_str(), "KG");
    } else if (readbyte == 0x1) {
      //換頁
      page = 1;
    } else if (readbyte == 0x2) {
      //換頁
      page = 0;

    } else if (readbyte == 0x6) {
      //重開機
      page = 0;
      byte receivedByte[3];
      for (int i = 0; i < 3; i++) {
        receivedByte[i] = mySerial.read();
      }
      if (receivedByte[0] == 0xFF && receivedByte[1] == 0xFF && receivedByte[2] == 0xFF) {
        mySerial.print(F("page page0"));
        mySerial.print(endstr);

        wdt_enable(100);
      }

    } else if (readbyte == 0x7) {
      //計算
      Serial.println(F("scale_factor"));
      Serial.println(scale_factor);
      Serial.println(weight);  //kg
                               //反推5KG的值
      float temp = weight / 5.0;
      Serial.println(scale_factor * temp);
      scale_factor = scale_factor * temp;
      scale_eeprom = scale_factor * 100;
      Serial.println(scale_eeprom);
      EEPROM.write(0, scale_eeprom % 256);
      EEPROM.write(1, scale_eeprom >> 8);
      mySerial.print(F("page page0"));
      mySerial.print(endstr);
      mySerial.print(F("page0.t0.txt=\"remove\""));
      mySerial.print(endstr);
      delay(5000);
      mySerial.print(F("page0.t0.txt=\"reboot\""));
      mySerial.print(endstr);
      delay(1000);
      wdt_enable(100);
    } else if (readbyte == 0x4) {
      //Set SiteName1

      Serial.println(F("Set SiteName1"));
      byte name[20];
      int len = 0;

      while (true) {
        // 讀取一個字節
        if (mySerial.available() == 0) {
          break;
        }
        byte receivedByte = mySerial.read();
        if (receivedByte == 0xFF) {
          Serial.println(F("break;"));
          break;  // 如果讀取到0xFF，結束循環
        }
        if (len > 10) {
          Serial.println(F("break;"));
          break;  // 如果讀取到0xFF，結束循環
        }
        name[len] = receivedByte;
        Serial.print(F("Received byte["));
        Serial.print(len);
        Serial.print(F("]:"));
        Serial.println(name[20]);
        len++;
      }
      String str = "";
      for (int i = 0; i < 20; i++) {
        if (name[i] == '\0') break;  // 遇到空字符終止
        str += (char)name[i];
      }
      int commaIndex = str.indexOf(',');
      if (commaIndex != -1) {
        SiteName1 = str.substring(0, commaIndex);
        SiteName2 = str.substring(commaIndex + 1);
      } else {
        SiteName1 = str;
        SiteName2 = "L7AB1F";
      }
      Serial.println(SiteName1);
      Serial.println(SiteName2);
      for (int i = 0; i < 20; i++) {
        EEPROM.write(30 + i, name[i]);
      }
    } else if (readbyte == 0x3) {
      //網路設定
      byte Val1 = mySerial.read();
      byte Val2 = mySerial.read();  // 包括結束符'\0'
      Serial.println(F("SetIP"));
      Serial.println(Val1, HEX);
      Serial.println(Val2, HEX);
      EEPROM.write(0, Val1);
      EEPROM.write(1, Val2);
      bool checkflag = true;
      //dhcp 1+4+4+4+6+4+2
      byte netsetting[50];
      int len = 0;
      while (true) {
        // 讀取一個字節
        if (mySerial.available() == 0) {
          break;
        }
        byte receivedByte = mySerial.read();
        if (receivedByte == 0xFF && len >= 27) {
          Serial.println(F("break;"));
          break;  // 如果讀取到0xFF，結束循環
        }
        netsetting[len] = receivedByte;
        Serial.print(F("Received byte["));
        Serial.print(len);
        Serial.println(netsetting[len]);
        len++;
      }
      uint16_t crc = calculateCRC16(netsetting, 25);
      Serial.print(F("Crc="));
      Serial.print(crc, HEX);
      Serial.print(F(","));

      Serial.println((netsetting[25] | netsetting[26] << 8), HEX);

      if (len == 27 && crc == netsetting[25] | netsetting[26] << 8) {
        Serial.println(F("write EEPROM"));
        for (int i = 0; i < 25; i++) {
          EEPROM.write(i + 2, netsetting[i]);
        }
      }
    } else {
      Serial.println(F("clear:"));
      while (mySerial.available() > 0) {
        mySerial.read();  // 讀取並丟棄數據
      }
    }
  }

  delay(200);
}

void sendJsonData(String bs002, String bs003, String bs004, String bs005) {
  String bs001 = String(millis() / 1000);
  String PostStr = " ";  //"GET http://10.96.192.93/bagscales?bs001=" + bs001 + "&bs002=" + bs002 + "&bs003=" + bs003 + "&bs004=" + bs004 + "&bs005=" + bs005 + " HTTP/1.1";

  Serial.println(F("Send Data"));
  Serial.println(ProxyPort);
  if (ProxyPort == 0) {
    Serial.println(F("Send Data2"));
    PostStr = "POST /bagscales?bs001=" + bs001 + "&bs002=" + bs002 + "&bs003=" + bs003 + "&bs004=" + bs004 + "&bs005=" + bs005 + " HTTP/1.1";
    Serial.println(PostStr);

    if (client.connect(serverOA, 80)) {
      Serial.println(F("Connected to server"));

      client.println(PostStr);

      client.println(F("Host: arduino"));

      client.println();
      //client.println(jsonData);
      mySerial.print(F("page0.t0.txt=\"OK\""));
      mySerial.print(endstr);
      mySerial.print(F("play 0,1,0"));
      mySerial.print(endstr);

      // 等待服务器响应
      unsigned long timeout = millis();
      while (client.connected() && millis() - timeout < 3000) {
        if (client.available()) {
          char c = client.read();
          Serial.print(c);
          timeout = millis();
        }
      }
      Serial.println(F("\nDisconnecting..."));
      client.stop();
    } else {
      Serial.println(F("Send Data3"));
      mySerial.print(F("page0.t0.txt=\"NG\""));
      mySerial.print(endstr);
      mySerial.print(F("play 0,2,0"));
      mySerial.print(endstr);
      delay(4000);
    }

  } else if (client.connect(proxy, ProxyPort)) {
    PostStr = "GET http://10.96.192.93/bagscales?bs001=" + bs001 + "&bs002=" + bs002 + "&bs003=" + bs003 + "&bs004=" + bs004 + "&bs005=" + bs005 + " HTTP/1.1";
    Serial.println(F("Connected to Proxy"));
    Serial.println(F("Send Data4"));
    // 发送HTTP POST请求
    Serial.println(PostStr);
    client.println(PostStr);

    client.println(F("Host: arduino"));


    client.println();

    mySerial.print(F("page0.t0.txt=\"OK\""));
    mySerial.print(endstr);
    mySerial.print(F("play 0,1,0"));
    mySerial.print(endstr);

    // 等待服务器响应
    unsigned long timeout = millis();
    while (client.connected() && millis() - timeout < 3000) {
      if (client.available()) {
        char c = client.read();
        Serial.print(c);
        timeout = millis();
      }
    }
    Serial.println(F("\nDisconnecting..."));
    client.stop();
  } else {
    Serial.println(F("Send Data5"));
    mySerial.print(F("page0.t0.txt=\"NG\""));
    mySerial.print(endstr);
    mySerial.print(F("play 0,2,0"));
    mySerial.print(endstr);
    delay(4000);
  }
}
void ReadNet() {
  /*dhcp = EEPROM.read(2);
    delay(10);
  ip[0] = EEPROM.read(3);
  delay(10);
  ip[1] = EEPROM.read(4);
  delay(10);
  ip[2] = EEPROM.read(5);
  delay(10);
  ip[3] = EEPROM.read(6);
delay(10);
  mask[0] = EEPROM.read(7);
  delay(10);
  mask[1] = EEPROM.read(8);
  delay(10);
  mask[2] = EEPROM.read(9);
  delay(10);
  mask[3] = EEPROM.read(10);
  delay(10);
  gateway[0] = EEPROM.read(11);
  delay(10);
  gateway[1] = EEPROM.read(12);
  delay(10);
  gateway[2] = EEPROM.read(13);
  delay(10);
  gateway[3] = EEPROM.read(14);
  delay(10);
  mac[0] = EEPROM.read(15);
  delay(10);
  mac[1] = EEPROM.read(16);
  delay(10);
  mac[2] = EEPROM.read(17);
  delay(10);
  mac[3] = EEPROM.read(18);
  delay(10);
  mac[4] = EEPROM.read(19);
  delay(10);
  mac[5] = EEPROM.read(20);
  delay(10);
  proxy[0] = EEPROM.read(21);
  delay(10);
  proxy[1] = EEPROM.read(22);
  delay(10);
  proxy[2] = EEPROM.read(23);
  delay(10);
  proxy[3] = EEPROM.read(24);
  delay(10);
  ProxyPort = EEPROM.read(25) | (EEPROM.read(26) << 8);
  Serial.print(F("ProxyPort:"));
  Serial.println(ProxyPort);*/
  SiteName1 = "M02";
  SiteName2 = "M02";
  for (int i = 0; i < 30; i++) {
    Serial.print(F("EEPROM"));
    Serial.print(i);
    Serial.print(F(":"));
    Serial.println(EEPROM.read(i));
  }
  String str = "";
  for (int i = 0; i < 20; i++) {
    byte r = EEPROM.read(30 + i);
    if (r >= 32 && r <= 126) {
      str += char(EEPROM.read(30 + i));
    } else {
      break;
    }
  }

  int commaIndex = str.indexOf(',');
  if (commaIndex != -1) {
    SiteName1 = str.substring(0, commaIndex);
    SiteName2 = str.substring(commaIndex + 1);
  } else {
    SiteName1 = "M0201";
    SiteName2 = "M0202";
  }
}
void SendNet() {
  mySerial.print(F("page1.dhcp.val="));
  mySerial.print(dhcp);
  mySerial.print(endstr);


  for (int i = 0; i <= 3; i++) {
    mySerial.print(F("page1.ip"));
    mySerial.print(i + 1);
    mySerial.print(F(".val="));
    mySerial.print(ip[i]);
    mySerial.print(endstr);

    mySerial.print(F("page1.mask"));
    mySerial.print(i + 1);
    mySerial.print(F(".val="));
    mySerial.print(mask[i]);
    mySerial.print(endstr);

    mySerial.print(F("page1.gateway"));
    mySerial.print(i + 1);
    mySerial.print(F(".val="));
    mySerial.print(gateway[i]);
    mySerial.print(endstr);

    mySerial.print(F("page1.mac"));
    mySerial.print(i + 1);
    mySerial.print(F(".val="));
    mySerial.print(mac[i]);
    mySerial.print(endstr);

    mySerial.print(F("page1.proxy"));
    mySerial.print(i + 1);
    mySerial.print(F(".val="));
    mySerial.print(proxy[i]);
    mySerial.print(endstr);
  }
  mySerial.print(F("page1.mac5.val="));
  mySerial.print(mac[4]);
  mySerial.print(endstr);
  mySerial.print(F("page1.mac6.val="));
  mySerial.print(mac[5]);
  mySerial.print(endstr);

  mySerial.print(F("page1.proxy5.val="));
  mySerial.print(ProxyPort);
  mySerial.print(endstr);

  mySerial.print(F("page1.SiteName1.txt=\""));
  mySerial.print(SiteName1);
  mySerial.print(F("\""));
  mySerial.print(endstr);
  mySerial.print(F("page1.SiteName2.txt=\""));
  mySerial.print(SiteName2);
  mySerial.print(F("\""));
  mySerial.print(endstr);

  mySerial.print(F("page0.SiteName1.txt=\""));
  mySerial.print(SiteName1);
  mySerial.print(F("\""));
  mySerial.print(endstr);
  mySerial.print(F("page0.SiteName2.txt=\""));
  mySerial.print(SiteName2);
  mySerial.print(F("\""));
  mySerial.print(endstr);
}
uint16_t calculateCRC16(const uint8_t* data, size_t length) {
  uint16_t crc = CRC16_INIT;
  for (size_t i = 0; i < length; i++) {
    crc ^= data[i];
    for (uint8_t j = 0; j < 8; j++) {
      if (crc & 0x0001) {
        crc = (crc >> 1) ^ CRC16_POLY;
      } else {
        crc >>= 1;
      }
    }
  }
  return crc;
}
