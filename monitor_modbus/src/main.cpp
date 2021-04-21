//Monitoreo de señales del servodrive a master arduino
//VL.FB --- Velocidad Servo motor -- Flotante, 64 bits con 32 bits bajos adress 856, solo se podrá leer parte baja 32 bits 

#include <Arduino.h>
#include <MgsModbus.h> 
#include <Ethernet.h>
#include <SPI.h>

long VL_Value = 0; 



long MSB_W_VL; //Tomará ambos byte de MSB 
long LSB_W_VL; //Tomará ambos byte de LSB 

long MSB_W_VBUS; //Tomará ambos byte de MSB 
long LSB_W_VBUS; //Tomará ambos byte de LSB 


byte STATUS_drv; 
byte STATUS_stop;
byte Temp_DRV;  

bool start = true; 



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

if (start){
    Poller(); 
    Mb.MbmRun();
    
}

if (start == false){
  long result1;
result1= organizar_w(MSB_W_VL, LSB_W_VL);
//organizar_w(MSB_W_VBUS, LSB_W_VBUS)
Serial.println(result1, HEX);
Serial.println(STATUS_drv);
Serial.println(STATUS_stop);
Serial.println(Temp_DRV);
result1= organizar_w(MSB_W_VBUS, LSB_W_VBUS);
Serial.println(result1, HEX);
delay(5000);
}

}

void Poller(){
  
  delay(1);
  acc = acc + 1; 

  //Reading holding register 
  if(acc == 3000){
    Mb.MbData[0] = 0; //limpio array de datos 
    Mb.Req(MB_FC_READ_REGISTERS,  858,1,0); //leer velocidad MSB , 856,857,* 858,859 *
  }

  if(acc == 3200){
    MSB_W_VL = Mb.MbData[0]; // !  Parte alta del num 
    }

  if(acc == 4000){
    Mb.MbData[0] = 0; //limpio array de datos 
    Mb.Req(MB_FC_READ_REGISTERS,  859,1,0); //leer velocidad LSB
  }

  if(acc == 4200){
    LSB_W_VL = Mb.MbData[0]; //! Parte baja del numero
    }

    if(acc == 5000){
    Mb.MbData[0] = 0; //limpio array de datos 
    Mb.Req(MB_FC_READ_REGISTERS,  221,1,0); //leer status activación drive
  }

    if(acc == 5200){
    STATUS_drv = Mb.MbData[0]; //! Parte baja del numero
    //acc =0; 
    //start = false; 
    }
    if(acc == 6000){
    Mb.MbData[0] = 0; //limpio array de datos 
    Mb.Req(MB_FC_READ_REGISTERS,  1055,1,0); //leer status activación drive
  }

    if(acc == 6200){
    STATUS_stop = Mb.MbData[0]; //! Parte baja del numero
    //acc =0; 
    //start = false; }
    }
      if(acc == 7000){
    Mb.MbData[0] = 0; //limpio array de datos 
    Mb.Req(MB_FC_READ_REGISTERS,  806,1,0); //leer velocidad MSB , 856,857,* 858,859 *
  }

  if(acc == 7200){
    MSB_W_VBUS = Mb.MbData[0]; // !  Parte alta del num 
    }

  if(acc == 8000){
    Mb.MbData[0] = 0; //limpio array de datos 
    Mb.Req(MB_FC_READ_REGISTERS,  807,1,0); //leer velocidad LSB
  }

  if(acc == 8200){
    LSB_W_VBUS = Mb.MbData[0]; //! Parte baja del numero
    //acc =0; 
    //start = false; 
    }
  if(acc == 9000){
    Mb.MbData[0] = 0; //limpio array de datos 
    Mb.Req(MB_FC_READ_REGISTERS,  1749,1,0); //leer velocidad LSB
  }

  if(acc == 9200){
    Temp_DRV = Mb.MbData[0]; //! Parte baja del numero
    acc =0; 
    start = false; 
    }

    
    
    
}
    
    //Poller


long organizar_w(long a, long b){
long num2 =0; 
num2 = a << 16;
num2 = num2 | b;
return num2; 
//VL_Value = (VL_Value * 245735)/42949672995; 
}