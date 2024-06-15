/*
프로젝트 : 산불 감지 시스템
코드 : 아두이노 액추에이터부
*/

#include <WiFiEsp.h>
#include <SoftwareSerial.h>
#include <TimerOne.h>

// 상태를 확인하기 위한 LCD
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// 수정 예정 -> macro + pin배치
#define AP_SSID "embC"
#define AP_PASS "embC1234"
#define SERVER_NAME "10.10.14.59"
#define SERVER_PORT 5000
#define LOGID "ACT_ARD"
#define PASSWD "PASSWD"

#define LED_PIN 2
#define BUZZER_PIN 3
#define WIFIRX 6  //6:RX-->ESP8266 TX
#define WIFITX 7  //7:TX -->ESP8266 RX
// 펌프 모터 핀 설정
#define ENB 9
#define IN3 10
#define IN4 11

#define CMD_SIZE 50
#define ARR_CNT 5

// flag 변수 생성
bool timerIsrFlag = false;
bool startFlag = false;
bool stopFlag = false;
bool ledFlag = false;
bool buzzerFlag = false;
bool led_light = false;

// 서버에 명령어를 전송할 버퍼
char sendBuf[CMD_SIZE];

// 확인용 lcd
char lcdLine1[17] = "ACTUATOR ON";
char lcdLine2[17] = "WiFi Connecting!";

unsigned int secCount;

// 와이파이 설정
SoftwareSerial wifiSerial(WIFIRX, WIFITX);
WiFiEspClient client;

// 디버깅용 LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

// BUZZER 주파수
int frequencies[] = {2000, 2100};
int buzzerSIZE = sizeof(frequencies) / sizeof(frequencies[0]);

void setup() {
  // 확인용 LCD
  lcd.init();
  lcd.backlight();
  lcdDisplay(0, 0, lcdLine1);
  lcdDisplay(0, 1, lcdLine2);

  // 부저, LED, 펌프 모터 출력 설정
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  wifi_Setup();

  // 타이머 설정
  Timer1.initialize(1000000);
  Timer1.attachInterrupt(timerIsr); // 타이머 인터럽트 함수가 1초 간격으로 실행됨
}

void loop() {
  if (client.available()) {
    socketEvent();
  }

  if (timerIsrFlag) //0.5초에 한번씩 실행
  {
    timerIsrFlag = false;

    if(startFlag)
    { 
      // 펌프 모터 작동
      digitalWrite(IN3, HIGH);
      digitalWrite(IN4, LOW);
      analogWrite(ENB, 255); // 속도 제어 코드

      // LED, BUZZER 동작 FLAG ON
      ledFlag = true;
      buzzerFlag = true;

      // 한번만 실행되도록 함
      startFlag = false;
    }

    if(stopFlag)
    {
      // 펌프 모터 정지
      digitalWrite(IN3, LOW);
      digitalWrite(IN4, LOW);
      analogWrite(ENB, 0);

      // LED, BUZZER FLAG OFF
      ledFlag = false;
      buzzerFlag = false;

      // led off
      led_light = false;
      digitalWrite(LED_PIN, LOW);

      // buzzer off
      noTone(BUZZER_PIN);

      // 한번만 실행되도록 한다.
      stopFlag = false;
    }

    // LED 동작
    if(ledFlag)
    {
      if(led_light)
      {
        led_light = false;
        digitalWrite(LED_PIN, LOW);
      }
      else
      {
        led_light = true;
        digitalWrite(LED_PIN, HIGH);
      }
    }
    // BUZZER 동작
    if(buzzerFlag)
      tone(BUZZER_PIN, frequencies[secCount % buzzerSIZE]);
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

  // 명령어 파싱
  pToken = strtok(recvBuf, "[@]");
  while (pToken != NULL)
  {
    pArray[i] =  pToken;
    if (++i >= ARR_CNT)
      break;
    pToken = strtok(NULL, "[@]");
  }

  // 명령어가 [ 와 ], @로 나눠져서 분리된다. 
  if ((strlen(pArray[1]) + strlen(pArray[2])) < 16)
  {
    // 받아온 명령어를 LCD에 출력함
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
  else if (!strncmp(pArray[1], "ACTUATOR", 8))  // [ACT_ARD]ACTUATOR@??? 동작 on/off 명령어 
  { 
    if(!strncmp(pArray[2], "ON", 2))
    {
      startFlag = true;
      sprintf(sendBuf, "[%s]%s@%s\n", pArray[0], pArray[1], pArray[2]);
    }
    else if(!strncmp(pArray[2], "OFF", 3))
    {
      stopFlag = true;
      sprintf(sendBuf, "[%s]%s@%s\n", pArray[0], pArray[1], pArray[2]);
    }
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

// 와이파이 초기화
void wifi_Init()
{
  // 와이파이 쉴드가 초기화 될 때까지 반복
  do {
    WiFi.init(&wifiSerial);

    // 와이파이 쉴드가 연결되어 있는 경우 루프를 탈출
    if (WiFi.status() != WL_NO_SHIELD)
      break;
  } while (1);

  // 와이파이가 연결될 때까지 반복
  while (WiFi.begin(AP_SSID, AP_PASS) != WL_CONNECTED);

  sprintf(lcdLine1, "ID:%s", LOGID);
  lcdDisplay(0, 0, lcdLine1);
  sprintf(lcdLine2, "%d.%d.%d.%d", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
  lcdDisplay(0, 1, lcdLine2);
}

int server_Connect()
{
  // 서버에 연결하기 위한 아이디와 패스워드 입력
  if (client.connect(SERVER_NAME, SERVER_PORT)) {
    client.print("["LOGID":"PASSWD"]");
  }
}

// 와이파이 상태
void printWifiStatus()
{
  // WiFi 쉴드로부터 할당된 IP주소를 저장함
  IPAddress ip = WiFi.localIP();

  // 쉴드의 RSSI값을 저장함.(무선 신호의 수신강도를 저장)
  long rssi = WiFi.RSSI();
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