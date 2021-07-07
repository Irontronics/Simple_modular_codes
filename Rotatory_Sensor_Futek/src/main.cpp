//Código del encoder Futek TRS605  para Arduino Mega
#include <RotaryEncoder.h>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>

//Pines de interrupción a usar 
RotaryEncoder encoder(A13, A14);

void setup()
{
  Serial.begin(9600);
  Serial.println("SimplePollRotator example for the RotaryEncoder library.");

  // Registros directos del microcontrolador para habilitar interrupciones 
  PCICR |= (1 << PCIE2);   
  PCMSK2 |= (1 << PCINT21) | (1 << PCINT22);  
} 


//Rutina de servicio a interrupción para los registros de interrupción anteriormente habilitados
ISR(PCINT2_vect) {
  encoder.tick(); 
}


// Read the current position of the encoder and print out when changed.
void loop()
{
  static int pos = 0;
  static int milis=0;//guarda el instante
  float deltaT,deltaPos,angSpeed;  
  int newmilis=0;//inicializa nuevo instante en cero;
  int newPos = encoder.getPosition(); //Toma valor de posición del encoder 
  
  if (pos != newPos) {//si hubo un cambio en la cuenta, entonces: 
    newmilis=millis();
    deltaPos=newPos-pos;//calcular el cambio en cuentas, 

    if(deltaPos){//procesar solo deltas positivos, skipeando los overflows
        deltaT=((newmilis-milis)/1000);//deltat en segundos
        deltaPos=deltaPos/360;//deltapos en revoluciones ?
        angSpeed=deltaPos/deltaT;//
        Serial.print(angSpeed,3);
        Serial.println();
        delay(1000);//lanzar lecturas solo cada segundo
      }
    milis=newmilis;
    pos = newPos;


  } // if
} // loop ()
