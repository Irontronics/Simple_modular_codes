//Monitoreo de señales del servodrive a master arduino

#include <Arduino.h>
#include <MgsModbus.h> 
#include <Ethernet.h>
#include <SPI.h>

//variables para el tiempo
bool tiempo_flag=false; 
unsigned long currentMillis =0; 
unsigned long previousMilis = 0; 
unsigned long interval = 100; 

String mensaje1 = ""; 

long VL_Value = 0; 



long MSB_W_VL; //Tomará ambos byte de MSB 
long LSB_W_VL; //Tomará ambos byte de LSB 

long MSB_W_VBUS; //Tomará ambos byte de MSB 
long LSB_W_VBUS; //Tomará ambos byte de LSB 


byte STATUS_drv; 
byte STATUS_stop;
byte Temp_DRV;  

bool start = false; 


void tiempo();
void Poller(); // TODO:  se debe de definir función antes 
//void revisar_num();
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

if (!start){
    Poller(); 
    Mb.MbmRun(); 
}

if(start){
long result1= organizar_w(MSB_W_VL, LSB_W_VL);
long result2 = organizar_w(MSB_W_VBUS, LSB_W_VBUS); 
mensaje1 = String(result1) + "Z" + String(STATUS_drv) + "Y"  + String(STATUS_stop) + "X" + String(result2) + "W" + String(Temp_DRV) + "V" + "\n"; 
Serial.print(mensaje1);
tiempo(); 
start = false; 
}
}

void Poller(){
  
  delay(1);
  acc = acc + 1; 

  //Reading holding register 
  if(acc ==120){ //VELOCIDAD DE MODBUS METE RUIDO AL  LEER DATOS REGISTROS 300 + 200 ok , 
    Mb.MbData[0] = 0; //limpio array de datos 
    Mb.MbData[1] = 0;
    Mb.Req(MB_FC_READ_REGISTERS,  858,4,0); //leer velocidad MSB , 856,857,* 858,859 *
  }

  if(acc ==  240){
    MSB_W_VL = Mb.MbData[0]; // !  Parte alta del num 
    LSB_W_VL = Mb.MbData[1];
    }

    if(acc == 360){
    Mb.MbData[0] = 0; //limpio array de datos 
    Mb.MbData[1] = 0; //limpio array de datos 
    Mb.Req(MB_FC_READ_REGISTERS,  221,1,0); //leer status activación drive
  }

    if(acc == 480){
    STATUS_drv = Mb.MbData[0]; //! Parte baja del numero
    //acc =0; 
    //start = true; 
    }
    
    
    if(acc == 600){
    Mb.MbData[0] = 0; //limpio array de datos 
    Mb.Req(MB_FC_READ_REGISTERS,  1055,1,0); //leer status activación drive
  }

    if(acc == 720){
    STATUS_stop = Mb.MbData[0]; //! Parte baja del numero
    //acc =0; 
    //start = true; }
    }
    
    if(acc == 840){
    Mb.MbData[0] = 0; //limpio array de datos 
    Mb.Req(MB_FC_READ_REGISTERS,  806,4,0); //leer velocidad MSB , 856,857,* 858,859 *
  }

  if(acc == 960){
    MSB_W_VBUS = Mb.MbData[0]; // !  Parte alta del num 
    LSB_W_VBUS = Mb.MbData[1];
    }

  if(acc == 1080){
    Mb.MbData[0] = 0; //limpio array de datos 
    Mb.MbData[1] = 0;
    Mb.Req(MB_FC_READ_REGISTERS,  1749,1,0); //leer velocidad LSB
  }

  if(acc == 1200){
    Temp_DRV = Mb.MbData[0]; //! Parte baja del numero
    acc =0; 
    start = true; 
    }

    
    
    
} //Poller


long organizar_w(long a, long b){
long num2 =0; 
num2 = a << 16;
num2 = num2 | b;
return num2; 
//VL_Value = (VL_Value * 245735)/42949672995; 
}

void tiempo(){ //función de tiempo
  currentMillis = millis(); //obtengo tiempo actual
  previousMilis=currentMillis; //se lo paso a previous ya que previous no debe de ser 0
 
  do{
  if(currentMillis - previousMilis  >= interval ){
    previousMilis = currentMillis;
    tiempo_flag = true; // bandera el cual hace salir del while
    //emergencyDetected=true; //bandera de estado de emergencia 
    }
    currentMillis = millis();
    }while(!tiempo_flag); //sale si se hace presente la bandera de timeover o señal de paro
     
  delay(50);
  tiempo_flag=false; //resetea bandera flag de tiempo  
  }