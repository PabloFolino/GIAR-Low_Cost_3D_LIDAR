/* //#include<Arduino.h>
#include <TMCStepper.h>

// Configura tus pines
#define PIN               2
#define EN_PIN_Nema17     11        // Pin de Enable (activo en bajo)
#define DIR_PIN_Nema17    12        // Pin de dirección
#define STEP_PIN_Nema17   13        // Pin de paso
#define SERIAL_PORT_Nema17 1        // Puerto UART del ESP32 (puede ser Serial1 o Serial2)
#define RX_PIN_Nema17      9        // RX del ESP32, es el Rx del Serial 1
#define TX_PIN_Nema17     10        // TX del ESP32, es el Tx del Serial 1

#define R_SENSE 0.11f   // Valor típico para la resistencia sense del TMC2208



void setup() {
  pinMode(PIN, OUTPUT);
  pinMode(14, OUTPUT);
  pinMode(STEP_PIN_Nema17, OUTPUT);
  pinMode(DIR_PIN_Nema17, OUTPUT);
  pinMode(EN_PIN_Nema17, OUTPUT);
  digitalWrite(EN_PIN_Nema17, LOW);  // Habilita el driver

  // HardwareSerial mySerial_Nema17(SERIAL_PORT_Nema17);  // Usa UART1
  // //mySerial_Nema17.begin((unsigned long) 115200,SERIAL_8N1,(int) RX_PIN_Nema17,(int) TX_PIN_Nema17);
  // TMC2208Stepper driverNema17(&mySerial_Nema17, R_SENSE);  // Inicializa el driver

  // driverNema17.begin();               // Inicializa el driver
  // driverNema17.toff(5);               // Habilita el regulador interno
  // driverNema17.rms_current(600);      // Establece la corriente del motor en mA
  // driverNema17.microsteps(16);        // Configura microstepping
  // driverNema17.en_spreadCycle(false); // Usa StealthChop para un movimiento silencioso
  // driverNema17.pwm_autoscale(true);   // Ajuste automático de la PWM
}

void loop() {
  // digitalWrite(PIN, HIGH);
  // delay(11000);
  // digitalWrite(PIN, LOW);
  // delay(2000);
  digitalWrite(PIN, HIGH);
  digitalWrite(DIR_PIN_Nema17, LOW);
  digitalWrite(STEP_PIN_Nema17, HIGH);

/*   digitalWrite(DIR_PIN_Nema17, HIGH);
  for (int i = 0; i < 200; i++) {
    digitalWrite(STEP_PIN_Nema17, HIGH);
    delayMicroseconds(500);
    digitalWrite(STEP_PIN_Nema17, LOW);
    delayMicroseconds(500);
  }
  delay(800);

  digitalWrite(DIR_PIN_Nema17, LOW);
  for (int i = 0; i < 200; i++) {
    digitalWrite(STEP_PIN_Nema17, HIGH);
    delayMicroseconds(500);
    digitalWrite(STEP_PIN_Nema17, LOW);
    delayMicroseconds(500);
  }
  delay(1000); 
} */
