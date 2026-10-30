
// Pines de conexión de los segmentos
const int segmentos[7] = {
  23, 22, 21, 19, 18, 5, 4
};

// Tabla de patrones: A, B, C, D, E, F, G
// 1 = segmento encendido
// 0 = segmento apagado
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

// Función para visualizar un carácter
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

  // Actualizar los segmentos
  for (int i = 0; i < 7; i++) {
    digitalWrite(segmentos[i], caracteres[indice][i]);
  }
}

void setup() {

  // Configurar los pines como salidas
  for (int i = 0; i < 7; i++) {
    pinMode(segmentos[i], OUTPUT);
  }

  // Mostrar un carácter inicial
  printdisplay('6');
}

void loop() {
  // Sin instrucciones adicionales
}
