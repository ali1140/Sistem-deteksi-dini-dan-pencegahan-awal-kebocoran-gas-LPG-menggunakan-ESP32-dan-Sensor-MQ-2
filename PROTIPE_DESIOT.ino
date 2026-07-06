#include <WiFi.h>
#include <PubSubClient.h>

// Konfigurasi WiFi
const char* ssid = "Your_WiFi_SSID";
const char* password = "Your_WiFi_Password";

// Konfigurasi MQTT
const char* mqtt_server = "192.168.1.100"; // IP broker Mosquitto
const char* topic_sensor1 = "sensor/mq2_1";
const char* topic_sensor2 = "sensor/mq2_2";
const char* topic_motor_speed1 = "motor1/speed";
const char* topic_motor_speed2 = "motor2/speed";
const char* topic_alert = "alert";
const char* topic_manual_mode = "manual/mode";
const char* topic_manual_speed1 = "manual/speed1";
const char* topic_manual_speed2 = "manual/speed2";

WiFiClient espClient;
PubSubClient client(espClient);

// Pin Sensor dan Motor
#define MQ_sensor1 35
#define MQ_sensor2 34
const int pwmPin1 = 14;
const int pwmPin2 = 32;
const int in1_motor1 = 27;
const int in2_motor1 = 26;
const int in1_motor2 = 25;
const int in2_motor2 = 33;

// Konstanta
const int pwmAwal = 100;
const int pwmTinggi = 200;
const int threshold = 1200;

// Variabel
int pwmMotor1 = pwmAwal;
int pwmMotor2 = pwmAwal;
bool manualMode = false;

// Fungsi Koneksi WiFi
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Menghubungkan ke WiFi: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi Terhubung");
  Serial.print("Alamat IP: ");
  Serial.println(WiFi.localIP());
}

// Fungsi Koneksi MQTT
void reconnect() {
  while (!client.connected()) {
    Serial.print("Menghubungkan ke MQTT...");
    if (client.connect("ESP32")) {
      Serial.println("Terhubung");
      client.subscribe(topic_manual_mode);
      client.subscribe(topic_manual_speed1);
      client.subscribe(topic_manual_speed2);
    } else {
      Serial.print("Gagal, rc=");
      Serial.print(client.state());
      Serial.println(" Coba lagi dalam 5 detik...");
      delay(5000);
    }
  }
}

// Fungsi Callback MQTT
void callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  if (String(topic) == topic_manual_mode) {
    manualMode = message.toInt();
  } else if (String(topic) == topic_manual_speed1) {
    pwmMotor1 = message.toInt();
  } else if (String(topic) == topic_manual_speed2) {
    pwmMotor2 = message.toInt();
  }
}

// Fungsi Utama
void setup() {
  Serial.begin(115200);

  // Inisialisasi Pin
  pinMode(MQ_sensor1, INPUT);
  pinMode(MQ_sensor2, INPUT);
  pinMode(pwmPin1, OUTPUT);
  pinMode(pwmPin2, OUTPUT);
  pinMode(in1_motor1, OUTPUT);
  pinMode(in2_motor1, OUTPUT);
  pinMode(in1_motor2, OUTPUT);
  pinMode(in2_motor2, OUTPUT);

  digitalWrite(in1_motor1, HIGH);
  digitalWrite(in2_motor1, LOW);
  digitalWrite(in1_motor2, HIGH);
  digitalWrite(in2_motor2, LOW);

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Membaca nilai sensor
  int sensorValue1 = analogRead(MQ_sensor1);
  int sensorValue2 = analogRead(MQ_sensor2);

  String alert = "AMAN";

  if (!manualMode) {
    pwmMotor1 = (sensorValue1 > threshold) ? pwmTinggi : pwmAwal;
    pwmMotor2 = (sensorValue2 > threshold) ? pwmTinggi : pwmAwal;

    if (sensorValue1 > threshold || sensorValue2 > threshold) {
      alert = "BAHAYA";
    }
  }

  // Mengirim data ke broker
  client.publish(topic_sensor1, String(sensorValue1).c_str());
  client.publish(topic_sensor2, String(sensorValue2).c_str());
  client.publish(topic_motor_speed1, String(pwmMotor1).c_str());
  client.publish(topic_motor_speed2, String(pwmMotor2).c_str());
  client.publish(topic_alert, alert.c_str());

  // Mengatur PWM motor
  analogWrite(pwmPin1, pwmMotor1);
  analogWrite(pwmPin2, pwmMotor2);

  delay(1000);
}