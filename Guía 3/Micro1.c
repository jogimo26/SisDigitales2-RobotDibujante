//Código completo para el micro1 (robot)
// Proyecto en Wokwi: https://wokwi.com/projects/480857814500482049
#include <Arduino.h>
#include <string.h>
#include <ESP32Servo.h>
#include <soc/soc.h>
#include <soc/gpio_reg.h>

// COMUNICACION UART
#define UART_RX 26
#define UART_TX 25

// MOTORES
#define MI1 0
#define MI2 15
#define MD1 16
#define MD2 17

#define MOTOR_MASK ((1UL << MI1) | (1UL << MI2) | \
                    (1UL << MD1) | (1UL << MD2))

// SERVOMOTOR
#define SERVO_PIN 2
#define PLUMA_ARRIBA 0
#define PLUMA_ABAJO 90

Servo servoPluma;

// ESTADOS DEL ROBOT
enum {
  QUIETO,
  ADELANTE,
  ATRAS,
  DERECHA,
  IZQUIERDA,
  LEVANTAPLUMA,
  BAJAPLUMA
};

int comando = QUIETO;
int valor = 0;

bool moviendo = false;
bool plumaAbajo = false;

// TIEMPOS PROVISIONALES
const uint32_t MS_POR_CM = 100;
const uint32_t MS_POR_GRADO = 10;
const int MAX_VALOR = 9999;

// CADENA DEL COMANDO
char cad_comando[20] = {0};

// TEMPORIZADOR
hw_timer_t *timer = NULL;
volatile bool tiempoTerminado = false;


// CONTROL DE MOTORES

void controlMotores(int izquierdo, int derecho) {

  uint32_t salidas = 0;

  if (izquierdo == 1)
    salidas |= (1UL << MI1);
  else if (izquierdo == -1)
    salidas |= (1UL << MI2);

  if (derecho == 1)
    salidas |= (1UL << MD1);
  else if (derecho == -1)
    salidas |= (1UL << MD2);

  uint32_t actual = REG_READ(GPIO_OUT_REG);

  actual = (actual & ~MOTOR_MASK) | salidas;

  REG_WRITE(GPIO_OUT_REG, actual);
}


// INTERRUPCION DEL TIMER

void IRAM_ATTR interrupcionTimer() {
  tiempoTerminado = true;
}


// CONVERSION DE DISTANCIA Y ANGULO

uint32_t distancia_tiempo(int cm) {
  return (uint32_t)cm * MS_POR_CM;
}

uint32_t angulo_tiempo(int grados) {
  return (uint32_t)grados * MS_POR_GRADO;
}


// INTERPRETAR COMANDOS

void interpreta_comando(char* cadena) {

  comando = QUIETO;
  valor = 0;

  if (strcmp(cadena, "D") == 0) {

    comando = plumaAbajo ? LEVANTAPLUMA : BAJAPLUMA;
    return;
  }

  int inicio = 0;

  if (cadena[0] == 'A' && cadena[1] == 'A') {
    comando = ATRAS;
    inicio = 2;
  }
  else if (cadena[0] == 'A') {
    comando = ADELANTE;
    inicio = 1;
  }
  else if (cadena[0] == 'B') {
    comando = DERECHA;
    inicio = 1;
  }
  else if (cadena[0] == 'C') {
    comando = IZQUIERDA;
    inicio = 1;
  }
  else {
    Serial.println("Comando desconocido");
    return;
  }

  if (cadena[inicio] == '\0') {
    comando = QUIETO;
    Serial.println("Falta distancia o angulo");
    return;
  }

  for (int i = inicio; cadena[i] != '\0'; i++) {

    if (cadena[i] < '0' || cadena[i] > '9') {
      comando = QUIETO;
      Serial.println("Valor no numerico");
      return;
    }

    int digito = cadena[i] - '0';

    if (valor > (MAX_VALOR - digito) / 10) {
      comando = QUIETO;
      valor = 0;
      Serial.println("Valor demasiado grande");
      return;
    }

    valor = valor * 10 + digito;
  }

  if (valor == 0) {
    comando = QUIETO;
    Serial.println("El valor debe ser mayor que cero");
  }
}


// DETENER MOVIMIENTO

void detenerMovimiento(bool emergencia) {

  controlMotores(0, 0);

  if (timer != NULL)
    timerStop(timer);

  tiempoTerminado = false;

  moviendo = false;
  comando = QUIETO;
  valor = 0;

  if (emergencia)
    Serial.println("PARADA DE EMERGENCIA");
  else
    Serial.println("Movimiento terminado");

  // Avisar al micro2 que termino la accion
  Serial2.print("OK#");
}


// ACTIVAR TEMPORIZADOR

void iniciarTimer(uint32_t tiempoMs) {

  if (timer == NULL) {

    controlMotores(0, 0);
    moviendo = false;
    comando = QUIETO;

    Serial.println("Error: timer no disponible");
    return;
  }

  tiempoTerminado = false;

  timerStop(timer);
  timerWrite(timer, 0);

  timerAlarm(timer,
             (uint64_t)tiempoMs * 1000ULL,
             false, 0);

  timerStart(timer);
}


// EJECUTAR COMANDOS

void ejecutarComando() {

  uint32_t duracion = 0;

  switch (comando) {

    case ADELANTE:

      controlMotores(1, 1);
      duracion = distancia_tiempo(valor);
      Serial.println("Adelante");
      break;

    case ATRAS:

      controlMotores(-1, -1);
      duracion = distancia_tiempo(valor);
      Serial.println("Atras");
      break;

    case DERECHA:

      controlMotores(1, -1);
      duracion = angulo_tiempo(valor);
      Serial.println("Giro derecha");
      break;

    case IZQUIERDA:

      controlMotores(-1, 1);
      duracion = angulo_tiempo(valor);
      Serial.println("Giro izquierda");
      break;

    case BAJAPLUMA:

      servoPluma.write(PLUMA_ABAJO);
      plumaAbajo = true;
      comando = QUIETO;

      Serial.println("Pluma abajo");

      Serial2.print("OK#");
      return;

    case LEVANTAPLUMA:

      servoPluma.write(PLUMA_ARRIBA);
      plumaAbajo = false;
      comando = QUIETO;

      Serial.println("Pluma arriba");

      Serial2.print("OK#");
      return;

    default:
      return;
  }

  moviendo = true;

  Serial.print("Valor: ");
  Serial.println(valor);

  Serial.print("Duracion (ms): ");
  Serial.println(duracion);

  iniciarTimer(duracion);
}


// RECIBIR COMANDOS DESDE EL MICRO2

void leer_comando_teclado() {

  if (!Serial.available())
    return;

  String recibido = Serial.readStringUntil('#');

  recibido.trim();

  if (recibido.length() == 0)
    return;

  if (recibido == "*") {
    detenerMovimiento(true);
    return;
  }

  if (moviendo)
    return;

  recibido.toCharArray(cad_comando, sizeof(cad_comando));

  Serial.print("Comando recibido: ");
  Serial.println(cad_comando);

  interpreta_comando(cad_comando);

  if (comando != QUIETO)
    ejecutarComando();
}


// CONFIGURACION INICIAL

void setup() {

  Serial.begin(115200);

  // Comunicacion con el micro2
  Serial2.begin(115200, SERIAL_8N1, UART_RX, UART_TX);
  Serial2.setTimeout(100);

  // Motores
  pinMode(MI1, OUTPUT);
  pinMode(MI2, OUTPUT);
  pinMode(MD1, OUTPUT);
  pinMode(MD2, OUTPUT);

  controlMotores(0, 0);

  // Servomotor
  servoPluma.setPeriodHertz(50);
  servoPluma.attach(SERVO_PIN, 500, 2400);

  servoPluma.write(PLUMA_ARRIBA);
  plumaAbajo = false;

  // Temporizador a 1 MHz
  timer = timerBegin(1000000);

  if (timer != NULL) {
    timerAttachInterrupt(timer, &interrupcionTimer);
    timerStop(timer);
  }

  Serial.println("MICRO1 ROBOT INICIADO");
}


// CICLO PRINCIPAL

void loop() {

  // Recibir comandos del micro2
  leer_comando_teclado();

  // Comprobar finalizacion del movimiento
  if (tiempoTerminado) {

    noInterrupts();
    tiempoTerminado = false;
    interrupts();

    if (moviendo)
      detenerMovimiento(false);
  }

  delay(1);
}
