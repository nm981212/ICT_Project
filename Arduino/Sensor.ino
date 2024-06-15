/*
프로젝트 : 산불 감지 시스템
코드 : 아두이노 센서부
*/
#include <WiFiEsp.h>
#include <SoftwareSerial.h>
#include <MsTimer2.h>
#include <DHT.h>
#include <TinyGPS.h>

// 상태 확인용 LCD
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// 서버 접속용 MACRO
#define AP_SSID "embC"
#define AP_PASS "embC1234"
#define SERVER_NAME "10.10.14.59"
#define SERVER_PORT 5000
#define LOGID "SEN_ARD"
#define PASSWD "PASSWD"

// 센서 핀 번호
#define FLAME_PIN 2
#define DHTPIN 4
#define WIFIRX 6  //6:RX-->ESP8266 TX
#define WIFITX 7  //7:TX -->ESP8266 RX
#define LED_BUILTIN_PIN 13
#define CO2_PIN A2

#define CMD_SIZE 50
#define ARR_CNT 5
#define DHTTYPE DHT11

// flag 변수 생성
bool timerIsrFlag = false;
bool sensorOnFlag = false;
bool sensorSet = false;
bool gpsFlag = false;

char sendBuf[CMD_SIZE];

char lcdLine1[17] = "FIRE DETECTOR";
char lcdLine2[17] = "WiFi Connecting!";

unsigned int secCount;

char getSensorId[10];
volatile int sensorTime;
int co2_threshold = 0;
int read_cnt = 0;

// 센서값 받아올 변수
float temp = 0.0;
float humi = 0.0;
int isfireDetected = LOW;
int co2 = 0;
int co2_percent = 0;
DHT dht(DHTPIN, DHTTYPE);

// 와이파이 설정
SoftwareSerial wifiSerial(WIFIRX, WIFITX);
WiFiEspClient client;

// gps 설정 - 하드웨어 시리얼 사용
TinyGPS gps;
float latitude, longitude;  // 위도, 경도를 저장할 변수 선언
// 위도, 경도값을 보내기 위한 문자열
char latStr[12];
char lonStr[12];

// LCD 설정
LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
  // LCD
  lcd.init();
  lcd.backlight();
  lcdDisplay(0, 0, lcdLine1);
  lcdDisplay(0, 1, lcdLine2);

  // 핀 설정
  pinMode(LED_BUILTIN_PIN, OUTPUT); //D13
  pinMode(FLAME_PIN, INPUT);
  pinMode(CO2_PIN, INPUT);

  wifi_Setup();
  Serial.begin(9600);

  MsTimer2::set(1000, timerIsr); // 1000ms period
  MsTimer2::start();

  dht.begin();
}

void loop() {
  if (client.available()) {
    socketEvent();
  }

  if (timerIsrFlag) //1초에 한번씩 실행
  {
    timerIsrFlag = false;

    // 센서의 임계값 설정(co2감지센서)
    if(sensorSet)
    { 
      if(sensorTime--)
        co2_threshold += analogRead(CO2_PIN);
      else  
      {
        co2_threshold = co2_threshold / read_cnt + 150;
        sensorSet = false;
        sprintf(sendBuf, "[ALLMSG]SET SENSOR COMPLETE\n");
        client.write(sendBuf, strlen(sendBuf));
        client.flush();
      }
    }

    // GPS센서 셋팅
    if(gpsFlag)
    {
      bool newData = false;
      if(Serial.available())
      {
        char c = Serial.read();
        
        if (gps.encode(c)) // 새로운 유효값이 들어왔는지 확인
          newData = true;
      }
      
      // 위도, 경도값 확인
      if (newData)
      {
        unsigned long age;
        gps.f_get_position(&latitude, &longitude, &age);
      }

      if (latitude != TinyGPS::GPS_INVALID_F_ANGLE && longitude != TinyGPS::GPS_INVALID_F_ANGLE)
      {
        gpsFlag = false;
        sprintf(sendBuf, "[ALLMSG]SET GPS COMPLETE\n");
        client.write(sendBuf, strlen(sendBuf));
        client.flush();
      }
    }

    // SENSOR@ON이라는 명령어가 들어오면 실행
    if (sensorOnFlag) 
    {
      // 센서 값 받아옴
      humi = dht.readHumidity();
      temp = dht.readTemperature();
      co2 = analogRead(CO2_PIN);
      isfireDetected = digitalRead(FLAME_PIN);
      co2_percent = map(co2, 0, 1023, 0, 100);

      // 센서 데이터를 지정된 시간에 따라 DB로 보냄
      if (sensorTime != 0 && !(secCount % sensorTime))
      {
        dtostrf(latitude, 9, 6, latStr);    //37.542304   9:전체자리수, 6:소수이하 자리수
        dtostrf(longitude, 10, 6, lonStr);  //126.841102  10:전체자리수, 6:소수이하 자리수
        sprintf(sendBuf, "[%s]SENSOR@%d@%d@%d@%s@%s@%s\r\n", getSensorId, (int)temp, (int)humi, co2_percent, isfireDetected ? "DETECT":"UNDETECT", latStr, lonStr);
        client.write(sendBuf, strlen(sendBuf));
        client.flush();
      }

      // 산불 감지가 되었는지 확인
      if(isfireDetected == HIGH || co2 > co2_threshold)
      {
        sprintf(sendBuf, "[CAM_JET]CAM@ON\n");
        client.write(sendBuf, strlen(sendBuf));
        client.flush();
        sensorOnFlag = false;
      }
      else if(temp > 30 && humi < 30) // 주변 환경 정보에 대해서 확인함
      {
        sprintf(sendBuf, "[ALLMSG]FIREWARN\n");
        client.write(sendBuf, strlen(sendBuf));
        client.flush();
      }
    }
  }
}

// 명령어 파싱 후 확인
void socketEvent()
{
  int i = 0;
  char * pToken;
  char * pArray[ARR_CNT] = {0};
  char recvBuf[CMD_SIZE] = {0};
  int len;

  sendBuf[0] = '\0';
  len = client.readBytesUntil('\n', recvBuf, CMD_SIZE);
  client.flush();

  // 서버에서 들어온 명령어를 []와 @를 구분자로 사용해서 분리한다.
  pToken = strtok(recvBuf, "[@]");
  while (pToken != NULL)
  {
    pArray[i] =  pToken;
    if (++i >= ARR_CNT)
      break;
    pToken = strtok(NULL, "[@]");
  }
  
  if ((strlen(pArray[1]) + strlen(pArray[2])) < 16)
  {
    // 들어온 명령어를 LCD에 출력한다.
    sprintf(lcdLine2, "%s %s", pArray[1], pArray[2]);
    lcdDisplay(0, 1, lcdLine2);
  }
  if (!strncmp(pArray[1], " New", 4)) // New Connected
  {
    strcpy(lcdLine2, "Server Connected");
    return ;
  }
  else if (!strncmp(pArray[1], " Alr", 4)) //Already logged
  {
    client.stop();
    server_Connect();
    return ;
  }
  else if (!strncmp(pArray[1], "SET", 3)) // [???_ARD]SET@SENSOR@10 OR [???_ARD]SET@GPS
  {
    if(!strncmp(pArray[2], "SENSOR", 6))
    {
      sensorSet = true;
      if (pArray[3] != NULL)
        sensorTime = atoi(pArray[3]);
      else
        sensorTime = 10;

      co2_threshold = 0;  // 임계값 초기화
      read_cnt = sensorTime;
    }
    else if(!strncmp(pArray[2], "GPS", 3))
    {
      if(pArray[3] == NULL)
        gpsFlag = true;
      else if(!strncmp(pArray[3], "INDOOR", 6))
      {
        // 실내에서 측정하는 경우 미리 지정한 값을 설정함
        latitude = 37.542304;
        longitude = 126.841102;
      }
    }
    else
      return;

    sprintf(sendBuf, "[%s]%s@%s\n", pArray[0], pArray[1], pArray[2]);
  }
  else if (!strncmp(pArray[1], "GET", 3)) // [???_ARD]GET@SENSOR@10 OR [???_ARD]GET@GPS
  {
    if(!strncmp(pArray[2], "SENSOR", 6)) // 센서 데이터 전송
    {
      if (pArray[3] != NULL)
      {
        sensorTime = atoi(pArray[3]);
        strcpy(getSensorId, pArray[0]);
        return;
      }
      else
      {
        sensorTime = 0;
        dtostrf(latitude, 9, 6, latStr);    //37.542304   9:전체자리수, 6:소수이하 자리수
        dtostrf(longitude, 10, 6, lonStr);  //126.841102  10:전체자리수, 6:소수이하 자리수
        sprintf(sendBuf, "[%s]%s@%d@%d@%d@%s@%s@%s\r\n", pArray[0], pArray[2], (int)temp, (int)humi, co2_percent, isfireDetected ? "DETECT":"UNDETECT", latStr, lonStr);
      }
    }
    else
      return;
  }
  else if (!strncmp(pArray[1], "SENSOR", 6))  // [SEN_ARD]SENSOR@ON OR [SEN_ARD]SENSOR@OFF
  {  
    if(!strncmp(pArray[2], "ON", 2))
      sensorOnFlag = true;
    else if(!strncmp(pArray[2], "OFF", 3))
      sensorOnFlag = false;

    sprintf(sendBuf, "[%s]%s@%s\n", pArray[0], pArray[1], pArray[2]);
  }
  else
    return;

  client.write(sendBuf, strlen(sendBuf));
  client.flush();
}


// 타이머 설정
void timerIsr()
{
  timerIsrFlag = true;
  secCount++;
}

// 와이파이 설정
void wifi_Setup() {
  wifiSerial.begin(38400);
  wifi_Init();
  server_Connect();
}
void wifi_Init()
{
  do {
    WiFi.init(&wifiSerial);
    if (WiFi.status() == WL_NO_SHIELD) {
    }
    else
      break;
  } while (1);

  while (WiFi.begin(AP_SSID, AP_PASS) != WL_CONNECTED) {
  }
  sprintf(lcdLine1, "ID:%s", LOGID);
  lcdDisplay(0, 0, lcdLine1);
  sprintf(lcdLine2, "%d.%d.%d.%d", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
  lcdDisplay(0, 1, lcdLine2);
}

int server_Connect()
{
  if (client.connect(SERVER_NAME, SERVER_PORT))
  {
    client.print("["LOGID":"PASSWD"]");
  }
}

// lcd 출력 함수
void lcdDisplay(int x, int y, char * str)
{
  int len = 16 - strlen(str);
  lcd.setCursor(x, y);
  lcd.print(str);
  for (int i = len; i > 0; i--)
    lcd.write(' ');
}
