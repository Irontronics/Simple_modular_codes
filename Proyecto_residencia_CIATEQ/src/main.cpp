#include <Arduino.h>
#include <MgsModbus.h> 
#include <Ethernet.h>
#include <SPI.h>
#include <RotaryEncoder.h>

#define  CH1 7  //definir salida control  CH1 para SSR 
#define  CH2 6  //definir salida control  CH2 para SSR 

//Pines de interrupción a usar 
RotaryEncoder encoder(A13, A14);



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
bool iniciar_set2 = false;
bool iniciar_set3 = false;
bool stop_axis=false; 
bool stop_move=false;
bool COMANDO_AXIS=false; 
bool COMANDO_MOVE=false; 
bool command_in = false; 

void acondicionar_variables_a_modbus();
void separar_ajustes_modbus(String Str_complete );
void activar_desactivar_potencia(); 
void activar_desactivar_movimiento();

//variables para el tiempo
bool tiempo_flag=false; 
unsigned long currentMillis =0; 
unsigned long previousMilis = 0; 

void tiempo(unsigned int a);

//variable de prueba para dirección modbus 
int test1 = 858; 


//acondicionar mensaje recibido de comandos de menu principal HMI  
byte command = 0; 
String inputString = "";


// !inicio de modbus 
String mensaje1 = "";  // mensaje formado para mandar  de arduino a HMI (valores de monitoreo modbus )
// *variables de llegada de ordenes de VS Studio a Arduino modbus 
bool set_command = false; //Para mandar escrituras a registros modbus de ajustes General 
byte flag = 0;  //La uso para determinar que voy a mandar por modbus 

long MSB_W_VL; //Tomará ambos byte de MSB 
long LSB_W_VL; //Tomará ambos byte de LSB 

byte STATUS_drv; 
byte STATUS_stop;


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

    // Registros directos del microcontrolador para habilitar interrupciones 
  PCICR |= (1 << PCIE2);   
  PCMSK2 |= (1 << PCINT21) | (1 << PCINT22);  

}

//Rutina de servicio a interrupción para los registros de interrupción anteriormente habilitados
ISR(PCINT2_vect) {
  encoder.tick(); 
}



void loop() {

if(command == 1){ //Programa modo generador modbus Servodrive 

  if( !start and !command_in ) { 

    Poller(); 
    Mb.MbmRun(); 
  }


if(start   and !command_in){ //cuando se terminó de tomar lectura a registros modbus, organizar string para mandar a visual studio 
long result1= organizar_w(MSB_W_VL, LSB_W_VL);
mensaje1 = String(result1) + "Z" + String(STATUS_drv) + "Y"  + String(STATUS_stop) + "X" + "\n"; 
Serial.print(mensaje1); //lo mandamos a visual studio para mostrar en HMI 
tiempo(250); //le damos un pequeño tiempo de refresco 
start = false; //flag para volver a tomar lecturas de modbus  
}

if(COMANDO_AXIS){ //Si el comando axis esta activo entonces 
  //Serial.println("hab potencia22"); 
  acc=0;
  activar_desactivar_potencia();
  command_in = false; 
  COMANDO_AXIS = false; 
  tiempo(380);
  acc=0; 
//  Serial.println("hab potencia222222222");
}

if(COMANDO_MOVE){ //Si el comando MOVE esta activo entonces 
  acc=0;
  activar_desactivar_movimiento();
  command_in = false; 
  COMANDO_MOVE = false; 
  tiempo(380);
  acc=0; 

}






}

else if (command == 2) { //en espera de selección 
 
  
  digitalWrite(LED_BUILTIN, HIGH);   
  delay(1100);                     
  digitalWrite(LED_BUILTIN, LOW);    
  delay(1100);
}

else if (command == 3) { // Programa modo motor  
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

      digitalWrite(CH2, HIGH); //settear freno
      tiempo(300);
      digitalWrite(CH2, LOW); //
      command_in= true ;  //no imprime valores   
      start = true; //no se mete a modbus
      command = 1;  //se queda en programa de generador 
      inputString ="";
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
      command_in= true ;  //no imprime valores   
      start = true; //no se mete a modbus 
      inputString ="";
    }
    else if (inputString =="F2"){  // 
    command_in = false;
    inputString ="";
    start = false ;  //Reanuda monitoreo modbus en aplicación 
    //comand_in = false;   
    }
    
    else if (inputString =="G2"){  //activar potencia  axis 
    //Serial.println("hab potencia"); 
    stop_axis = true; //habilitada potencia  
    COMANDO_AXIS = true; //es un comando para el axis 
    command_in  = true; //bandera de flag de comando de control (detiene monitoreo momentaneamente)
     tiempo(300);
    inputString ="";
 
    }

    else if (inputString =="H2"){  //desactivar potencia  axis 
    //Serial.println("des potencia"); 
    stop_axis = false; //desactivada potencia 
    COMANDO_AXIS = true; //es un comando para el axis 
    command_in  = true; //bandera de flag de comando de control 
    tiempo(300);
    inputString ="";

    }

    else if (inputString =="I2"){  //Iniciar movimiento
    stop_move = true; //iniciar movimiento   
    COMANDO_MOVE = true; //es un comando para el move 
    command_in  = true; //bandera de flag de comando de control (detiene monitoreo momentaneamente)
     tiempo(300);
    inputString ="";
 
    }

    else if (inputString =="J2"){  //Detener movimiento  
    stop_move = false; //desactivada potencia 
    COMANDO_MOVE = true; //es un comando para el axis 
    command_in  = true; //bandera de flag de comando de control 
    tiempo(300);
    inputString ="";

    }


    }//primer elsif
    else if (inputString.length() >= 8){ //de lo contrario si cadena recibida es larga (datos de settings modbus)

    separar_ajustes_modbus(inputString); //a separar ajustes 
    acondicionar_variables_a_modbus(); 

  inputString ="";
    }
    else{inputString = "";}



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
    
} //Poller_monitoreo general 
// ? FIN de poller monitoreo modbus 


// ? Inico de poller2  
  void Poller_2(){  //* lectura de configuración actual velocidad y aceleración
  delay(1);
  acc = acc + 1; 

if(stop_axis){ //si vamos a activar axis entonces
if(acc == 350){
    Mb.MbData[0] = 1; 
    Mb.Req(MB_FC_WRITE_MULTIPLE_REGISTERS,  254,1,0); //SM_MOD
}
if(acc == 550){
    Mb.MbData[0] = 0; 
    Mb.Req(MB_FC_WRITE_MULTIPLE_REGISTERS,  254,1,0); //SM_MOD
    acc = 0 ;
    iniciar_set2 = true; 

}

}
else if (!stop_axis){ //si se va a desactivar axis entonces 
if(acc == 350){
    Mb.MbData[0] = 1; 
    Mb.Req(MB_FC_WRITE_MULTIPLE_REGISTERS,  236,1,0); //SM_MOD
}
if(acc == 550){
    Mb.MbData[0] = 0; 
    Mb.Req(MB_FC_WRITE_MULTIPLE_REGISTERS,  236,1,0); //SM_MOD
    acc = 0 ;
    iniciar_set2 = true; 
}

}

} //PolLer2 

// ? Inicio de poller4  
  void Poller_4(){  //* lectura de configuración actual velocidad y aceleración
  delay(1);
  acc = acc + 1; 

if(stop_move){ //si vamos a activar axis entonces
if(acc == 350){
    Mb.MbData[0] = 1; 
    Mb.Req(MB_FC_WRITE_MULTIPLE_REGISTERS,  752,1,0); //SM.MOVE
}
if(acc == 550){
    Mb.MbData[0] = 0; 
    Mb.Req(MB_FC_WRITE_MULTIPLE_REGISTERS,  752,1,0); //SM_MOVE
    acc = 0 ;
    iniciar_set3 = true; 

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
    iniciar_set3 = true; 
}

}

} //PolLer4






void activar_desactivar_potencia(){ //función para activar potencia 
while(!iniciar_set2){ //loop de configuración 
Poller_2();
Mb.MbmRun();
}

iniciar_set2=false; 

}

void activar_desactivar_movimiento(){ //función para activar potencia 
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

void acondicionar_variables_a_modbus(){ //Acondicionar variables y escribir a registros modbus
   num1 = one.toInt(); // SM_MODE DATA 
   num2 = two.toInt();  
   num3 = three.toInt(); 
   num4 = four.toInt(); 
   num5 = five.toInt(); 
   num6 = six.toInt(); 
   num7 = seven.toInt(); 
   num8 = eight.toInt(); 
   acc =0; 
 

while(!iniciar_set){ 
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
    iniciar_set = true; 
    acc=0; 
}
}

