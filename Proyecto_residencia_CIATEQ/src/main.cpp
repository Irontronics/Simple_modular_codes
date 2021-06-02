#include <Arduino.h>
#include <MgsModbus.h> 
#include <Ethernet.h>
#include <SPI.h>

//variables para el tiempo
bool tiempo_flag=false; 
unsigned long currentMillis =0; 
unsigned long previousMilis = 0; 
unsigned int interval = 100; 

//variable de prueba para dirección modbus 
int test1 = 858; 

bool firstimeGen = false; //flag de primer inicio de modo generador  
bool done= false;  //flag de que termino de leer datos modbus 

//acondicionar mensaje recibido de comandos de menu principal HMI  
byte command = 0; 
String inputString = "";


// !inicio de modbus 
String mensaje1 = "";  // mensaje formado para mandar  de arduino a HMI (valores de monitoreo modbus )
// *variables de llegada de ordenes de VS Studio a Arduino modbus 
bool set_command = false; //Para mandar escrituras a registros modbus de ajustes General 
bool comand_in = false; //Se recibe un comando de la interfaz de usuario (VS Studio mando algo a arduino, un comando)
//bool StringComplete; //Flag de que se cachó todo el string proveniente de Vs Studio serial port 
//String inputString = ""; // String de lo que llego del puerto serial 
byte flag = 0;  //La uso para determinar que voy a mandar por modbus 

long MSB_W_VL; //Tomará ambos byte de MSB 
long LSB_W_VL; //Tomará ambos byte de LSB 

long MSB_W_VL_Set; //registro de velocidad seteada 1 
long LSB_W_VL_Set; //registro de velocidad seteada 1
long MSB_W_VL2_Set; //registro de velocidad seteada 2 
long LSB_W_VL2_Set; //registro de velocidad seteada 2
long MSB_W_ACC_Set; //Tomará ambos byte de MSB 
long LSB_W_ACC_Set; //Tomará ambos byte de LSB 
long MSB_W_DEC_Set; //Tomará ambos byte de MSB 
long LSB_W_DEC_Set; //Tomará ambos byte de LSB 

long MSB_W_VBUS; //Tomará ambos byte de MSB 
long LSB_W_VBUS; //Tomará ambos byte de LSB 
byte STATUS_drv; 
byte STATUS_stop;
byte Temp_DRV;  

bool start = false;  //flag para iniciar monitoreo modbus o dar tiempo para mandar comandos 

//de esta linea a Subnet, el consumo de ram se sube a 13.4% de tener 3% checar. 
void tiempo(); //funcion de tiempo, no es de modbus 
void Poller(); // TODO:  se debe de definir función antes 
void Poller_2();
long organizar_w(long a, long b);//funcion de organización word , no es de modbus 

//Creación de objeto Mb
MgsModbus Mb;

int acc = 0; //accumulator  / seconds counter //32,767 number 

// Ethernet settings (depending on MAC and Local network)
byte mac[] = {0x90, 0xA2, 0xDA, 0x0E, 0x94, 0xB5 };
IPAddress ip(192, 168, 0, 192);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);
// ! Fin de modbus 

void setup() {
 Serial.begin(9600);
 pinMode(LED_BUILTIN, OUTPUT); //led for testing 
//Serial.println("hola");
  //*initialize ethernet shield 
  Ethernet.begin(mac, ip, gateway, subnet); 

//*Server - Slave address: //IP SERVO DRIVE
  Mb.remServerIP[0] = 192;
  Mb.remServerIP[1] = 168;
  Mb.remServerIP[2] = 0;
  Mb.remServerIP[3] = 195;
}

void loop() {

if(command == 1){ //Programa modo generador modbus Servodrive 
 //  Serial.println(firstimeGen); 

  if(firstimeGen){ //si es primera vez que ingresa a modo generador 
     Poller_2(); // sacar valores setteados de velocidad y aceleración 
     Mb.MbmRun();
  }
  if(!firstimeGen and !start and !done) { // si no es primera vez y no vamos a organizar datos, entonces (monitoreo general modbus )
   // Serial.println("estoy mon modbus general");
    Poller(); 
    Mb.MbmRun(); 
  }


  if(done){  // Don
      long result1= organizar_w(MSB_W_VL_Set, LSB_W_VL_Set); //velocidad1 setteada en servo 
      long result2 = organizar_w(MSB_W_VL2_Set, LSB_W_VL2_Set); //velocidad2 setteada en servo
      long result3 = organizar_w(MSB_W_ACC_Set, LSB_W_ACC_Set); //velocidad2 setteada en servo
      long result4 = organizar_w(MSB_W_DEC_Set, LSB_W_DEC_Set); //velocidad2 setteada en servo

      mensaje1 = String(result1) + "M" + String(result2) + "N" + String(result3) + "L" + String(result4) + "K" + "\n";
      Serial.print(mensaje1); //lo mandamos a visual studio para mostrar en HMI 
      tiempo(); //le damos un pequeño tiempo de refresco 
      firstimeGen = false; //false por que ya no es primera  vez 
      done=false; //reset de variables 
  }

if(start and (!comand_in) and !firstimeGen and !done){ //cuando se terminó de tomar lectura a registros modbus, organizar string para mandar a visual studio 
long result1= organizar_w(MSB_W_VL, LSB_W_VL);
long result2 = organizar_w(MSB_W_VBUS, LSB_W_VBUS); 
mensaje1 = String(result1) + "Z" + String(STATUS_drv) + "Y"  + String(STATUS_stop) + "X" + String(result2) + "W" + String(Temp_DRV) + "V" + "\n"; 
Serial.print(mensaje1); //lo mandamos a visual studio para mostrar en HMI 
tiempo(); //le damos un pequeño tiempo de refresco 
start = false; //flag para volver a tomar lecturas de modbus  
}

}
//} //fin programa modo motor 

else if (command == 2) { //en espera de selección 
  digitalWrite(LED_BUILTIN, HIGH);   
  delay(1100);                     
  digitalWrite(LED_BUILTIN, LOW);    
  delay(1100);
}

else if (command == 3) { // Programa modo motor  
  digitalWrite(LED_BUILTIN, HIGH);  
  delay(80);                    
  digitalWrite(LED_BUILTIN, LOW);   
  delay(80);
}

}

void serialEvent (){ // lee la cadena proveniente de visual studio HMI 
    bool StringComplete = false;



  while (Serial.available() > 0){

    char inChar = (char)Serial.read();
    if(inChar == '$'){ StringComplete = true;}
    else { inputString += inChar;}
  }

  if(StringComplete){  //validar comando recibido 
    inputString.trim(); 
  //  Serial.println(inputString.length()); //string de lo que leyó 4 Init, 2 A2, etc 
    if(inputString.length() == 4){
     
      if(inputString =="Init"){ //comandos de inicio de HMI a Arduino 
      inputString =""; 
      delay(300);
      Serial.println("123Z"); //comando de respuesta Alive de arduino a HMI 
  
    }

    } else if (inputString.length() == 2){

    if(inputString =="A2"){ //modo standy en espera de selección  
      inputString =""; 
      command = 2; 
    }

    else if (inputString =="B2"){ //Comando para inicializar modo generador //entro a la ventana de modo generador, revisar velocidades y aceleraciones 
      //va a correr el programa de generador, entonces, en primer instancia y solo por una vez, se debe de tomar lectura de registros de velocidad y aceleración, entonces: 
      firstimeGen = true; //primera vez 
      inputString ="";
      command = 1;  
    }
    else if (inputString =="D2"){ //Comando para inicializar modo motor 
      inputString ="";
      command = 3;  
    }
    else if (inputString =="C2"){  // sin activación, se pierde comunicación con software de usuario 
      inputString ="";
      digitalWrite(LED_BUILTIN, LOW);
      command = 0;  
    }
    else if (inputString =="E2"){  // sin activación, se pierde comunicación con software de usuario 
      inputString ="";
      start = true ;  //deja de meterse a la etapa de modbus, pero sigue imprimiendo valores 
      comand_in = true;   
    }
        else if (inputString =="F2"){  // sin activación, se pierde comunicación con software de usuario 
      inputString ="";
      start = false ;  //deja de meterse a la etapa de modbus, pero sigue imprimiendo valores 
      comand_in = false;   
    }

    }
    else{ //de lo contrario si cadena recibida es larga (datos de settings modbus)

    }
    } //validación de cadena recibida*** 
  } //función de dato serial recibido 

// ? Inico de poller monitoreo modbus 
  void Poller(){  //*secuencia de modbus registros de monitoreo: velocidad, voltaje, temperatura, Driver En, emergency Stop 
  delay(1);
  acc = acc + 1; 

  if(acc ==120){ //!Mete ruido cuando se coloca una velocidad alta (300 + 200) ok , 
    Mb.MbData[0] = 0; 
    Mb.MbData[1] = 0;
    Mb.Req(MB_FC_READ_REGISTERS,  test1,2,0); 
  }

  if(acc ==  240){ 
    MSB_W_VL = Mb.MbData[0];
    LSB_W_VL = Mb.MbData[1];
    }

    if(acc == 360){
    Mb.MbData[0] = 0; 
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
    Mb.MbData[1] = 0; 
    Mb.Req(MB_FC_READ_REGISTERS,  806,2,0); //Solicitud de lectura voltaje de potencia 
  }

  if(acc == 960){
    MSB_W_VBUS = Mb.MbData[0]; 
    LSB_W_VBUS = Mb.MbData[1];
    }

  if(acc == 1080){
    Mb.MbData[0] = 0; 
    Mb.Req(MB_FC_READ_REGISTERS,  1749,1,0); //Solicitud de lectura de temperatura driver 
  }

  if(acc == 1200){
    Temp_DRV = Mb.MbData[0]; 
    acc =0; 
    start = true;  //fin 
   
  }


} //Poller_monitoreo general 
// ? FIN de poller monitoreo modbus 

// ? Inico de poller2  
  void Poller_2(){  //* lectura de configuración actual velocidad y aceleración
  delay(1);
  acc = acc + 1; 

  if(acc ==120){ //!Mete ruido cuando se coloca una velocidad alta (300 + 200) ok , 
    Mb.MbData[0] = 0; 
    Mb.MbData[1] = 0;
    Mb.MbData[2] = 0;
    Mb.MbData[3] = 0;
    Mb.Req(MB_FC_READ_REGISTERS,  758,4,0); 
  }

  if(acc ==  240){ 
    MSB_W_VL_Set = Mb.MbData[0]; //velocidad 1 
    LSB_W_VL_Set = Mb.MbData[1];
    MSB_W_VL2_Set = Mb.MbData[2];
    LSB_W_VL2_Set = Mb.MbData[3];
   // acc =0;
   // done = true; //FLAG DE TERMINACIÓN LECTURA REG
    }

  if(acc ==360){  
    Mb.MbData[0] = 0; 
    Mb.MbData[1] = 0;
    Mb.Req(MB_FC_READ_REGISTERS,  218,2,0); 
  }

  if(acc ==  480){ 
    MSB_W_ACC_Set = Mb.MbData[0]; //velocidad 1 
    LSB_W_ACC_Set = Mb.MbData[1];
    }

  if(acc ==600){  
    Mb.MbData[0] = 0; 
    Mb.MbData[1] = 0;
    Mb.Req(MB_FC_READ_REGISTERS,  232,2,0); 
  }

  if(acc ==  720){ 
    MSB_W_DEC_Set = Mb.MbData[0]; //velocidad 1 
    LSB_W_DEC_Set = Mb.MbData[1];
    acc =0;
    done = true; //FLAG DE TERMINACIÓN LECTURA REG
    }


} //PolLer2 

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

