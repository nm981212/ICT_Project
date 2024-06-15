#define digitalPin 2

// 디지털 핀 번호 설정
const int sensorPin = 2; // KY-026 센서 모듈이 연결된 디지털 핀 번호

void setup() {
  // 시리얼 통신 시작
  Serial.begin(9600);
  // 센서 핀을 입력으로 설정
  pinMode(sensorPin, INPUT);
}

void loop() {
  // 센서에서 값을 읽음
  int sensorValue = digitalRead(sensorPin);

  // 센서 값이 HIGH(1)이면 화염이 감지됨
  if (sensorValue == HIGH) {
    Serial.println("화염이 감지되었습니다!"); // 시리얼 모니터에 메시지 출력
  }
  delay(1000); // 1초 대기
}
