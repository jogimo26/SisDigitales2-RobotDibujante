// Codigo-guia#2

#include <Arduino.h>
#include <string.h>
#include <ESP32Servo.h>
#include <soc/soc.h>
#include <soc/gpio_reg.h>

// FILAS DEL TECLADO
#define ROW1 13
#define ROW2 12
#define ROW3 14
#define ROW4 27

// COLUMNAS DEL TECLADO
#define COL1 26
#define COL2 25
#define COL3 33
#define COL4 32

// MOTORES
#define MI1 0
#define MI2 15
#define MD1 16
#define MD2 17

// SERVOMOTOR
#define SERVO_PIN 2

// ANGULOS DE LA PLUMA
#define PLUMA_ARRIBA 0
#define PLUMA_ABAJO 90

Servo servoPluma;

// DISPLAY DE 7 SEGMENTOS
const int segmentos[7] = {
  23, 22, 21, 19, 18, 5, 4
};

// MASCARAS
#define ROW_MASK ((1UL << ROW1) | (1UL << ROW2) | \
                  (1UL << ROW3) | (1UL << ROW4))

#define MOTOR_MASK ((1UL << MI1) | (1UL << MI2) | \
                    (1UL << MD1) | (1UL << MD2))

// MATRIZ DEL TECLADO
const char matTeclas[4][4] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

// PATRONES DEL DISPLAY: A, B, C, D, E, F, G
const byte caracteres[16][7] = {
  {1,1,1,1,1,1,0}, // 0
  {0,1,1,0,0,0,0}, // 1
  {1,1,0,1,1,0,1}, // 2
  {1,1,1,1,0,0,1}, // 3
  {0,1,1,0,0,1,1}, // 4
  {1,0,1,1,0,1,1}, // 5
  {1,0,1,1,1,1,1}, // 6
  {1,1,1,0,0,0,0}, // 7
  {1,1,1,1,1,1,1}, // 8
  {1,1,1,1,0,1,1}, // 9
  {1,1,1,0,1,1,1}, // A
  {0,0,1,1,1,1,1}, // b
  {1,0,0,1,1,1,0}, // C
  {0,1,1,1,1,0,1}, // d
  {1,0,0,1,1,1,1}, // E
  {1,0,0,0,1,1,1}  // F
};

// PATRONES DE LAS FILAS
const uint32_t row_sel[4] = {
  ROW_MASK & ~(1UL << ROW1),
  ROW_MASK & ~(1UL << ROW2),
  ROW_MASK & ~(1UL << ROW3),
  ROW_MASK & ~(1UL << ROW4)
};

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
const uint32_t ANTIRREBOTE_MS = 25;

// COMANDO
char cad_comando[20] = {0};
int pos = 0;

// INTERRUPCIONES
volatile bool teclaPendiente = false;
volatile bool tiempoTerminado = false;

// TEMPORIZADOR
hw_timer_t *timer = NULL;


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


// INTERRUPCION DEL TECLADO

void IRAM_ATTR interrupcionTeclado() {
  teclaPendiente = true;
}


// INTERRUPCION DEL TIMER

void IRAM_ATTR interrupcionTimer() {
  tiempoTerminado = true;
}


// ACTIVAR INTERRUPCIONES DEL TECLADO

void activarInterrupciones() {

  attachInterrupt(digitalPinToInterrupt(COL1),
                  interrupcionTeclado, FALLING);

  attachInterrupt(digitalPinToInterrupt(COL2),
                  interrupcionTeclado, FALLING);

  attachInterrupt(digitalPinToInterrupt(COL3),
                  interrupcionTeclado, FALLING);

  attachInterrupt(digitalPinToInterrupt(COL4),
                  interrupcionTeclado, FALLING);
}


// DESACTIVAR INTERRUPCIONES DEL TECLADO

void desactivarInterrupciones() {

  detachInterrupt(digitalPinToInterrupt(COL1));
  detachInterrupt(digitalPinToInterrupt(COL2));
  detachInterrupt(digitalPinToInterrupt(COL3));
  detachInterrupt(digitalPinToInterrupt(COL4));
}


// LECTURA DEL TECLADO

char readKeyPad() {

  char tecla = 0;

  desactivarInterrupciones();

  for (int fila = 0; fila < 4; fila++) {

    uint32_t out = REG_READ(GPIO_OUT_REG);

    REG_WRITE(GPIO_OUT_REG,
              (out & ~ROW_MASK) | row_sel[fila]);

    delayMicroseconds(3);

    uint32_t port0 = REG_READ(GPIO_IN_REG);
    uint32_t port1 = REG_READ(GPIO_IN1_REG);

    uint8_t columnas =
      (((port0 >> COL1) & 1U) << 0) |
      (((port0 >> COL2) & 1U) << 1) |
      (((port1 >> (COL3 - 32)) & 1U) << 2) |
      (((port1 >> (COL4 - 32)) & 1U) << 3);

    if (columnas != 0x0F) {

      int col =
        ((columnas & 0x01) == 0) ? 0 :
        ((columnas & 0x02) == 0) ? 1 :
        ((columnas & 0x04) == 0) ? 2 : 3;

      tecla = matTeclas[fila][col];
      break;
    }
  }

  // Todas las filas en LOW
  uint32_t out = REG_READ(GPIO_OUT_REG);
  REG_WRITE(GPIO_OUT_REG, out & ~ROW_MASK);

  teclaPendiente = false;

  activarInterrupciones();

  return tecla;
}


// DISPLAY DE 7 SEGMENTOS

void printdisplay(char caracter) {

  int indice;

  if (caracter >= '0' && caracter <= '9') {

    indice = caracter - '0';
  }
  else if (caracter >= 'A' && caracter <= 'F') {

    indice = caracter - 'A' + 10;
  }
  else if (caracter >= 'a' && caracter <= 'f') {

    indice = caracter - 'a' + 10;
  }
  else {

    return;
  }

  for (int i = 0; i < 7; i++) {

    digitalWrite(segmentos[i],
                 caracteres[indice][i]);
  }
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

    comando = plumaAbajo ?
              LEVANTAPLUMA : BAJAPLUMA;

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


// ACTIVAR TEMPORIZADOR

void iniciarTimer(uint32_t tiempoMs) {

  if (timer == NULL) {

    Serial.println("Error: timer no disponible");

    controlMotores(0, 0);
    moviendo = false;
    comando = QUIETO;

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

    // BAJAR PLUMA
    case BAJAPLUMA:

      servoPluma.write(PLUMA_ABAJO);

      plumaAbajo = true;
      comando = QUIETO;

      Serial.println("Pluma ABAJO");
      return;

    // LEVANTAR PLUMA
    case LEVANTAPLUMA:

      servoPluma.write(PLUMA_ARRIBA);

      plumaAbajo = false;
      comando = QUIETO;

      Serial.println("Pluma ARRIBA");
      return;

    default:
      return;
  }

  moviendo = true;

  Serial.print("Valor: ");
  Serial.println(valor);

  Serial.print("Tiempo (ms): ");
  Serial.println(duracion);

  iniciarTimer(duracion);
}


// LEER COMANDO DEL TECLADO

void leer_comando_teclado(char key) {

  if (key == 0)
    return;

  Serial.print("Tecla: ");
  Serial.println(key);

  // Mostrar tecla en el display
  printdisplay(key);

  // PARADA DE EMERGENCIA
  if (key == '*') {

    pos = 0;
    cad_comando[0] = '\0';

    detenerMovimiento(true);
    return;
  }

  // Durante el movimiento no aceptar otros comandos
  if (moviendo)
    return;

  // ENTER: interpretar comando
  if (key == '#') {

    cad_comando[pos] = '\0';

    if (pos > 0) {

      Serial.print("Comando recibido: ");
      Serial.println(cad_comando);

      interpreta_comando(cad_comando);

      if (comando != QUIETO)
        ejecutarComando();
    }
    else {

      Serial.println("Comando vacio");
    }

    pos = 0;
    cad_comando[0] = '\0';

    return;
  }

  // Guardar tecla en la cadena
  if (pos < sizeof(cad_comando) - 1) {

    cad_comando[pos] = key;
    pos++;

    cad_comando[pos] = '\0';
  }
  else {

    pos = 0;
    cad_comando[0] = '\0';

    Serial.println("Comando demasiado largo");
  }
}


// CONFIGURACION INICIAL

void setup() {

  Serial.begin(115200);

  // Filas del teclado
  pinMode(ROW1, OUTPUT);
  pinMode(ROW2, OUTPUT);
  pinMode(ROW3, OUTPUT);
  pinMode(ROW4, OUTPUT);

  // Columnas del teclado
  pinMode(COL1, INPUT_PULLUP);
  pinMode(COL2, INPUT_PULLUP);
  pinMode(COL3, INPUT_PULLUP);
  pinMode(COL4, INPUT_PULLUP);

  // Motores
  pinMode(MI1, OUTPUT);
  pinMode(MI2, OUTPUT);
  pinMode(MD1, OUTPUT);
  pinMode(MD2, OUTPUT);

  // Display
  for (int i = 0; i < 7; i++) {

    pinMode(segmentos[i], OUTPUT);
    digitalWrite(segmentos[i], LOW);
  }

  printdisplay('6');

  // Configurar servomotor
  servoPluma.setPeriodHertz(50);
  servoPluma.attach(SERVO_PIN, 500, 2400);

  // Posicion inicial: pluma arriba
  servoPluma.write(PLUMA_ARRIBA);
  plumaAbajo = false;

  // Motores apagados
  controlMotores(0, 0);

  // Todas las filas en LOW
  uint32_t out = REG_READ(GPIO_OUT_REG);
  REG_WRITE(GPIO_OUT_REG, out & ~ROW_MASK);

  // Configurar TIMER a 1 MHz
  timer = timerBegin(1000000);

  if (timer != NULL) {

    timerAttachInterrupt(timer, &interrupcionTimer);
    timerStop(timer);
  }

  // Interrupciones del teclado
  activarInterrupciones();

  Serial.println("Sistema iniciado");

  Serial.println("A15# = Adelante");
  Serial.println("AA20# = Atras");
  Serial.println("B90# = Giro derecha");
  Serial.println("C45# = Giro izquierda");
  Serial.println("D# = Subir/Bajar pluma");
  Serial.println("* = Parar");
}


// CICLO PRINCIPAL

void loop() {

  // Detectar pulsacion del teclado
  if (teclaPendiente) {

    noInterrupts();
    teclaPendiente = false;
    interrupts();

    delay(ANTIRREBOTE_MS);

    char key = readKeyPad();

    leer_comando_teclado(key);
  }

  // Revisar si termino el movimiento
  if (tiempoTerminado) {

    noInterrupts();
    tiempoTerminado = false;
    interrupts();

    if (moviendo)
      detenerMovimiento(false);
  }

  delay(1);
}
