
#include <Arduino.h>
#include <string.h>

// REGISTROS GPIO DEL ESP32
#define GPIO_OUT_REG        (*(volatile uint32_t*)0x3FF44004)
#define GPIO_OUT_W1TS_REG   (*(volatile uint32_t*)0x3FF44008)
#define GPIO_OUT_W1TC_REG   (*(volatile uint32_t*)0x3FF4400C)
#define GPIO_IN_REG         (*(volatile uint32_t*)0x3FF4403C)
#define GPIO_IN1_REG        (*(volatile uint32_t*)0x3FF44040)

// Pines de las filas
#define ROW1 13
#define ROW2 12
#define ROW3 14
#define ROW4 27

// Pines de las columnas
#define COL1 26
#define COL2 25
#define COL3 33
#define COL4 32

// Pines de los motores
#define MI1 0
#define MI2 15
#define MD1 16
#define MD2 17

// Mascara de las filas
#define ROW_MASK ((1UL << ROW1) | (1UL << ROW2) | \
                  (1UL << ROW3) | (1UL << ROW4))

// Mascara de los motores
#define MOTOR_MASK ((1UL << MI1) | (1UL << MI2) | \
                    (1UL << MD1) | (1UL << MD2))

// Patrones para seleccionar cada fila
const uint32_t row_sel[4] = {
  ROW_MASK & ~(1UL << ROW1),
  ROW_MASK & ~(1UL << ROW2),
  ROW_MASK & ~(1UL << ROW3),
  ROW_MASK & ~(1UL << ROW4)
};

// Matriz de caracteres
const char matTeclas[4][4] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

// Estados del robot
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

// Variables del temporizador
uint32_t inicioMovimiento = 0;
uint32_t duracionMovimiento = 0;

// Valores provisionales para simulacion
const uint32_t MS_POR_CM = 100;
const uint32_t MS_POR_GRADO = 10;

const uint32_t ANTIRREBOTE_MS = 25;
const int MAX_VALOR = 9999;

// Almacenamiento del comando
char cad_comando[20] = {0};
size_t pos = 0;

// Variables de deteccion de teclas
char lecturaAnterior = 0;
char teclaEstable = 0;
uint32_t tiempoCambio = 0;


// CONTROL DE MOTORES MEDIANTE REGISTROS

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

  // Apagar primero todas las salidas
  GPIO_OUT_W1TC_REG = MOTOR_MASK;

  // Activar las salidas necesarias
  if (salidas != 0)
    GPIO_OUT_W1TS_REG = salidas;
}



// LECTURA DEL TECLADO

char readKeyPad() {

  char tecla = 0;

  for (int fila = 0; fila < 4; fila++) {

    uint32_t out = GPIO_OUT_REG;

    GPIO_OUT_REG =
      (out & ~ROW_MASK) | row_sel[fila];

    delayMicroseconds(3);

    uint32_t port0 = GPIO_IN_REG;
    uint32_t port1 = GPIO_IN1_REG;

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

  GPIO_OUT_W1TS_REG = ROW_MASK;

  return tecla;
}


// ANTIRREBOTE DEL TECLADO

char leerTeclaNueva() {

  char actual = readKeyPad();

  if (actual != lecturaAnterior) {

    lecturaAnterior = actual;
    tiempoCambio = millis();
  }

  if ((uint32_t)(millis() - tiempoCambio)
        >= ANTIRREBOTE_MS &&
      actual != teclaEstable) {

    teclaEstable = actual;

    if (teclaEstable != 0)
      return teclaEstable;
  }

  return 0;
}


// DISPLAY

void printdisplay(char caracter) {

  (void)caracter;

  // Integrar el driver real del display.
  // Se conserva pendiente de conocer
  // la asignacion de sus segmentos.
}


// CONVERSION DE DISTANCIA Y ANGULO


uint32_t distancia_tiempo(int cm) {

  return (uint32_t)cm * MS_POR_CM;
}

uint32_t angulo_tiempo(int grados) {

  return (uint32_t)grados * MS_POR_GRADO;
}


// INTERPRETACION DE COMANDOS

void interpreta_comando(char* cadena) {

  comando = QUIETO;
  valor = 0;

  // Control logico de pluma
  if (strcmp(cadena, "D") == 0) {

    comando = plumaAbajo ?
              LEVANTAPLUMA : BAJAPLUMA;

    return;
  }

  size_t inicioNumero = 0;

  if (cadena[0] == 'A' &&
      cadena[1] == 'A') {

    comando = ATRAS;
    inicioNumero = 2;
  }
  else if (cadena[0] == 'A') {

    comando = ADELANTE;
    inicioNumero = 1;
  }
  else if (cadena[0] == 'B') {

    comando = DERECHA;
    inicioNumero = 1;
  }
  else if (cadena[0] == 'C') {

    comando = IZQUIERDA;
    inicioNumero = 1;
  }
  else {

    Serial.println("Error: comando desconocido");
    return;
  }

  if (cadena[inicioNumero] == '\0') {

    comando = QUIETO;
    Serial.println("Error: falta distancia o angulo");
    return;
  }

  int numero = 0;

  for (size_t i = inicioNumero;
       cadena[i] != '\0'; i++) {

    if (cadena[i] < '0' ||
        cadena[i] > '9') {

      comando = QUIETO;
      Serial.println("Error: valor no numerico");
      return;
    }

    int digito = cadena[i] - '0';

    if (numero > (MAX_VALOR - digito) / 10) {

      comando = QUIETO;
      Serial.println("Error: numero demasiado grande");
      return;
    }

    numero = numero * 10 + digito;
  }

  if (numero == 0) {

    comando = QUIETO;
    Serial.println("Error: valor debe ser mayor que cero");
    return;
  }

  valor = numero;
}


// DETENCION DEL ROBOT

void detenerMovimiento(bool emergencia) {

  controlMotores(0, 0);

  moviendo = false;
  comando = QUIETO;
  valor = 0;

  if (emergencia) {

    Serial.println("PARADA INMEDIATA (*)");
  }
  else {

    Serial.println("Movimiento terminado. Motores detenidos.");
  }
}


// EJECUCION DE COMANDOS

void ejecutarComando() {

  switch (comando) {

    case ADELANTE:

      controlMotores(1, 1);
      duracionMovimiento = distancia_tiempo(valor);

      Serial.println("Adelante");
      break;

    case ATRAS:

      controlMotores(-1, -1);
      duracionMovimiento = distancia_tiempo(valor);

      Serial.println("Atras");
      break;

    case DERECHA:

      controlMotores(1, -1);
      duracionMovimiento = angulo_tiempo(valor);

      Serial.println("Giro derecha");
      break;

    case IZQUIERDA:

      controlMotores(-1, 1);
      duracionMovimiento = angulo_tiempo(valor);

      Serial.println("Giro izquierda");
      break;

    case BAJAPLUMA:

      plumaAbajo = true;

      Serial.println("Pluma ABAJO (estado logico)");
      comando = QUIETO;
      return;

    case LEVANTAPLUMA:

      plumaAbajo = false;

      Serial.println("Pluma ARRIBA (estado logico)");
      comando = QUIETO;
      return;

    default:
      return;
  }

  inicioMovimiento = millis();
  moviendo = true;

  Serial.print("Valor: ");
  Serial.println(valor);

  Serial.print("Duracion simulada (ms): ");
  Serial.println(duracionMovimiento);
}


// LECTURA DE COMANDOS COMPLETOS

void leer_comando_teclado(char key) {

  if (key == 0)
    return;

  Serial.print("Tecla: ");
  Serial.println(key);

  printdisplay(key);

  // Parada de emergencia sin esperar #
  if (key == '*') {

    pos = 0;
    cad_comando[0] = '\0';

    detenerMovimiento(true);
    return;
  }

  // Durante el movimiento solo se acepta *
  if (moviendo)
    return;

  // Fin del comando
  if (key == '#') {

    cad_comando[pos] = '\0';

    if (pos == 0) {

      Serial.println("Error: comando vacio");
    }
    else {

      Serial.print("Comando recibido: ");
      Serial.println(cad_comando);

      interpreta_comando(cad_comando);

      if (comando != QUIETO)
        ejecutarComando();
    }

    pos = 0;
    cad_comando[0] = '\0';

    return;
  }

  // Proteccion contra desbordamiento
  if (pos >= sizeof(cad_comando) - 1) {

    pos = 0;
    cad_comando[0] = '\0';

    Serial.println("Error: comando demasiado largo");
    return;
  }

  cad_comando[pos++] = key;
  cad_comando[pos] = '\0';
}


// CONFIGURACION INICIAL

void setup() {

  Serial.begin(115200);

  pinMode(ROW1, OUTPUT);
  pinMode(ROW2, OUTPUT);
  pinMode(ROW3, OUTPUT);
  pinMode(ROW4, OUTPUT);

  pinMode(COL1, INPUT_PULLUP);
  pinMode(COL2, INPUT_PULLUP);
  pinMode(COL3, INPUT_PULLUP);
  pinMode(COL4, INPUT_PULLUP);

  pinMode(MI1, OUTPUT);
  pinMode(MI2, OUTPUT);
  pinMode(MD1, OUTPUT);
  pinMode(MD2, OUTPUT);

  GPIO_OUT_W1TS_REG = ROW_MASK;

  controlMotores(0, 0);

  Serial.println("Entregable 2 iniciado");
  Serial.println("Ejemplos: A15#, AA20#, B90#, C45#, D#");
}


// CICLO PRINCIPAL NO BLOQUEANTE

void loop() {

  char key = leerTeclaNueva();

  leer_comando_teclado(key);

  // Revisar si termino el movimiento
  if (moviendo &&
      (uint32_t)(millis() - inicioMovimiento)
        >= duracionMovimiento) {

    detenerMovimiento(false);
  }

  // Permite que otras tareas del ESP32 se ejecuten
  delay(1);
}
