#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);

// === Servo setup =======
#define SERVO_COUNT 5
int potPin[SERVO_COUNT] = {A4, A3, A2, A1, A0};

// MG90S
#define SERVOMIN 100
#define SERVOMAX 500
// HV23MG (servo ke-4 = index 3) 
#define SERVOMIN_HV23 100
#define SERVOMAX_HV23 500

// === Button ===
const int buttonPin = 2;
bool playmode = false;

// === Record buffer ===
#define MAX_STEP 50
int positions[MAX_STEP][SERVO_COUNT];
int arrayMax = 0;
int arrayStep = 0;

// === Servo movement ===
int ist[SERVO_COUNT];   // posisi servo sekarang
int sol[SERVO_COUNT];   // posisi target

// =============================================================
void setup() {
  Serial.begin(9600);
  pwm.begin();
  pwm.setPWMFreq(60); // frekuensi servo

  pinMode(buttonPin, INPUT_PULLUP);

  // inisialisasi servo ke 90 derajat
  for (int i = 0; i < SERVO_COUNT; i++) {
    ist[i] = 90;
    if(i == 3) pwm.setPWM(i, 0, angleToPulseHV23(ist[i]));
    else pwm.setPWM(i, 0, angleToPulse(ist[i]));
  }

  Serial.println("System ready:");
  Serial.println("Short press = Record snapshot");
  Serial.println("Long press  = Toggle Play mode");
}

// =============================================================
void loop() {
  unsigned long now = millis();
  checkButton(now);

  if (!playmode) {
    // ===== Manual Mode =====
    for (int i = 0; i < SERVO_COUNT; i++) {
      int val = analogRead(potPin[i]);
      ist[i] = map(val, 0, 1023, 0, 180);
      if(i == 3) pwm.setPWM(i, 0, angleToPulseHV23(ist[i])); // HV23MG
      else pwm.setPWM(i, 0, angleToPulse(ist[i]));           // MG90S
    }
  } else {
    // ===== Play Mode =====
    play_servo();
    delay(20); // delay 20ms untuk gerakan halus
  }
}

void checkButton(unsigned long now) {
  static bool lastState = HIGH;
  static unsigned long pressedAt = 0;

  bool reading = digitalRead(buttonPin);

  if (lastState == HIGH && reading == LOW) pressedAt = now;

  if (lastState == LOW && reading == HIGH) {
    unsigned long pressDuration = now - pressedAt;

    if (pressDuration < 600) recordSnapshot();
    else {
      playmode = !playmode;
      if(playmode){
        Serial.println("Play Mode ON");
        arrayStep = 0;
        prepareStep();
      } else {
        Serial.println("Play Mode OFF (Manual)");
      }
    }
  }
  lastState = reading;
}

void recordSnapshot() {
  if(arrayMax < MAX_STEP){
    for(int i=0; i<SERVO_COUNT; i++) positions[arrayMax][i] = ist[i];
    Serial.print("Snapshot saved step ");
    Serial.println(arrayMax);
    arrayMax++;
  } else Serial.println("Buffer full!");
}

void prepareStep() {
  if(arrayMax == 0) return;

  for(int i=0; i<SERVO_COUNT; i++) sol[i] = positions[arrayStep][i];
}

void play_servo() {
  bool moving = false;

  for(int i=0; i<SERVO_COUNT; i++){
    if(ist[i] < sol[i]) { ist[i]++; moving = true; }
    else if(ist[i] > sol[i]) { ist[i]--; moving = true; }

    if(i == 3) pwm.setPWM(i, 0, angleToPulseHV23(ist[i])); // HV23MG
    else pwm.setPWM(i, 0, angleToPulse(ist[i]));           // MG90S
  }

  if(!moving){
    arrayStep++;
    if(arrayStep >= arrayMax) arrayStep = 0;
    prepareStep();
  }
}

int angleToPulse(int ang){
  if(ang < 0) ang = 0;
  if(ang > 180) ang = 180;
  return map(ang, 0, 180, SERVOMIN, SERVOMAX);
}

int angleToPulseHV23(int ang){
  if(ang < 0) ang = 0;
  if(ang > 180) ang = 180;
  return map(ang, 0, 180, SERVOMIN_HV23, SERVOMAX_HV23);
}
