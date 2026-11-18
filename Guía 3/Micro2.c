//Código completo para el micro2 (mando)
// Proyecto en Wokwi: https://wokwi.com/projects/480860271395859457
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// UART
#define UART_RX 16
#define UART_TX 17

// TECLADO
#define ROW1 13
#define ROW2 12
#define ROW3 14
#define ROW4 27

#define COL1 26
#define COL2 25
#define COL3 33
#define COL4 32

// OLED
#define ANCHO 128
#define ALTO 64
#define OLED_RESET -1

Adafruit_SSD1306 display(ANCHO, ALTO, &Wire, OLED_RESET);

// MATRIZ DEL TECLADO
const char matTeclas[4][4] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

const int filas[4] = {ROW1, ROW2, ROW3, ROW4};
const int columnas[4] = {COL1, COL2, COL3, COL4};

// ESTADOS DE LA INTERFAZ
enum Estado {
  MENU_INICIAL,
  MODO_INTERACTIVO,
  MODO_PROGRAMA
};

Estado pantallaActual = MENU_INICIAL;

// ANTIRREBOTE
char lecturaAnterior = 0;
char teclaEstable = 0;

uint32_t tiempoCambio = 0;
const uint32_t ANTIRREBOTE_MS = 30;

// COMANDOS
char cad_comando[20] = {0};
int pos = 0;

bool esperandoRespuesta = false;
bool mostrarOK = false;

// ANIMACION
int cantidadPuntos = 0;

const int MAX_PUNTOS = 20;

uint32_t tiempoAnimacion = 0;
const uint32_t INTERVALO_ANIMACION = 120;

// PROGRAMAS
// Cada lado mide 10 cm.
// D baja la pluma al iniciar y la levanta al terminar.

const char* const programaCuadrado[] = {
  "D",
  "A10", "B90",
  "A10", "B90",
  "A10", "B90",
  "A10", "B90",
  "D"
};

const char* const programaTriangulo[] = {
  "D",
  "A10", "B120",
  "A10", "B120",
  "A10", "B120",
  "D"
};

const char* const* programaActual = nullptr;

int cantidadComandosPrograma = 0;
int indiceComandoPrograma = 0;

bool ejecutandoPrograma = false;


// PROTOTIPOS
void mostrarMenuInicial();
void mostrarModoInteractivo();
void mostrarModoPrograma();
void mostrarError(const char* linea1, const char* linea2);
void enviarSiguienteComando();


// ========================================
// LEER TECLADO
// ========================================

char readKeyPad() {

  char tecla = 0;

  for (int fila = 0; fila < 4; fila++) {

    for (int i = 0; i < 4; i++)
      digitalWrite(filas[i], HIGH);

    digitalWrite(filas[fila], LOW);

    delayMicroseconds(100);

    for (int col = 0; col < 4; col++) {

      if (digitalRead(columnas[col]) == LOW) {

        tecla = matTeclas[fila][col];
        return tecla;
      }
    }
  }

  return tecla;
}


// ========================================
// ANTIRREBOTE
// ========================================

char leerTeclaNueva() {

  char actual = readKeyPad();

  if (actual != lecturaAnterior) {

    lecturaAnterior = actual;
    tiempoCambio = millis();
  }

  if (millis() - tiempoCambio >= ANTIRREBOTE_MS &&
      actual != teclaEstable) {

    teclaEstable = actual;

    if (teclaEstable != 0)
      return teclaEstable;
  }

  return 0;
}


// ========================================
// VALIDAR COMANDO
// ========================================

bool comandoValido(const char* cadena) {

  if (strcmp(cadena, "D") == 0)
    return true;

  int inicio = 0;

  if (cadena[0] == 'A' && cadena[1] == 'A') {
    inicio = 2;
  }
  else if (cadena[0] == 'A' ||
           cadena[0] == 'B' ||
           cadena[0] == 'C') {

    inicio = 1;
  }
  else {
    return false;
  }

  if (cadena[inicio] == '\0')
    return false;

  int numero = 0;

  for (int i = inicio; cadena[i] != '\0'; i++) {

    if (cadena[i] < '0' || cadena[i] > '9')
      return false;

    int digito = cadena[i] - '0';

    if (numero > (9999 - digito) / 10)
      return false;

    numero = numero * 10 + digito;
  }

  return numero > 0;
}


// ========================================
// MENU INICIAL
// ========================================

void mostrarMenuInicial() {

  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setTextWrap(false);

  display.setCursor(0, 0);
  display.println(">>> ROBOTLOGO");

  display.setCursor(0, 16);
  display.println("1> Modo Interactivo");

  display.setCursor(0, 32);
  display.println("2> Modo Programa");

  display.display();
}


// ========================================
// MODO INTERACTIVO
// ========================================

void mostrarModoInteractivo() {

  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setTextWrap(false);

  display.setCursor(0, 0);
  display.println(">>> MODO INTERACTIVO");

  display.setCursor(0, 16);
  display.println("Escriba comando");

  display.setCursor(0, 24);
  display.println("y presione #");

  display.setCursor(0, 40);
  display.println("Guardar/Salir *");

  display.setCursor(0, 48);
  display.print("> ");
  display.print(cad_comando);

  // MOSTRAR CONFIRMACION O ANIMACION
  display.setCursor(0, 56);

  if (mostrarOK) {
    display.print("OK#");
  }
  else if (esperandoRespuesta) {

    for (int i = 0; i < cantidadPuntos; i++) {
      display.print(".");
    }
  }

  display.display();
}


// ========================================
// MODO PROGRAMA
// ========================================

void mostrarModoPrograma() {

  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setTextWrap(false);

  display.setCursor(0, 0);
  display.println(">>> MODO PROGRAMA");

  display.setCursor(0, 16);
  display.println("1> Cuadrado");

  display.setCursor(0, 24);
  display.println("2> Triangulo");

  display.setCursor(0, 32);
  display.println("3> Ultimo guardado");

  display.setCursor(0, 48);
  display.println("Salir *");

  // CONFIRMACION O ANIMACION
  display.setCursor(0, 56);

  if (mostrarOK) {
    display.print("OK#");
  }
  else if (esperandoRespuesta) {

    for (int i = 0; i < cantidadPuntos; i++) {
      display.print(".");
    }
  }

  display.display();
}


// ========================================
// MOSTRAR ERROR
// ========================================

void mostrarError(const char* linea1, const char* linea2) {

  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setTextWrap(false);

  display.setCursor(0, 16);
  display.println(linea1);

  display.setCursor(0, 32);
  display.println(linea2);

  display.display();

  delay(900);

  if (pantallaActual == MODO_INTERACTIVO)
    mostrarModoInteractivo();

  else if (pantallaActual == MODO_PROGRAMA)
    mostrarModoPrograma();
}


// ========================================
// ENTRAR AL MODO INTERACTIVO
// ========================================

void entrarModoInteractivo() {

  pantallaActual = MODO_INTERACTIVO;

  pos = 0;
  cad_comando[0] = '\0';

  esperandoRespuesta = false;
  mostrarOK = false;

  cantidadPuntos = 0;

  mostrarModoInteractivo();

  Serial.println("MODO INTERACTIVO");
}


// ========================================
// SALIR AL MENU INICIAL
// ========================================

void salirAlMenuInicial() {

  // SI EL ROBOT ESTA EJECUTANDO UN COMANDO,
  // ENVIAR PARADA DE EMERGENCIA

  if (esperandoRespuesta) {

    Serial2.print("*#");
    Serial.println("Parada enviada al micro1");
  }

  esperandoRespuesta = false;
  ejecutandoPrograma = false;

  programaActual = nullptr;

  cantidadComandosPrograma = 0;
  indiceComandoPrograma = 0;

  mostrarOK = false;

  pos = 0;
  cad_comando[0] = '\0';

  cantidadPuntos = 0;

  pantallaActual = MENU_INICIAL;

  mostrarMenuInicial();

  Serial.println("Regresando al menu inicial");
}


// ========================================
// INICIAR PROGRAMA
// ========================================

void iniciarPrograma(const char* const lista[], int cantidad) {

  programaActual = lista;

  cantidadComandosPrograma = cantidad;
  indiceComandoPrograma = 0;

  ejecutandoPrograma = true;
  esperandoRespuesta = true;

  mostrarOK = false;

  cantidadPuntos = 0;
  tiempoAnimacion = millis();

  mostrarModoPrograma();

  enviarSiguienteComando();
}


// ========================================
// ENVIAR SIGUIENTE COMANDO DEL PROGRAMA
// ========================================

void enviarSiguienteComando() {

  if (indiceComandoPrograma >= cantidadComandosPrograma) {

    esperandoRespuesta = false;
    ejecutandoPrograma = false;

    programaActual = nullptr;

    cantidadPuntos = 0;

    mostrarOK = true;

    Serial.println("PROGRAMA TERMINADO: OK#");

    if (pantallaActual == MODO_PROGRAMA)
      mostrarModoPrograma();

    return;
  }

  Serial2.print(programaActual[indiceComandoPrograma]);
  Serial2.print("#");

  Serial.print("Enviado al micro1: ");
  Serial.println(programaActual[indiceComandoPrograma]);

  esperandoRespuesta = true;

  cantidadPuntos = 0;
  tiempoAnimacion = millis();

  if (pantallaActual == MODO_PROGRAMA)
    mostrarModoPrograma();
}


// ========================================
// PROCESAR TECLAS
// ========================================

void procesarTecla(char key) {

  if (key == 0)
    return;

  Serial.print("Tecla: ");
  Serial.println(key);


  // ====================================
  // SALIR CON *
  // TIENE PRIORIDAD SOBRE LA ESPERA
  // ====================================

  if (key == '*' && pantallaActual != MENU_INICIAL) {

    salirAlMenuInicial();
    return;
  }


  // ====================================
  // MENU INICIAL
  // ====================================

  if (pantallaActual == MENU_INICIAL) {

    if (key == '1') {

      entrarModoInteractivo();
      return;
    }

    if (key == '2') {

      pantallaActual = MODO_PROGRAMA;

      mostrarOK = false;

      mostrarModoPrograma();

      Serial.println("MODO PROGRAMA");
      return;
    }

    return;
  }


  // ====================================
  // NO RECIBIR OTROS COMANDOS MIENTRAS
  // EL ROBOT ESTA TRABAJANDO
  // ====================================

  if (esperandoRespuesta)
    return;


  mostrarOK = false;


  // ====================================
  // MODO INTERACTIVO
  // ====================================

  if (pantallaActual == MODO_INTERACTIVO) {

    // ENVIAR COMANDO CON #

    if (key == '#') {

      if (pos == 0) {

        mostrarError("ERROR", "Comando vacio");
        return;
      }

      cad_comando[pos] = '\0';

      if (!comandoValido(cad_comando)) {

        pos = 0;
        cad_comando[0] = '\0';

        mostrarError("ERROR", "Comando invalido");
        return;
      }

      Serial2.print(cad_comando);
      Serial2.print("#");

      Serial.print("Enviado: ");
      Serial.println(cad_comando);

      esperandoRespuesta = true;

      cantidadPuntos = 0;
      tiempoAnimacion = millis();

      mostrarModoInteractivo();

      return;
    }

    // GUARDAR CARACTERES

    if (pos < sizeof(cad_comando) - 1) {

      cad_comando[pos++] = key;
      cad_comando[pos] = '\0';

      mostrarModoInteractivo();
    }
    else {

      pos = 0;
      cad_comando[0] = '\0';

      mostrarError("ERROR", "Comando largo");
    }

    return;
  }


  // ====================================
  // MODO PROGRAMA
  // ====================================

  if (pantallaActual == MODO_PROGRAMA) {

    // CUADRADO

    if (key == '1') {

      Serial.println("INICIANDO CUADRADO");

      iniciarPrograma(
        programaCuadrado,
        sizeof(programaCuadrado) / sizeof(programaCuadrado[0])
      );

      return;
    }

    // TRIANGULO

    if (key == '2') {

      Serial.println("INICIANDO TRIANGULO");

      iniciarPrograma(
        programaTriangulo,
        sizeof(programaTriangulo) / sizeof(programaTriangulo[0])
      );

      return;
    }

    // ULTIMO GUARDADO

    if (key == '3') {

      mostrarError("SIN HISTORIAL", "No hay guardado");
      return;
    }
  }
}


// ========================================
// RECIBIR RESPUESTA DEL MICRO1
// ========================================



void procesarRespuesta(String respuesta) {
  respuesta.trim();

  Serial.print("Respuesta recibida: ");
  Serial.println(respuesta);

  if (respuesta == "OK") {

    esperandoRespuesta = false;
    ejecutandoPrograma = false;
    programaActual = nullptr;

    cantidadComandosPrograma = 0;
    indiceComandoPrograma = 0;

    cantidadPuntos = 0;
    pos = 0;

    cad_comando[0] = '\0';

    mostrarOK = false;

    // Regresar al menú inicial
    pantallaActual = MENU_INICIAL;

    Serial.println("Robot listo. Regresando al menu inicial.");

    mostrarMenuInicial();

    return;
  }

  if (respuesta == "ERROR") {

    esperandoRespuesta = false;
    ejecutandoPrograma = false;
    programaActual = nullptr;

    cantidadComandosPrograma = 0;
    indiceComandoPrograma = 0;

    cantidadPuntos = 0;
    pos = 0;

    cad_comando[0] = '\0';

    mostrarOK = false;

    mostrarError("ERROR", "Comando rechazado");
  }
}



// ========================================
// RECIBIR RESPUESTA
// ========================================

void recibirRespuesta() {

  // RESPUESTA REAL DEL MICRO1 POR UART
  if (Serial2.available()) {

    String respuesta = Serial2.readStringUntil('#');

    procesarRespuesta(respuesta);
  }

  // RESPUESTA SIMULADA DESDE EL MONITOR SERIAL
  else if (Serial.available()) {

    String respuesta = Serial.readStringUntil('#');

    procesarRespuesta(respuesta);
  }
}



// ========================================
// ACTUALIZAR ANIMACION
// ========================================

void actualizarAnimacion() {

  if (!esperandoRespuesta)
    return;

  if (cantidadPuntos >= MAX_PUNTOS)
    return;

  if (millis() - tiempoAnimacion >= INTERVALO_ANIMACION) {

    tiempoAnimacion = millis();

    cantidadPuntos++;

    if (pantallaActual == MODO_INTERACTIVO)
      mostrarModoInteractivo();

    else if (pantallaActual == MODO_PROGRAMA)
      mostrarModoPrograma();
  }
}


// ========================================
// CONFIGURACION INICIAL
// ========================================

void setup() {

  Serial.begin(115200);

  // UART CON MICRO1
  Serial2.begin(115200, SERIAL_8N1, UART_RX, UART_TX);
  Serial2.setTimeout(20);

  // FILAS DEL TECLADO
  for (int i = 0; i < 4; i++) {

    pinMode(filas[i], OUTPUT);
    digitalWrite(filas[i], HIGH);
  }

  // COLUMNAS DEL TECLADO
  for (int i = 0; i < 4; i++) {

    pinMode(columnas[i], INPUT_PULLUP);
  }

  // COMUNICACION I2C
  Wire.begin(21, 22);

  // INICIAR OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {

    Serial.println("Error al iniciar OLED");

    while (true)
      delay(100);
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setTextWrap(false);

  mostrarMenuInicial();

  Serial.println("MICRO2 MANDO INICIADO");
}


// ========================================
// CICLO PRINCIPAL
// ========================================

void loop() {

  char key = leerTeclaNueva();

  procesarTecla(key);

  recibirRespuesta();

  actualizarAnimacion();

  delay(1);
}
