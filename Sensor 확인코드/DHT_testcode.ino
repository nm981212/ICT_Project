#include <DHT.h>

// 센서 유형 선택
#define DHT_TYPE DHT11   // DHT11을 사용하는 경우 DHT11로 변경

// 센서 핀 설정
#define DHT_PIN 4       // 센서가 연결된 디지털 핀 번호

// DHT 객체 생성
DHT dht(DHT_PIN, DHT_TYPE);

void setup() {
  // 시리얼 통신 시작
  Serial.begin(9600);
  // DHT 센서 시작
  dht.begin();
}

void loop() {
  // 온도 및 습도 값을 읽기
  float temperature = dht.readTemperature(); // 온도(섭씨)
  float humidity = dht.readHumidity();       // 습도(%)

  // 읽은 값이 올바른지 확인
  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("센서에서 값을 읽을 수 없습니다!");
    return;
  }

  // 온습도 값을 시리얼 모니터에 출력
  Serial.print("온도: ");
  Serial.print(temperature);
  Serial.print("°C, 습도: ");
  Serial.print(humidity);
  Serial.println("%");

  // 측정 주기 (예: 1초마다)
  delay(1000);
}
