//Monitoreo de señales del servodrive a master arduino
#include <Arduino.h>
#include <MgsModbus.h> 
#include <Ethernet.h>
#include <SPI.h>

//variables para el tiempo
bool tiempo_flag=false; 
unsigned long currentMillis =0; 
unsigned long previousMilis = 0; 
unsigned int interval = 100; 

String mensaje1 = ""; 

//variables de llegada de ordenes de VS Studio a Arduino modbus 
bool set_command = false; //Para mandar escrituras a registros modbus de ajustes General 
bool comand_in = false; //Se recibe un comando de la interfaz de usuario (VS Studio mando algo a arduino, un comando)
bool StringComplete; //Flag de que se cachó todo el string proveniente de Vs Studio serial port 
String inputString = ""; // String de lo que llego del puerto serial 
byte flag = 0;  //La uso para determinar que voy a mandar por modbus 

long MSB_W_VL; //Tomará ambos byte de MSB 
long LSB_W_VL; //Tomará ambos byte de LSB 
long MSB_W_VBUS; //Tomará ambos byte de MSB 
long LSB_W_VBUS; //Tomará ambos byte de LSB 
byte STATUS_drv; 
byte STATUS_stop;
byte Temp_DRV;  

bool start = false;  //flag para iniciar monitoreo modbus o dar tiempo para mandar comandos 


void tiempo();
void Poller(); // TODO:  se debe de definir función antes 
void Poller_2();
long organizar_w(long a, long b);

//Creación de objeto Mb
MgsModbus Mb;

int acc = 0; //accumulator  / seconds counter //32,767 number 

// Ethernet settings (depending on MAC and Local network)
byte mac[] = {0x90, 0xA2, 0xDA, 0x0E, 0x94, 0xB5 };
IPAddress ip(192, 168, 0, 192);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);

void setup() {
   Serial.begin(9600);

  //initialize ethernet shield 
  Ethernet.begin(mac, ip, gateway, subnet); 

//Server - Slave address: //IP SERVO DRIVE
  Mb.remServerIP[0] = 192;
  Mb.remServerIP[1] = 168;
  Mb.remServerIP[2] = 0;
  Mb.remServerIP[3] = 195;
  
}

void loop() {

if (!start){ // monitoreo modbus 
  Poller(); 
  Mb.MbmRun(); 
}

if(start and (!comand_in)){ //cuando se terminó de tomar lectura a registros modbus, organizar string para mandar a visual studio 
long result1= organizar_w(MSB_W_VL, LSB_W_VL);
long result2 = organizar_w(MSB_W_VBUS, LSB_W_VBUS); 
mensaje1 = String(result1) + "Z" + String(STATUS_drv) + "Y"  + String(STATUS_stop) + "X" + String(result2) + "W" + String(Temp_DRV) + "V" + "\n"; 
Serial.print(mensaje1); //lo mandamos a visual studio para mostrar en HMI 
tiempo(); //le damos un pequeño tiempo de refresco 
start = false; //flag para volver a tomar lecturas de modbus  
}


if(comand_in){ //si se recibio un dato proveniente de la interfaz visual estudio entonces: 
  if(set_command){ //si es es un comando valido entonces ingresa 
    Poller_2(); 
    Mb.MbmRun();
}
//! Si fuera un comando no valido que hacer?, pero como lo va a mandar HMI Visual, el comando estará definido ya. 
//else{
    //acc=0;
  //  comand_in=false; 
 //   inputString =""; 
//}
} //comand_in
} //loop

void Poller(){  //secuencia de modbus registros de monitoreo: velocidad, voltaje, temperatura, Driver En, emergency Stop 
  delay(1);
  acc = acc + 1; 

  if(acc ==120){ //!Mete ruido cuando se coloca una velocidad alta (300 + 200) ok , 
    Mb.MbData[0] = 0; 
    Mb.MbData[1] = 0;
    Mb.Req(MB_FC_READ_REGISTERS,  858,4,0); //Solicitud de lectura registros velocidad
  }

  if(acc ==  240){ 
    MSB_W_VL = Mb.MbData[0];
    LSB_W_VL = Mb.MbData[1];
    }

    if(acc == 360){
    Mb.MbData[0] = 0; 
    Mb.MbData[1] = 0;  
    Mb.Req(MB_FC_READ_REGISTERS,  221,1,0); //Solicitud de lectura status activación drive
  }

    if(acc == 480){
    STATUS_drv = Mb.MbData[0]; 
    }
    
    
    if(acc == 600){
    Mb.MbData[0] = 0; 
    Mb.Req(MB_FC_READ_REGISTERS,  1055,1,0); //Solicitud de lectura status paro emergencia
  }

    if(acc == 720){
    STATUS_stop = Mb.MbData[0];
    }
    
    if(acc == 840){
    Mb.MbData[0] = 0;  
    Mb.Req(MB_FC_READ_REGISTERS,  806,4,0); //Solicitud de lectura voltaje de potencia 
  }

  if(acc == 960){
    MSB_W_VBUS = Mb.MbData[0]; 
    LSB_W_VBUS = Mb.MbData[1];
    }

  if(acc == 1080){
    Mb.MbData[0] = 0; 
    Mb.MbData[1] = 0;
    Mb.Req(MB_FC_READ_REGISTERS,  1749,1,0); //Solicitud de lectura de temperatura driver 
  }

  if(acc == 1200){
    Temp_DRV = Mb.MbData[0]; 
    acc =0; 
    start = true;  //fin 
  }  
} //Poller


void Poller_2(){ //Solicitudes de escritura de registros modbus, comandado por vs studio 
  delay(1);
  acc = acc + 1; 

 if(acc ==120 and flag == 1){ //flag 1 = activar potencia 
    Mb.MbData[0] = 1; 
    Mb.Req(MB_FC_WRITE_MULTIPLE_REGISTERS,  255,1,0); //manda solicitud para escribir registro de activación potencia y  lo escribe
    acc=0;
    comand_in=false; 
    set_command=false;
} 

else if (acc == 120 and flag == 2){ //manda solicitud para escribir registro de deshabilitación potencia y lo escribe 
    Mb.MbData[0] = 1; 
    Mb.Req(MB_FC_WRITE_MULTIPLE_REGISTERS,  237,1,0); 
    acc=0;
    comand_in=false; 
    set_command=false;
}
}

long organizar_w(long a, long b){
  long num2 =0; 
  num2 = a << 16;
  num2 = num2 | b;
  return num2; 
}

void tiempo(){ //función de tiempo
  currentMillis = millis(); 
  previousMilis=currentMillis; 
  do{
  if(currentMillis - previousMilis  >= interval ){
    previousMilis = currentMillis;
    tiempo_flag = true; 
    }
    currentMillis = millis();
    }while(!tiempo_flag); 
     
  delay(50);
  tiempo_flag=false;  
  }


void serialEvent (){ // lee la cadena proveniente de visual studio 
  comand_in = true; //bandera de que se recibió un comando de HMI 
  
  while (Serial.available() > 0){

    char inChar = (char)Serial.read();
    if(inChar == '$'){ StringComplete = true;}
    else { inputString += inChar;}
  }

  if(StringComplete){  //validar comando recibido 
    inputString.trim(); 

    if(inputString.equals("A2")){
      flag = 1; 
      acc=0;
      set_command = true;
      inputString =""; 
    }

    else if (inputString.equals("B2")){
      flag = 2; 
      acc=0;
      set_command = true;
      inputString =""; 
    }
    }
  }
