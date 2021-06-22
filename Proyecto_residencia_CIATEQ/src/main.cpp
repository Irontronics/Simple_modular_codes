#include <Arduino.h>
#include <MgsModbus.h> 
#include <Ethernet.h>
#include <SPI.h>

#define  CH1 7  //definir salida control  CH1 para SSR 
#define  CH2 6  //definir salida control  CH2 para SSR 

//variables para separar ajustes
bool finish1, finish2, finish3, finish4, finish5, finish6, finish7,finish8;  //estos booleanos 
String one, two, three, four, five, six, seven, eight;  //para el dato individual 
byte num1;
long num2; 
long num3; 
long num4; 
long num5; 
word num6;
word num7; 
word num8; 

bool iniciar_set = false;

void acondicionar_variables_a_modbus();
void separar_ajustes_modbus(String Str_complete );


//variables para el tiempo
bool tiempo_flag=false; 
unsigned long currentMillis =0; 
unsigned long previousMilis = 0; 
//unsigned int interval = 100; 

void tiempo(unsigned int a);

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
word time1_set; 
word time2_set; 
bool dir_set;  
bool op_mode_set; 

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
void Poller_3();
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
 pinMode(CH1, OUTPUT); 
 pinMode(CH2, OUTPUT);
 pinMode(LED_BUILTIN, OUTPUT); //led for testing 
//Serial.println("hola");
  //*initialize ethernet shield , con este activo, es cuando no hace match a veces con el com serial. 
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
   //firstimeGEN y done son iguales, ambas variables son para primera vez, estas no deben de estar activas, entonces start si para iniciar monitoreo normal 
    Poller(); 
    Mb.MbmRun(); 
  }


  if(done){  // Don
     // long result1 = 45193; 
      long result1= organizar_w(MSB_W_VL_Set, LSB_W_VL_Set); //velocidad1 setteada en servo 
      long result2 = organizar_w(MSB_W_VL2_Set, LSB_W_VL2_Set); //velocidad2 setteada en servo
      long result3 = organizar_w(MSB_W_ACC_Set, LSB_W_ACC_Set); //velocidad2 setteada en servo
      long result4 = organizar_w(MSB_W_DEC_Set, LSB_W_DEC_Set); //velocidad2 setteada en servo

      mensaje1 = String(result1) + "M" + String(result2) + "N" + String(result3) + "L" + String(result4) + "K" + String(time1_set) + "J" + String(time2_set) + "I" + String(dir_set) + "H" + String(op_mode_set) + "G" ;
     // mensaje1="123M456N789L2K5J12I0H1G";
      Serial.print(mensaje1); //lo mandamos a visual studio para mostrar en HMI 
      tiempo(100); //le damos un pequeño tiempo de refresco 
      //mensaje1 = ""; 
      firstimeGen = false; //false por que ya no es primera  vez 
      done=false; //reset de variables 
  }

if(start and !firstimeGen and !done and !comand_in){ //cuando se terminó de tomar lectura a registros modbus, organizar string para mandar a visual studio 
long result1= organizar_w(MSB_W_VL, LSB_W_VL);
//long result2 = organizar_w(MSB_W_VBUS, LSB_W_VBUS); 
//mensaje1 = String(result1) + "Z" + String(STATUS_drv) + "Y"  + String(STATUS_stop) + "X" + String(result2) + "W" + String(Temp_DRV) + "V" + "\n"; 
mensaje1 = String(result1) + "Z" + String(STATUS_drv) + "Y"  + String(STATUS_stop) + "X" + "\n"; 
Serial.print(mensaje1); //lo mandamos a visual studio para mostrar en HMI 
//mensaje1 = ""; 
tiempo(250); //le damos un pequeño tiempo de refresco 
start = false; //flag para volver a tomar lecturas de modbus  
}

}

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
      digitalWrite(CH2, HIGH); //settear freno
      tiempo(300);
      digitalWrite(CH2, LOW); //
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
    else if (inputString =="E2"){  // 
      comand_in= true ;  //deja de meterse a la etapa de modbus, no imprime valores   
      inputString ="";
    }
    else if (inputString =="F2"){  // 
    comand_in = false;
    inputString ="";
    start = false ;  //Reanuda monitoreo modbus en aplicación 
    //comand_in = false;   
    }
    

    }//primer elsif
    else if (inputString.length() >= 8){ //de lo contrario si cadena recibida es larga (datos de settings modbus)

    separar_ajustes_modbus(inputString); //a separar ajustes 
    acondicionar_variables_a_modbus(); 
  //digitalWrite(LED_BUILTIN, HIGH);  
  //tiempo(2500);                  
  //digitalWrite(LED_BUILTIN, LOW);   
  inputString ="";
    }



    } //String Complete* 
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
    LSB_W_VL = Mb.MbData[1]; //VELOCIDAD MOTOR 
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
    acc =0; 
    start = true;  //fin 
    }
    
  //  if(acc == 840){
  //  Mb.MbData[0] = 0;  
  //  Mb.MbData[1] = 0; 
  //  Mb.Req(MB_FC_READ_REGISTERS,  806,2,0); //Solicitud de lectura voltaje de potencia 
  //}

  //if(acc == 960){
    //MSB_W_VBUS = Mb.MbData[0]; 
    //LSB_W_VBUS = Mb.MbData[1];
   // }

  //if(acc == 1080){
  //  Mb.MbData[0] = 0; 
  //  Mb.Req(MB_FC_READ_REGISTERS,  1749,1,0); //Solicitud de lectura de temperatura driver 
 // }

  //if(acc == 1200){
   // Temp_DRV = Mb.MbData[0]; 
   // acc =0; 
   // start = true;  //fin 
   
 // }


} //Poller_monitoreo general 
// ? FIN de poller monitoreo modbus 

// ? Inico de poller2  
  void Poller_2(){  //* lectura de configuración actual velocidad y aceleración
  delay(1);
  acc = acc + 1; 

  if(acc ==200){ //!Mete ruido cuando se coloca una velocidad alta (300 + 200) ok , 
    Mb.MbData[0] = 0; 
    Mb.MbData[1] = 0;
    Mb.MbData[2] = 0;
    Mb.MbData[3] = 0;
    Mb.Req(MB_FC_READ_REGISTERS,  758,4,0); 
  }

  if(acc ==  400){ 
    MSB_W_VL_Set = Mb.MbData[0]; //velocidad 1  y velocidad 2 
    LSB_W_VL_Set = Mb.MbData[1];
    MSB_W_VL2_Set = Mb.MbData[2];
    LSB_W_VL2_Set = Mb.MbData[3];

    }

  if(acc ==600){  
    Mb.MbData[0] = 0; 
    Mb.MbData[1] = 0;
    Mb.Req(MB_FC_READ_REGISTERS,  218,2,0); 
  }

  if(acc ==  800){ 
    MSB_W_ACC_Set = Mb.MbData[0]; //Acceleración 
    LSB_W_ACC_Set = Mb.MbData[1];
    }

  if(acc ==1000){  
    Mb.MbData[0] = 0; 
    Mb.MbData[1] = 0;
    Mb.Req(MB_FC_READ_REGISTERS,  232,2,0); 
  }

  if(acc ==  1200){ 
    MSB_W_DEC_Set = Mb.MbData[0]; 
    LSB_W_DEC_Set = Mb.MbData[1];
    //acc =0;
    //done = true; //Desaceleración 
    }

  if(acc ==  1400){ 
  Mb.MbData[0] = 0; 
  Mb.Req(MB_FC_READ_REGISTERS,  755,1,0);
    }

  if(acc ==  1600){ 
    time1_set = Mb.MbData[0]; 
   // acc =0;
   // done = true; //Time1 set  
    }

  if(acc ==  1800){ 
  Mb.MbData[0] = 0; 
  Mb.Req(MB_FC_READ_REGISTERS,  757,1,0);
    }

  if(acc ==  2000){ 
    time2_set = Mb.MbData[0]; 
   // acc =0;
   // done = true; //Time2 set 
    }

  if(acc ==  2200){ 
  Mb.MbData[0] = 0; 
  Mb.Req(MB_FC_READ_REGISTERS,  235,1,0);
    }

  if(acc ==  2400){ 
    dir_set = Mb.MbData[0]; 
   //acc =0;
   //done = true; //Dirección set 
    }

  if(acc ==  2600){ 
  Mb.MbData[0] = 0; 
  Mb.Req(MB_FC_READ_REGISTERS,  751,1,0);
    }

  if(acc ==  2800){ 
    op_mode_set = Mb.MbData[0]; 
   acc =0;
   done = true; //Dirección set 
    }



} //PolLer2 

long organizar_w(long a, long b){
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

void separar_ajustes_modbus(String Str_complete){
  char b; //variable de concatenacion 
  int a = Str_complete.length();   //se determina la longintud de String 
  
  for(int i =0;i <= a;i++){ //hacemos un recorrido por los caracteres del string frame 
    b= Str_complete.charAt(i);  //le damos el valor de dicho caracter a b 

    if(b == 'A' or finish1){ //si se encuentra A, se termina el paquete 1 o si Flag esta activa es por que ya pasó por ahí 
        finish1 = true; //flag de que ya pasó por paquete1 
        if( b == 'B' or finish2){ //si se encuentra B, se termina el paquete 2 o si Flag esta activa es por que ya pasó por ahí 
          finish2 = true; //flag de que ya pasó por paquete2
          if(b == 'C' or finish3){ //si se encuentra C, se termina el paquete 3 o si Flag esta activa es por que ya pasó por ahí 
            finish3 = true; //flag de que ya pasó por paquete3
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

                  } //if septimo paquete
                  else if (b != 'F'){seven += b;}

                } //if sexto paquete
                else if (b!='E'){six += b;}

              } //if quinto paquete
              else if(b!= 'D'){five += b;}
            } //if cuarto paquete
            else if(b!= 'C'){four += b;}
          }//if tercer paquete
          else if(b!= 'B'){three += b;}

        } //if segundo paquete
        else if( b!= 'A'){two += b;}
    
    }//if 1er paquete
    else {one += b;} //primer paquete

} //for
//b = ''; 
Str_complete = ""; 
} //funcion 

void acondicionar_variables_a_modbus(){
   num1 = one.toInt(); // SM_MODE DATA 
   num2 = two.toInt();  
   num3 = three.toInt(); 
   num4 = four.toInt(); 
   num5 = five.toInt(); 
   num6 = six.toInt(); 
   num7 = seven.toInt(); 
   num8 = eight.toInt(); 
   acc =0; 
 

while(!iniciar_set){ //loop de configuración 
    Poller_3(); 
    Mb.MbmRun();
}

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
    iniciar_set = true; 
    acc=0; 
}




}