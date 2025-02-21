#include <SoftwareSerial.h>
#include <Ethernet.h>
#include <avr/wdt.h>
#include "HX711.h"
#include <EEPROM.h>
#include <floatToString.h>
#define rxPin 2
#define txPin 3
const int DT_PIN = 6;
const int SCK_PIN = 5;



//============================使用者設定======================
String SiteName1 = "L8B_1F";
String SiteName2 = "L8B_1F"; //如果有第二個按鈕
byte dhcp = 0;                //自動取得IP 0 為固定IP  1為自動取得
byte mac[] = {0xFE, 0x19, 0x19, 0x02, 0x23, 0x31 };
IPAddress ip(10, 97, 238, 146);
IPAddress mask(255, 255, 254, 0);
IPAddress gateway(10, 97, 239, 254);

//IPAddress proxy(10, 31, 10, 188);
IPAddress proxy(10, 97, 4, 1);
int ProxyPort = 8080;  //0為不使用
IPAddress serverOA(10, 97, 4, 1);
float scale_factor = 1200; 
int scale_factor_int = 1200; 

//===========================================================
IPAddress dns(0, 0, 0, 0);   //不用設定
 //比例參數，從校正程式中取得
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
    Val1 = scale_factor_int % 256 ;
    Val2 = scale_factor_int>>8;
  }
  scale_eeprom = Val1 + Val2 * 256;
  Serial.println(scale_eeprom);
  if (scale_eeprom <= 0 or scale_eeprom >=30000 ) {
    scale_eeprom = scale_factor_int;
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
      char S[15];
      //strValue = strValue.substring(1, strValue.length() - 3);
      if( weight < 0.0)
      {
        weight=0.0;
      }
      floatToString(weight, S, sizeof(S), 2);
      String Str=String(S);
      sendJsonData(SiteName1, bagtype[CheckType.toInt()], Str, "KG");
    } else if (readbyte == 0x56) {
      String strValue = mySerial.readStringUntil(0xFF);
      String CheckType = strValue.substring(0, 1);
      strValue = strValue.substring(1, strValue.length() - 3);
      strValue = strValue.substring(1, strValue.length() - 3);
      char S[15];
      //strValue = strValue.substring(1, strValue.length() - 3);
      if( weight < 0.0)
      {
        weight=0.0;
      }
      floatToString(weight, S, sizeof(S), 2);
      String Str=String(S);

      sendJsonData(SiteName2, bagtype[CheckType.toInt()],Str, "KG");
    } else if (readbyte == 0x1) {
      //換頁
      page = 1;
    } else if (readbyte == 0x2) {
      //換頁
      page = 0;

    } else if (readbyte == 0x08) {
      mySerial.print(F("page0.t0.txt=\"Zero...\""));
      mySerial.print(endstr);
      scale.tare();                   // 歸零
    }else if (readbyte == 0x6) {
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
      
    } else if (readbyte == 0x3) {
     
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

