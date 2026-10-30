void loop() {
//Read Keypad
char tecla = 0;
digitalWrite(ROW1, LOW);
digitalWrite(ROW2, HIGH);
digitalWrite(ROW3, HIGH);
digitalWrite(ROW4, HIGH);
if (digitalRead(COL1)==LOW)
tecla = '1';
else if (digitalRead(COL2)==LOW)
tecla = '2';
else if (digitalRead(COL3)==LOW)
tecla = '3';
else if (digitalRead(COL4)==LOW)
tecla = 'A';
digitalWrite(ROW1, HIGH);
digitalWrite(ROW2, LOW);
digitalWrite(ROW3, HIGH);
digitalWrite(ROW4, HIGH);
if (digitalRead(COL1)==LOW)
tecla = '4';
else if (digitalRead(COL2)==LOW)
tecla = '5';
else if (digitalRead(COL3)==LOW)
tecla = '6';
else if (digitalRead(COL4)==LOW)
tecla = 'B';
digitalWrite(ROW1, HIGH);
digitalWrite(ROW2, HIGH);
digitalWrite(ROW3, LOW);
digitalWrite(ROW4, HIGH);
if (digitalRead(COL1)==LOW)
tecla = '7';
else if (digitalRead(COL2)==LOW)
tecla = '8';
else if (digitalRead(COL3)==LOW)
4
tecla = '9';
else if (digitalRead(COL4)==LOW)
tecla = 'C';
digitalWrite(ROW1, HIGH);
digitalWrite(ROW2, HIGH);
digitalWrite(ROW3, HIGH);
digitalWrite(ROW4, LOW);
if (digitalRead(COL1)==LOW)
tecla = '*';
else if (digitalRead(COL2)==LOW)
tecla = '0';
else if (digitalRead(COL3)==LOW)
tecla = '#';
else if (digitalRead(COL4)==LOW)
tecla = 'D';
if (tecla!=0) {
Serial.println(tecla);
}
}
