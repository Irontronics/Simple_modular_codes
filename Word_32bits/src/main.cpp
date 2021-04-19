#include <Arduino.h>
long VBUS_value = 0; //registro de VBUS VALUE,  00000000 00000000 00000000 00000000 32bits // tIPO DE DATO DE 32 BITS (4 Bytes) coma flotante. 

long numero_MSB = 2; //00000010 8bin, 02 hex, 2 dec , Este Byte me lo la dirección 806 Modbus como MSB // Tipo de dato 32 (4Bytes) bits Entero
long numero_LSB = 47102; //10110111 11111110 16bin, B7 FE hex, 47102 dec. , Estos dos bytes me los dio Direccion 807 como MLSB Tipo de dato 16 bits ENTERO
long num2 =0; // Utilizado para almecenar la correción de bits de MSB (4 bytes)


String mensaje; 

void setup() {
  Serial.begin(9600); 
  Serial.println(VBUS_value, BIN); //mostrar registro en binario (0)
  Serial.println(numero_MSB, BIN); //mostrar parte MSB (10)

  num2 = numero_MSB << 16; //agregar MSB a num2 (recorrer el byte MSB a posición 00000000 00000010 000000000 00000000)
  Serial.println(num2, BIN); 

  VBUS_value = num2 | numero_LSB; // Hacer una OR a nivel bits para agregar la otra parte que falta: 00000000 00000010 10110111 11111110 , haciendo esto tenemos acomodado el registro
  Serial.println(VBUS_value);
  Serial.println(VBUS_value,HEX);
  mensaje= "Voltaje VBUS_value es: " + String((VBUS_value/1000.0)) + " Volts"; //muestra el valor flotante del registro VBUS_Value de servodrive
  Serial.println(mensaje);

 
}

void loop() {
  // put your main code here, to run repeatedly:

}
