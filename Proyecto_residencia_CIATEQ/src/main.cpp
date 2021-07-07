#include <Arduino.h>
#include <MgsModbus.h> 
#include <Ethernet.h>
#include <SPI.h>
#include <RotaryEncoder.h>
#include <Adafruit_ADS1X15.h>
#include <Wire.h>

// Crear objeto de la clase
Adafruit_ADS1115 ads;
float AverageTemperature = 0;
int MeasurementsToAverage = 24;
float factorEscala = 	0.0078125F;

String mensaje2 = "";  
bool start2 = false; 

//salidas digitales para freno industrial 
#define  CH1 7  
#define  CH2 6  

//definición de pines de interrupción para sensor Futek Velocidad 
RotaryEncoder encoder(A13, A14);

//variables para separar ajustes
bool finish1, finish2, finish3, finish4, finish5, finish6, finish7,finish8;  
String one, two, three, four, five, six, seven, eight;   
byte num1;
long num2, num3, num4,num5;
word num6, num7,num8;  

//variables para escritura y lectura modbus 
bool iniciar_set = false; 
bool iniciar_set2 = false;  
bool iniciar_set3 = false; 
bool stop_axis=false; 
bool stop_move=false;  
bool COMANDO_AXIS=false;  
bool COMANDO_MOVE=false;   
bool command_in = false;   

//Funcion de tiempo 
bool tiempo_flag=false; 
unsigned long currentMillis =0; 
unsigned long previousMilis = 0; 
void tiempo(unsigned int a);

//declaración de funciones 
void Poller(); 
void Poller_2();
void Poller_3();
void Poller_4();
void acondicionar_variables_a_modbus();
void separar_ajustes_modbus(String Str_complete );
void activar_desactivar_potencia(); 
void activar_desactivar_movimiento();
long organizar_w(long a, long b); 
float analog_torque_voltage(); 
float Velocidad_torque_futek();

//acondicionar mensaje recibido de GUI a arduino  
byte command = 0; 
String inputString = "";

//inicio de modbus 
String mensaje1 = "";   
long MSB_W_VL;  
long LSB_W_VL;  
byte STATUS_drv;  
byte STATUS_stop;  
bool start = false;   

//Creación de objeto Mb
MgsModbus Mb;
int acc = 0; //accumulator 

// Ethernet settings (depending on MAC and Local network)
byte mac[] = {0x90, 0xA2, 0xDA, 0x0E, 0x94, 0xB5 };
IPAddress ip(192, 168, 0, 192);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);


void setup() {
  Serial.begin(9600);
  pinMode(CH1, OUTPUT); 
  pinMode(CH2, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT); //led for testing 
  
  //*initialize ethernet shield , con este activo, es cuando no hace match a veces con el com serial. 
  Ethernet.begin(mac, ip, gateway, subnet); 
  
  //*Server - Slave address: //IP SERVO DRIVE
  Mb.remServerIP[0] = 192;
  Mb.remServerIP[1] = 168;
  Mb.remServerIP[2] = 0;
  Mb.remServerIP[3] = 195;
  
  //*Registros directos del microcontrolador para habilitar interrupciones 
  PCICR |= (1 << PCIE2);   
  PCMSK2 |= (1 << PCINT21) | (1 << PCINT22); 

  // Factor de escala
  ads.setGain(GAIN_SIXTEEN);

  // Iniciar el ADS1115
  ads.begin();

}

//Rutina de servicio a interrupción para los registros de interrupción anteriormente habilitados
ISR(PCINT2_vect) {
  encoder.tick(); 
}

void loop() {

  if(command == 1){ // Modo generador 

    if( !start and !command_in ) { //Monitoreo general modbus 
      Poller(); 
      Mb.MbmRun(); 
    }

    if(start   and !command_in){ //Organizar String para GUI Visual studio de datos de monitoreo general
      float result_torq_voltage = analog_torque_voltage(); 
      long result1= organizar_w(MSB_W_VL, LSB_W_VL); 
      mensaje1 = String(result1) + "Z" + String(STATUS_drv) + "Y"  + String(STATUS_stop) + "X" + String(result_torq_voltage,4) + "W" + "\n"; 
      Serial.print(mensaje1); 
      tiempo(250);  
      start = false; //flag para volver a tomar lecturas de modbus  
    }

    if(COMANDO_AXIS){ //Si se encuentra comando axis entonces 
      acc=0;
      activar_desactivar_potencia(); 
      command_in = false; 
      COMANDO_AXIS = false; 
      tiempo(380);
      acc=0; 
    }

    if(COMANDO_MOVE){ //Si el comando MOVE esta activo entonces 
      acc=0;
      activar_desactivar_movimiento();
      command_in = false; 
      COMANDO_MOVE = false; 
      tiempo(380);
      acc=0; 
    }
  } //llave de cierre modo generador 

  else if (command == 2) { //El controlador espera modo de selección 
    digitalWrite(LED_BUILTIN, HIGH);   
    tiempo(1100);                   
    digitalWrite(LED_BUILTIN, LOW);    
    tiempo(1100);
  }

  else if (command == 3) { // Modo motor 


    if(start2){ //inicio de monitoreo 
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
          float dato = ads.readADC_Differential_0_1();
          mensaje2 = String(angSpeed) + "%" + String(dato) + "Y" + "\n"; 
          Serial.print(mensaje2);
          //Serial.println();
          delay(1000);//lanzar lecturas solo cada segundo
        }
      milis=newmilis;
      pos = newPos;
      } // if

      
  //for(int i = 0; i < MeasurementsToAverage; ++i)
 // {
 //   AverageTemperature += ads.readADC_Differential_0_1();
 //   delay(1);
 // }
 // AverageTemperature /= MeasurementsToAverage;
 // AverageTemperature *= factorEscala;
  //Serial.println(AverageTemperature);
     // float result_torq_voltage = analog_torque_voltage();  //obtenemos el voltaje de torque 
      //Serial.println(result_torq_voltage);
     // tiempo(450);
     // float result_torq_voltage = analog_torque_voltage();  //obtenemos el voltaje de torque 
     // Serial.println(result_torq_voltage)
      //7mensaje2 = String(texto) + "Z" + String(result_torq_voltage) + "Y" + "\n";
      //Serial.print(mensaje2);
      //tiempo(250);  
     // start = false; //flag para volver a tomar lecturas de modbus 

    }
  } //llave de cierre modo motor 
}//Loop


void serialEvent (){ // lee la cadena proveniente de visual studio HMI 
  bool StringComplete = false;
  while (Serial.available() > 0){
    char inChar = (char)Serial.read();
    if(inChar == '$'){ StringComplete = true;}
    else { inputString += inChar;}
  }

  if(StringComplete){  //validar comando recibido 
    inputString.trim();  
    if(inputString.length() == 4){
      if(inputString =="Init"){ 
      inputString =""; 
      tiempo(300);
      Serial.println("123Z"); //comando de respuesta "alive" de arduino a HMI
      }
    } 
    else if (inputString.length() == 2){
      if(inputString =="A2"){ //modo en espera de selección  
        inputString =""; 
        command = 2; 
      }
      else if (inputString =="B2"){ //Modo generador 
        digitalWrite(CH2, HIGH); //settear freno en modo libre 
        tiempo(300);
        digitalWrite(CH2, LOW); 
        command_in= true ;  //no imprime valores   
        start = true; //no se mete a modbus
        command = 1;  //se queda en programa de generador 
        inputString ="";
      }

      else if (inputString =="D2"){ //Modo motor 
        inputString ="";
        command = 3;  
      }

      else if (inputString =="C2"){  // sin activación, se pierde comunicación con GUI de usuario 
        inputString ="";
        digitalWrite(LED_BUILTIN, LOW);
        command = 0;  
      }

      else if (inputString =="E2"){  //Cuando el usuario entra a pestaña de Service Motion en modo generador
        command_in= true ;  //no imprime valores   
        start = true; //no se mete a modbus 
        inputString ="";
      }
      else if (inputString =="F2"){  //Cuando el usuario esta en pestaña monitoreo modbus modo generador
        command_in = false;
        inputString ="";
        start = false ;  //Reanuda monitoreo modbus en aplicación 
      }
      else if (inputString =="G2"){ //Activar Axis  
        stop_axis = true;   
        COMANDO_AXIS = true; 
        command_in  = true; //Detiene monitoreo General momentaneamente
        tiempo(300);
        inputString ="";
      }
      else if (inputString =="H2"){  //Desactivar axis  
        stop_axis = false; 
        COMANDO_AXIS = true; 
        command_in  = true;  //Detiene monitoreo General momentaneamente
        tiempo(300);
        inputString ="";
      }
      else if (inputString =="I2"){  //Iniciar movimiento
        stop_move = true;    
        COMANDO_MOVE = true;  
        command_in  = true; //Detiene monitoreo General momentaneamente
        tiempo(300);
        inputString ="";
      }
      else if (inputString =="J2"){  //Detener movimiento  
        stop_move = false;  
        COMANDO_MOVE = true;  
        command_in  = true; //Detiene monitoreo General momentaneamente
        tiempo(300);
        inputString ="";
      }

      else if (inputString =="K2"){  //Iniciar monitoreo modo motor (inicio de prueba)
        start2 = true; 
        tiempo(150);
        inputString ="";
      }

        else if (inputString =="L2"){  //No entrar en monitoreo de modo motor (paro de prueba)
        start2 = false; 
        tiempo(150);
        inputString ="";
      }

        else if (inputString =="M2"){  //Activar freno ajustado
        digitalWrite(CH1, HIGH); //settear freno ajustado
        tiempo(300);
        digitalWrite(CH1, LOW); 
        tiempo(150);
        inputString ="";
      }
        else if (inputString =="N2"){  //Activar freno libre
        digitalWrite(CH2, HIGH); //settear freno libre
        tiempo(300);
        digitalWrite(CH2, LOW); 
        tiempo(150);
        inputString ="";
      }



    } //If de comandos 
    else if (inputString.length() >= 8){ //de lo contrario si cadena recibida es larga (datos de settings modbus GUI to Arduino)
      separar_ajustes_modbus(inputString); //a separar ajustes 
      acondicionar_variables_a_modbus();  //acondicionar y escribir registros modbus 
      inputString ="";
    }
    else{inputString = "";}
  } //If string Complete 
} //función de dato serial recibido 

 
void Poller(){ //secuencia de modbus registros de monitoreo: velocidad,Driver En, emergency Stop 
  delay(1);
  acc = acc + 1; 
  
  if(acc ==120){ //Mete ruido cuando se coloca un tiempo pequeño en acc 
    Mb.MbData[0] = 0; 
    Mb.MbData[1] = 0;
    Mb.Req(MB_FC_READ_REGISTERS,  858,2,0); // VL.FB
  }
  if(acc ==  240){ 
    MSB_W_VL = Mb.MbData[0];
    LSB_W_VL = Mb.MbData[1]; //Velocidad motor 
  }
  
  if(acc == 360){
    Mb.MbData[0] = 0; 
    Mb.Req(MB_FC_READ_REGISTERS,  221,1,0); //DRV.ACTIVE
  }
  if(acc == 480){
    STATUS_drv = Mb.MbData[0]; //Status driver 
  }
  
  if(acc == 600){
    Mb.MbData[0] = 0; 
    Mb.Req(MB_FC_READ_REGISTERS,  1055,1,0); //DRV.HWENABLE
  }
  if(acc == 720){
    STATUS_stop = Mb.MbData[0]; //Status paro de emergencia 
    acc =0; 
    start = true;  //fin 
  }
} //Poller_monitoreo general 


void Poller_2(){  //Registros de Axis Servomotor
  delay(1);
  acc = acc + 1; 

  if(stop_axis){  //si se va activar, entonces 
    if(acc == 350){
      Mb.MbData[0] = 1; 
      Mb.Req(MB_FC_WRITE_MULTIPLE_REGISTERS,  254,1,0); //DRV.EN
    }
    if(acc == 550){
      Mb.MbData[0] = 0; 
      Mb.Req(MB_FC_WRITE_MULTIPLE_REGISTERS,  254,1,0); //DRV.EN 
      acc = 0 ;
      iniciar_set2 = true; //salimos del loop 
    }   
  } 
  else if (!stop_axis){ //si se va a desactivar, entonces 
    if(acc == 350){
      Mb.MbData[0] = 1; 
      Mb.Req(MB_FC_WRITE_MULTIPLE_REGISTERS,  236,1,0); //DRV.DIS
    }
    if(acc == 550){
      Mb.MbData[0] = 0; 
      Mb.Req(MB_FC_WRITE_MULTIPLE_REGISTERS,  236,1,0); //DRV.DIS
      acc = 0 ;
      iniciar_set2 = true;  //salimos del loop 
    }
  }
}

void Poller_4(){  //Registros de Movimiento Servomotor
  delay(1);
  acc = acc + 1; 
  if(stop_move){ //si se va a iniciar movimiento, entonces 
    if(acc == 350){
      Mb.MbData[0] = 1; 
      Mb.Req(MB_FC_WRITE_MULTIPLE_REGISTERS,  752,1,0); //SM.MOVE
    }
    if(acc == 550){
      Mb.MbData[0] = 0; 
      Mb.Req(MB_FC_WRITE_MULTIPLE_REGISTERS,  752,1,0); //SM_MOVE
      acc = 0 ;
      iniciar_set3 = true; //salimos del loop
    }
  }
  else if (!stop_move){ //si se va a detener movimiento 
    if(acc == 350){
      Mb.MbData[0] = 1; 
      Mb.Req(MB_FC_WRITE_MULTIPLE_REGISTERS,  274,1,0); //DRV.STOP
    }
    if(acc == 550){
      Mb.MbData[0] = 0; 
      Mb.Req(MB_FC_WRITE_MULTIPLE_REGISTERS,  274,1,0); //DRV.STOP
      acc = 0 ;
      iniciar_set3 = true; //salimos del loop
    }
  }
} 

void activar_desactivar_potencia(){ //función axis servomotor
  while(!iniciar_set2){ //loop de escritura registros 
    Poller_2();
    Mb.MbmRun();
  } 
  iniciar_set2=false; 
}

void activar_desactivar_movimiento(){ //función movimiento servomotor
  while(!iniciar_set3){ //loop de configuración 
    Poller_4();
    Mb.MbmRun();
  }
  iniciar_set3=false; 
}

long organizar_w(long a, long b){ //organizar palabras largas 
  long num2 =0; 
  num2 = a << 16;
  num2 = num2 | b;
  return num2; 
}

void tiempo(unsigned int a){ //función de tiempo
  currentMillis = millis(); 
  previousMilis=currentMillis; 
  do{
    if(currentMillis - previousMilis  >= a ){
      previousMilis = currentMillis;
      tiempo_flag = true; 
    }
    currentMillis = millis();
  }while(!tiempo_flag);    
  delay(50);
  tiempo_flag=false;  
}

void separar_ajustes_modbus(String Str_complete){ //separar paquete de datos de configuración modbus 
  char b; 
  int a = Str_complete.length();   

  for(int i =0;i <= a;i++){ //hacemos un recorrido por los caracteres del string frame 
    b= Str_complete.charAt(i);  //le damos el valor de dicho caracter a b 
    if(b == 'A' or finish1){  
        finish1 = true; 
        if( b == 'B' or finish2){ 
          finish2 = true; 
          if(b == 'C' or finish3){ 
            finish3 = true; 
            if(b == 'D' or finish4){
              finish4 = true;
              if(b == 'E' or finish5){
                finish5=true;
                if(b =='F' or finish6){
                  finish6 = true;
                  if(b == 'G' or finish7){
                    finish7 = true;
                    if(b == 'H' or finish8){
                      finish8 = true;
                    }
                    else if (b != 'G'){ eight += b;} 
                  } // septimo paquete
                  else if (b != 'F'){seven += b;}
                } // sexto paquete
                else if (b!='E'){six += b;}
              } // quinto paquete
              else if(b!= 'D'){five += b;}
            } // cuarto paquete
            else if(b!= 'C'){four += b;}
          }//tercer paquete
          else if(b!= 'B'){three += b;}
        } //segundo paquete
        else if( b!= 'A'){two += b;}
    }//primer paquete
    else {one += b;} 
  } //for
Str_complete = ""; 
} //funcion 

void acondicionar_variables_a_modbus(){ //Acondicionar variables y escribir a registros modbus
   num1 = one.toInt(); //modo de operacion
   num2 = two.toInt(); // Velocidad1
   num3 = three.toInt(); //Velocidad2
   num4 = four.toInt(); //Aceleración
   num5 = five.toInt(); //Deceleración
   num6 = six.toInt();  //Time1
   num7 = seven.toInt(); //Time2
   num8 = eight.toInt(); //Sentido de giro
   acc =0; 
 
  while(!iniciar_set){ //bucle de escritura registros modbus 
    Poller_3(); 
    Mb.MbmRun();
  }

  //limpiar todo 
  finish1=false; 
  finish2=false; 
  finish3=false; 
  finish4=false; 
  finish5=false; 
  finish6=false;  
  finish7=false;
  finish8=false; 
  iniciar_set = false; 
  num1 = 0; 
  num2 = 0; 
  num3 = 0; 
  num4 = 0; 
  num5 = 0; 
  num6 = 0; 
  num7 = 0; 
  num8 = 0; 
  one = '0'; 
  two = '0'; 
  three = '0'; 
  four = '0'; 
  five = '0'; 
  six = '0'; 
  seven = '0'; 
  eight = '0'; 
}

void Poller_3(){ //Configurar registros modbus 
  delay(1);
  acc = acc + 1; 
  
  if(acc ==120 ){ 
    Mb.MbData[0] = num1; 
    Mb.Req(MB_FC_WRITE_MULTIPLE_REGISTERS,  751,1,0); //SM_MODE
  } 

  if (acc == 240){ 
    Mb.MbData[0] =  num2;  
    Mb.Req(MB_FC_WRITE_MULTIPLE_REGISTERS,  758,1,0);  //Velocidad 1 
  }

  if (acc == 360){ 
    Mb.MbData[0] =  num3;  
    Mb.Req(MB_FC_WRITE_MULTIPLE_REGISTERS,  760,1,0);  //Velocidad 2
  }

  if (acc == 480){ 
    Mb.MbData[0] =  num4;  
    Mb.Req(MB_FC_WRITE_MULTIPLE_REGISTERS,  218,1,0);  //ACC
  }

  if (acc == 600){ 
    Mb.MbData[0] =  num5;  
    Mb.Req(MB_FC_WRITE_MULTIPLE_REGISTERS,  232,1,0);  //DECC
  }

  if (acc == 720){ 
    Mb.MbData[0] =  num6;  
    Mb.Req(MB_FC_WRITE_MULTIPLE_REGISTERS,  755,1,0);  //Timer1 
  }

  if (acc == 840){ 
    Mb.MbData[0] =  num7;  
    Mb.Req(MB_FC_WRITE_MULTIPLE_REGISTERS,  757,1,0);  //Timer2
  }

  if (acc == 960){ 
    Mb.MbData[0] =  num8;  
    Mb.Req(MB_FC_WRITE_MULTIPLE_REGISTERS,  235,1,0);  //Sentido de giro
    iniciar_set = true; //sale del bucle while 
    acc=0; 
  }
}

float analog_torque_voltage(){

  for(int i = 0; i < MeasurementsToAverage; ++i)
  {
    AverageTemperature += ads.readADC_Differential_0_1();
    delay(1);
  }
  AverageTemperature /= MeasurementsToAverage;
  AverageTemperature *= factorEscala;
  return(AverageTemperature);
}

float Velocidad_torque_futek(){
      static int pos = 0;
      static int milis=0;
      float deltaT,deltaPos,angSpeed;  
      int newmilis=0;
      int newPos = encoder.getPosition(); //Toma valor de posición del encoder 
  
      if (pos != newPos) {//si hubo un cambio en la cuenta, entonces: 
        newmilis=millis();
        deltaPos=newPos-pos;//calcular el cambio en cuentas, 
        if(deltaPos){
          deltaT=((newmilis-milis)/1000);
          deltaPos=deltaPos/360;
          angSpeed=deltaPos/deltaT;
          //return angSpeed; 
          Serial.println(angSpeed,3); 
          tiempo(1000); 
        }
      milis=newmilis;
      pos = newPos;
      }
}