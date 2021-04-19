//Mi primer código de proyecto residencia  en platformio 
//Arduino client, servodrive server Modbus TCP/IP
//Código de extracción de bytes del registro VBUS.VALUE 32 bits address: 806 , 807 flotante (pag. 896)
//       MSB                LSB            :  MSB_WORD    [] LSB_WORD 
// 00000000 00000000[ ] 00000000 00000000  = 32 bits 

#include <Arduino.h>
#include <MgsModbus.h> 
#include <Ethernet.h>
#include <SPI.h>

bool start = true; //Flag de inicio programa prueba 
word MSB_W_VBUS; //Tomará ambos byte de MSB 
word LSB_W_VBUS; //Tomará ambos byte de LSB 

void Poller(); // TODO:  se debe de definir función antes 

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

   if(start){
    Poller(); 
    Mb.MbmRun();
   }

  if(start == false){
  Serial.println(MSB_W_VBUS,HEX);
  Serial.println(LSB_W_VBUS,HEX);
 
  delay(5000);
  }

}

void Poller(){
  
  delay(1);
  acc = acc + 1; 

  //Reading holding register 
  if(acc == 3000){
    Mb.MbData[0] = 0; //limpio array de datos 
    Mb.Req(MB_FC_READ_REGISTERS,  806,1,0); //dirección de registro a leer  +1
  }

  if(acc == 3200){
    MSB_W_VBUS = Mb.MbData[0]; // !  NO HACE FALTA DIVIDIRLO, PUESTO QUE EL TIPO DE VARIABLE ES DE 16 BITS, DEBE DE AGREGAR ESOS DOS BYTES
    }

  if(acc == 4000){
    Mb.MbData[0] = 0; //limpio array de datos 
    Mb.Req(MB_FC_READ_REGISTERS,  807,1,0); //dirección de registro a leer  +1
  }

  if(acc == 4200){
    LSB_W_VBUS = Mb.MbData[0]; //! NO HACE FALTA DIVIDIRLO, PUESTO QUE EL TIPO DE VARIABLE ES DE 16 BITS, DEVE DE AGREGAR ESOS DOS BYTES
    acc = 0;
    start = false; 
    }
   
    }//Poller