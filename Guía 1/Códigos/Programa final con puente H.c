
#define GPIO_OUT_REG (*(volatile unsigned int*)0x3FF44004)
#define GPIO_IN_REG  (*(volatile unsigned int*)0x3FF4403C)
#define GPIO_IN1_REG (*(volatile unsigned int*)0x3FF44040)

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

// Patrones para seleccionar cada fila
const unsigned int row_sel[4] = {
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

// Variables para detectar la secuencia AA
bool primeraA = false;
unsigned long tiempoA = 0;
char teclaAnterior = 0;

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

  unsigned int val = GPIO_OUT_REG;
  GPIO_OUT_REG = val | ROW_MASK;

  controlMotores(0, 0);
}

// Lectura del teclado
char readKeyPad() {

  char tecla = 0;
  int col = 0;

  for (int fila = 0; fila < 4; fila++) {

    unsigned int val = GPIO_OUT_REG;
    val = (val & ~ROW_MASK) | row_sel[fila];
    GPIO_OUT_REG = val;

    unsigned int portc0 = GPIO_IN_REG;
    unsigned int portc1 = GPIO_IN1_REG;

    unsigned char columnas =
        (((portc0 >> COL1) & 1) << 0) |
        (((portc0 >> COL2) & 1) << 1) |
        (((portc1 >> 1) & 1) << 2) |
        (((portc1 >> 0) & 1) << 3);

    if (columnas != 0x0F) {

      col = (columnas & 0x01) == 0 ? 0 :
            (columnas & 0x02) == 0 ? 1 :
            (columnas & 0x04) == 0 ? 2 : 3;

      tecla = matTeclas[fila][col];
      break;
    }
  }

  return tecla;
}

// Control de los motores
// 1 = adelante, -1 = atras, 0 = detenido
void controlMotores(int izquierdo, int derecho) {

  digitalWrite(MI1, izquierdo == 1);
  digitalWrite(MI2, izquierdo == -1);

  digitalWrite(MD1, derecho == 1);
  digitalWrite(MD2, derecho == -1);
}

void printdisplay(char caracter) {

  // Aqui va tu codigo del display
}

void loop() {

  char key = readKeyPad();

  // Detectar una nueva pulsacion
  if (key != 0 && teclaAnterior == 0) {

    Serial.println(key);

    switch (key) {

      // A: Adelante / AA: Atras
      case 'A':

        if (!primeraA || millis() - tiempoA > 2000) {

          primeraA = true;
          tiempoA = millis();

          controlMotores(1, 1);

          Serial.println("Adelante");
        }
        else {

          primeraA = false;

          controlMotores(-1, -1);

          Serial.println("Atras");
        }

        break;

      // B: Giro derecha
      case 'B':

        primeraA = false;
        controlMotores(1, -1);

        Serial.println("Giro derecha");
        break;

      // C: Giro izquierda
      case 'C':

        primeraA = false;
        controlMotores(-1, 1);

        Serial.println("Giro izquierda");
        break;

      // D: Bajar/levantar pluma
      case 'D':

        primeraA = false;

        Serial.println("Control de pluma");
        break;

      // #: Enter
      case '#':

        primeraA = false;

        Serial.println("Enter");
        break;

      // *: Parar
      case '*':

        primeraA = false;
        controlMotores(0, 0);

        Serial.println("Motores detenidos");
        break;
    }

    printdisplay(key);
  }

  // Actualizar estado de la tecla
  teclaAnterior = key;

  delay(30);
}
