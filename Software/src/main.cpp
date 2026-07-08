#include <Arduino.h>
#include <stdio.h>
#include <Wire.h>
#include <esp_timer.h>
#include <TMCStepper.h>


// Definición de constantes
#define LABEL_ON                1
#define LABEL_ON_TXRX_EXTERIOR  1
#define LABEL_ON_TXRX_LIDAR     1
#define LABEL_ON_MOTOR          1
#define LABEL_ON_TMC2208        1 // Habilita mensajes de bajo nivel de los drivers TMC2208


#define TRUE 1
#define FALSE 0
#define UART0_BPS 115200
#define LONG_BUFFER_RX 15
#define LONG_COMANDOS_RX 15

#define PIN              2        // LED interno (activo LOW)
#define NEMA17           0
#define MICROMOTOR       1
#define HORARIO          0
#define ANTIHORARIO      1
#define TIME_STEP      100       // En microsegundos, es un uint16_t
#define TIME_STEP_MAX 1000       // Tiempo máximo es de 1mseg
#define R_SENSE      0.11f       // Valor del resistor sense (normalmente 0.11Ω)0.11f 
#define UART_TMC_BPS 115200      // Es la velocidad de la UART1   

HardwareSerial TMCSerial(1);                      // Serial1 en ESP32
TMC2208Stepper driverTMC(&TMCSerial, R_SENSE);   // Crea el objeto driver

#define TOFF              5
#define RMS_CURRENT     400      // Es la corriente por defecto en miliamperios
#define RMS_MAX         600      // Es la corriente máxima en miliamperios
#define MICROSTEPS       16      // Divide el paso por 16
#define PWM_AUTOESCALE_ON  1     // Recomendado para StealthChop
#define PWM_AUTOESCALE_OFF 0     //

#define RxLidar         16       // Asignación de pines LIDAR
#define TxLidar         17

#define VIO_TMC2208     19

#define RxMicroMotor    32       // Asignación de pines Micromotor
#define TxMicroMotor    33
#define EnMicroMotor    25  
#define StepMicroMotor  26  
#define DirMicroMotor   27 


#define RxNema17        36       // Asignación de pines Nema17, antiguo GPIO9
#define TxNema17        10  
#define EnNema17        14  
#define StepNema17      13  
#define DirNema17       12  

#define TX_EXT          23
#define RX_EXT          22
// Definición de variables globales
uint16_t contadorHola=0;
/* El buffer comandosRx es una pila FIFO en el primer
campo me dice a qué dispositivo se está refieriendo el 
comando, y en el segundo campo el comando, en el tercer 
o cuarto campo los parámetros*/
uint8_t comandosRx[LONG_COMANDOS_RX][6];
uint8_t indiceComando=0;
uint16_t timeStepNema17=TIME_STEP;
uint16_t timeStepMicromotor=TIME_STEP;


//##################################################################
/* Funciónes generales*/
//##################################################################
void limpiarBuffer(uint8_t* datos){
  uint8_t longitud = sizeof(datos) / sizeof(datos[0]);
  for(uint8_t c=0;c<longitud;c++){
    datos[c]=0x00;
  }
}
void limpiarBufferComandos(void){
  uint8_t cantidadFilas = sizeof(comandosRx) / sizeof(comandosRx[0]);
  uint8_t cantidadColumnas = sizeof(comandosRx[0]) / sizeof(comandosRx[0][0]);  
  for(uint8_t f=0;f<cantidadFilas;f++){
    for(uint8_t c=0;c<cantidadColumnas;c++){
      comandosRx[f][c]=0x00;
    }
  }
}
void popBufferComandos(void){
  uint8_t cantidadFilas = sizeof(comandosRx) / sizeof(comandosRx[0]);
  uint8_t cantidadColumnas = sizeof(comandosRx[0]) / sizeof(comandosRx[0][0]);  
  for(uint8_t f=(cantidadFilas-1);f>0;f--){
    for(uint8_t c=0;c<cantidadColumnas;c++){
      comandosRx[f-1][c]=comandosRx[f][c];
    }
  }
  indiceComando--;   
}
//##################################################################
/* Funciónes para comunicarme con el USB*/
//##################################################################
void limpiarBufferRxUART0(void) {
  while (Serial.available()) {
    Serial.read();  // Vaciar el buffer de recepción
  }
}
void limpiarBufferRxUART2(void) {
  while (Serial2.available()) {
    Serial2.read();  // Vaciar el buffer de recepción
  }
}
/* Lee-espera los flag's  de inicio
   Devuelce 0- si no es le FLAG esperado
            1- si encontró los dos bytes 0x59,0x59*/
uint8_t find_frame_start_Exterior(void){
  if (LABEL_ON_TXRX_EXTERIOR)  Serial.println("Entre en find_frame_start_Exterior ");
  if(Serial.read()==0x59){
      if(Serial.available()){
        if(Serial.read()==0x59){
          if (LABEL_ON_TXRX_EXTERIOR)  Serial.println("TRAMA-Se detecto Flag de inicio");
          return 1; // Sale si detecto los dos byte 0x59
        } else {
          if (LABEL_ON_TXRX_EXTERIOR)  Serial.println("TRAMA-ERROR 2do Flag de inicio erroneo");
          limpiarBufferRxUART0();
          return 0;
        }
      } else {
        if (LABEL_ON_TXRX_EXTERIOR)  Serial.println("TRAMA-ERROR no se detecto el 2do Flag de inicio");
        limpiarBufferRxUART0();
        return 0;
      }
  } else {
    if (LABEL_ON_TXRX_EXTERIOR)  Serial.println("TRAMA-ERROR 1er Flag de inicio erroneo");
    limpiarBufferRxUART0();
    return 0;
  } 
}
/* La trama del exterior tiene el siguiente formato
   Dos bytes de inicio 0x59,0x59
   Un byte de longitud todos, los campos de la trama incluyendo el los flag de inicio y CHECKSUM
   Un byte de dispositivo: 1-LIDAR
                           2_ Motor (Nema17) de torreta (movimiento en XY)
                           3_ Micromotor (movimiento en Z)
                           4_ SW de torreta (movimiento en XY)
                           5_ SW de MIcromotor (movimiento en Z)    
   Varios bytes de datos
   Un byte de CHECKSUM */ 
uint8_t dataRxExterior(void){
  uint8_t index=2;
  uint8_t checksum=0;
  uint8_t data[LONG_BUFFER_RX];   // Inicio el RX de recepción
  limpiarBuffer(data);
  data[0]=0x59;
  data[1]=0x59;
  if(find_frame_start_Exterior()){
    while (Serial.available() && index<LONG_BUFFER_RX) {
      data[index]=Serial.read();  // Leer
      index++;
    }
    if (LABEL_ON_TXRX_EXTERIOR){    // Muestra el encabezado de la trama y su checksum
      Serial.printf("Start 1= %02X \r\n",data[0]);
      Serial.printf("Start 2= %02X  \r\n",data[1]);
      Serial.printf("Longitud= %02X \r\n",data[2]);
      Serial.printf("Dispositivo= %02X  \r\n",data[3]);
      Serial.printf("Comando= %02X  \r\n",data[4]);
      Serial.printf("CHECKSUM= %02X  \r\n",data[index-1]);

    } 
    // Chequeos de trama
    if(index!=data[2]){
      if (LABEL_ON_TXRX_EXTERIOR)  Serial.println("TRAMA-ERROR de longitud");
      return 0;
    }
    for(int c=0;c<(index-1);c++) checksum+=data[c];
    if(checksum!=data[index-1]){
      if (LABEL_ON_TXRX_EXTERIOR){
        Serial.printf("checksum = %02X  \r\n",checksum);
        Serial.println("TRAMA-ERROR de CHECKSUM");
      }  
      return 0;
    }
  }
  // Guardo el comando
 switch (data[3]){                        // Pregunto por el dispositivo
 case 0x01:                               // Si es el LIDAR
  comandosRx[indiceComando][0]=data[3];   // Dispositivo LIDAR
  comandosRx[indiceComando][1]=data[4];   // Número de comando
  switch (data[4]){                       // Pregunto por el comando
    case 0x01:                                // Muestreo simple
      comandosRx[indiceComando][2]=0x00;      // Por ahora no los uso
      comandosRx[indiceComando][3]=0x00;
      comandosRx[indiceComando][5]=0x00;
      comandosRx[indiceComando][6]=0x00;
      break;
    case 0x02:                                // Resetea el LIDAR
      comandosRx[indiceComando][2]=0x00;      // Por ahora no los uso
      comandosRx[indiceComando][3]=0x00;
      comandosRx[indiceComando][5]=0x00;
      comandosRx[indiceComando][6]=0x00;
      break;
    case 0x03:                                // Configuración de muestras por segundo
      comandosRx[indiceComando][2]=data[5];   // Parte baja y alta
      comandosRx[indiceComando][3]=data[6];
      comandosRx[indiceComando][5]=0x00;
      comandosRx[indiceComando][6]=0x00;
      if (LABEL_ON_TXRX_LIDAR) Serial.printf("Las muestras por segundo recibidas son: %d \r\n",(data[6]*0xFF+data[5])); 
      break;
    default:
      if (LABEL_ON_TXRX_EXTERIOR)  Serial.println("Comando no soportado-LIDAR");
      break;
  }
  indiceComando++;
  break;

 case 0x02:
  comandosRx[indiceComando][0]=data[3];   // Dispositivo Motor
  comandosRx[indiceComando][1]=data[4];   // Número de comando
  switch (data[4]){                       // Pregunto por el comando
    case 0x01:                                // Solicitud de estado
      comandosRx[indiceComando][2]=data[5];   // Cuál motor, 0=Nema17, 1=Micromotor
      comandosRx[indiceComando][3]=0x00;
      comandosRx[indiceComando][4]=0x00;
      comandosRx[indiceComando][5]=0x00;
      break;
    case 0x02:                                // Configur0 el time step
      comandosRx[indiceComando][2]=data[5];   // Cuál motor, 0=Nema17, 1=Micromotor
      comandosRx[indiceComando][3]=data[6];   // step_LOW   
      comandosRx[indiceComando][4]=data[7];   // step_HIGH 
      comandosRx[indiceComando][5]=0x00;
      break;  
    case 0x04:                                // Configurar micropasos
      comandosRx[indiceComando][2]=data[5];   // Cuál motor, 0=Nema17, 1=Micromotor
      comandosRx[indiceComando][3]=data[6];   // 1,2,4,8,16,32,64,128,256   
      comandosRx[indiceComando][4]=0x00;
      comandosRx[indiceComando][5]=0x00;
      break;
    case 0x05:                                // Configurar corriente máxima en rms
      comandosRx[indiceComando][2]=data[5];   // Cuál motor, 0=Nema17, 1=Micromotor
      comandosRx[indiceComando][3]=data[6];   // rms_LOW   
      comandosRx[indiceComando][4]=data[7];   // rms_HIGH 
      comandosRx[indiceComando][5]=0x00;
      break;      
    case 0x06:                                // Configurar motor, direcciín y cantidad de pulsos
      comandosRx[indiceComando][2]=data[5];   // Cuál motor, 0=Nema17, 1=Micromotor 
      comandosRx[indiceComando][3]=data[6];   // dirección 0=HORARIO, 1=ANTIHORARIO
      comandosRx[indiceComando][4]=data[7];   // pulsos_LOW
      comandosRx[indiceComando][5]=data[8];   // pulsos_HIGH 
      break;   
    default:
      if (LABEL_ON_TXRX_EXTERIOR)  Serial.println("Comando no soportado-MOTOR");
      break;
  }
  indiceComando++;
  break;
 
 default:
  if (LABEL_ON_TXRX_EXTERIOR)  Serial.println("DISPOSIVO- NO soportado");
  break;
 }
  return 1;
}

// ################### Comnados externos ###########################
/*  Comandos del LIDAR

 Trama vacía de prueba --> 59 59 04 B6        No Hace nada

 Lectura simple LIDAR:
   Se envía --> 0x59,0x59,0x06(Long),0x01(LIDAR),0x01(comando),0xBA
                59 59 06 01 01 BA
   Devuelve --> 59 59 0B 01(LIDAR) 01(comando) Dist_L Dist_h Intems_L Intems_H Temp_L Temp_H CHECKSUM

 Resetear el LIDAR:
   Se envía --> 0x59,0x59,0x06(Long),0x01(LIDAR),0x02(comando),0xBB
                59 59 06 01 02 BB
   Devuelve --> Nada

 Configuración de las muestras por segundo:
   Se envía --> 0x59,0x59,0x08(Long),0x01(LIDAR),0x03(comando),LL,HH,CHECKSUM
                59 59 08 01 03 EE 03 AF   --> 1000 muestras por segundo
                59 59 08 01 03 F6 01 B5   --> 500 muestras por segundo
                59 59 08 01 03 64 00 22   --> 100 muestras por segundo
                59 59 08 01 03 32 00 F0   --> 50 muestras por segundo
   Devuelve --> 59 59 08 01(LIDAR) 03(comando) LL HH CHECKSUM */
//
/* Comando de los motores

  Consulta del estado del motor:
        Se envía --> 0x59,0x59,0x07(Long),0x02(Motor),0x01(comando),
                                                          0x00(selecciona el motor),CHECKSUM
                     0x02(Motor)    --> Indica que es un comando de motor
                     0x01(comando)  --> El comando 0x01 es solicitud de status
                     0x00(selecciona el motor) --> 0=es Nema17, o 1=es el Micromotor
                 Ej: 59 59 07 02 01 00 BC   --> pide el estado del motor Nema17
        Devuelve --> 

  Setear el time step (es el tiempo  en microsegundos que está un uno y cero del seudo PWM):
        Se envía --> 0x59,0x59,0x09(Long),0x02(Motor),0x02(comando),0x00(selecciona el motor),
                                                                         step_L,step_H,CHECKSUM
                 La corriente es recomendable que no supere los 600mA=0x0258
                  Ej: 59 59 09 02 02 00 32 00 F1   -->  50useg para el motor Nema17
                      59 59 09 02 02 00 64 00 23   --> 100useg para el motor Nema17
                      59 59 09 02 02 00 C8 00 87   --> 200useg para el motor Nema17
                      59 59 09 02 02 00 90 01 50   --> 400useg para el motor Nema17
                      59 59 09 02 02 00 58 02 19   --> 600useg para el motor Nema17
        Devuelve --> 

  Setear la canidad de micropasos:
        Se envía --> 0x59,0x59,0x08(Long),0x02(Motor),0x04(comando),0x00(selecciona el motor),
                                                                          micropasos,CHECKSUM
                 Los micropasos aceptados son 1,2,4,8,16,32,64,128,256
                  Ej: 59 59 08 02 04 00 04 C4   -->  4 micropasospara el motor Nema17
                      59 59 08 02 04 00 08 C8   -->  8 micropasospara el motor Nema17
                      59 59 08 02 04 00 10 D0   --> 16 micropasospara el motor Nema17
                      59 59 08 02 04 00 20 E0   --> 32 micropasospara el motor Nema17
                      59 59 08 02 04 00 40 00   --> 64 micropasospara el motor Nema17
        Devuelve --> 

  Setear la corriente máxima rms:
        Se envía --> 0x59,0x59,0x09(Long),0x02(Motor),0x05(comando),0x00(selecciona el motor),
                                                                         rms_L,rms_H,CHECKSUM
                 La corriente es recomendable que no supere los 600mA=0x0258
                  Ej: 59 59 09 02 05 00 32 00 F4   -->  50mA para el motor Nema17
                      59 59 09 02 05 00 64 00 26   --> 100mA para el motor Nema17
                      59 59 09 02 05 00 C8 00 8A   --> 200mA para el motor Nema17
                      59 59 09 02 05 00 90 01 53   --> 400mA para el motor Nema17
                      59 59 09 02 05 00 58 02 1C   --> 600mA para el motor Nema17
        Devuelve --> 

  Elegir el motor, la dirección y cantidad de pulsos:
        Se envía --> 0x59,0x59,0x0A(Long),0x02(Motor),0x06(comando),0x00(selecciona el motor),
                                                   0x00(dirección),pulsos_L,pulsos_H,CHECKSUM
                 El motor Nema17 tiene 200 pulsos por vuelta(1,8°), si los micropasos son 16
                 significa que por vualta hay 3200 pulsos(0,1125°)
                  Ej: 59 59 0A 02 06 00 00 10 00 D4   -->   16 pasos Horario para el motor Nema17
                      59 59 0A 02 06 00 00 20 00 E4   -->   32 pasos Horario para el motor Nema17
                      59 59 0A 02 06 00 00 40 00 04   -->   64 pasos Horario para el motor Nema17
                      59 59 0A 02 06 00 00 64 00 28   -->  100 pasos Horario para el motor Nema17
                      59 59 0A 02 06 00 00 C8 00 8C   -->  200 pasos Horario para el motor Nema17
                      59 59 0A 02 06 00 00 91 01 56   -->  400 pasos Horario para el motor Nema17
                      59 59 0A 02 06 00 00 23 03 EA   -->  800 pasos Horario para el motor Nema17
                      59 59 0A 02 06 00 00 46 06 10   --> 1600 pasos Horario para el motor Nema17
                      59 59 0A 02 06 00 00 69 09 36   --> 2400 pasos Horario para el motor Nema17
                      59 59 0A 02 06 00 00 8C 0C 5C   --> 3200 pasos Horario para el motor Nema17
                      59 59 0A 02 06 00 01 8C 0C 5D   --> 3200 pasos ANTIHorario para el motor Nema17

                      59 59 0A 02 06 00 00 8C 9C EC   --> 3200 pasos Horario para el motor Nema17
        Devuelve --> 
  
 */  
// ##################################################################
/* Función que detecta el inicio de la trama que viene desde
   el LIDAR (TF-Mini-S)*/
// ##################################################################
void resetLidar(void){
  uint8_t data[4]={0x5A, 0x04, 0x02, 0x60}; 
  Serial2.write(data,sizeof(data));
}
/* Sincroniza la trama del TF-Mini 
  Devuelve:
     0 sino encontró el inicio de trama después de 1200useg 
     1 si encontró el inicio de trama*/
uint8_t find_frame_start_TFmini(void) { 
  // Acordaese qu el LIDAR constantemente me está tirando tramas
  int64_t start,end;
  start= esp_timer_get_time();  //Mido tiempo inicial
  while(TRUE){
    end= esp_timer_get_time();  //Mido tiempo final
    if (end-start>=1200) return 0;
    if(Serial2.read()==0x59){
      if(Serial2.read()==0x59){
            return 1;
      }
    }
  }
}
/* Se configura la velocidad de muestreo del LIDAR
   El valor puede ser de 50 a 1000 muestras por segundo
*/
void configBaudRateTFmini(uint16_t valor){
/*  0x5A, 0x06, 0x03, LL, HH,CHECKSUM*/  
  uint8_t data[6]={0x5A, 0x06, 0x03, 0xE8, 0x03,0x00}; 
  uint8_t lsb,msb;
  lsb= uint8_t (valor & 0xFF);
  msb = uint8_t ((valor >> 8) & 0xFF);
  data[3]=lsb;
  data[4]=msb;
  data[5]=(data[0]+data[1]+data[2]+data[3]+data[4])&0xFF;
  if (LABEL_ON_TXRX_LIDAR) Serial.printf("Las muestras por segundo son: %d \r\n",(data[4]*0xFF+data[3])); 
  Serial2.write(data,sizeof(data));
}
/* Lee una trama de datos
   Devuelve:
         0- si hay un error
         1- si todo está bien */
uint8_t dataRxTFmini(uint8_t command, uint16_t* distancia, uint16_t* intensidad, uint16_t* temperatura ){
  uint8_t data[LONG_BUFFER_RX];   // Inicio el RX de recepción
  uint8_t checksum=0;
  limpiarBuffer(data);

  if(find_frame_start_TFmini()){
    if (LABEL_ON_TXRX_LIDAR)  Serial.println("Encontro el inicio de trama del LIDAR");
    switch (command)
    {
    case 0x01:    // Solicita una medición única
      if(Serial2.read(data,7)>=7){
        checksum=(0x59+0x59+data[0]+data[1]+data[2]+data[3]+data[4]+data[5])&0xFF;
        *distancia=data[1]*0xFF+data[0];
        *intensidad=data[3]*0xFF+data[2];
        *temperatura=data[5]*0xFF+data[4];

        if (LABEL_ON_TXRX_LIDAR){    // Muestra el encabezado de la trama y su checksum
          Serial.printf("Start 1= %02X \r\n",0x59);
          Serial.printf("Start 2= %02X  \r\n",0x59);
          Serial.printf("Dist_L= %02X \r\n",data[0]);
          Serial.printf("Dist_H= %02X  \r\n",data[1]);
          Serial.printf("Intens_L= %02X  \r\n",data[2]);
          Serial.printf("Intens_H= %02X  \r\n",data[3]);
          Serial.printf("Temp_L= %02X  \r\n",data[4]);
          Serial.printf("Temp_H= %02X  \r\n",data[5]);
          Serial.printf("CHECKSUM= %02X  \r\n",data[6]);
          Serial.printf("Distancia= %dcm\r\n",*distancia);
          Serial.printf("Intensidad= %d \r\n",*intensidad);
          Serial.printf("Temperatura= %d Centigrados\r\n",*temperatura);
          Serial.printf("checksum calculado= %02X  \r\n",checksum);
        } 
        if(checksum==data[6]) return 1;
      }
      break;
    
    default:
      if (LABEL_ON_TXRX_LIDAR) Serial.println("No se reconoce el Comando del LIDAR"); 
      break;
    }

  } else{
    if (LABEL_ON_TXRX_LIDAR)  Serial.println("NO encontro el inicio de trama del LIDAR");
  };
  return 0;
}
//##################################################################

// ##################################################################
/* Función que manejan los driver's  del motor TMC2208            */
// ##################################################################
/* Función que configura los driver's  TMC2208 de los motores     */
/* La función configDriverMotor (), establece qué Dirver TMC2208
   está conectada a la UART1
   Parámetros
             tipo--> 0 si se configura el motor Nema17
                 --> 1 si se configura el micromotor
   Acotaciones: la velocidad de comunicación con los motores
                es fija 115200bps */
void configDriverMotor (uint8_t motor){
  unsigned char rx,tx,enable, dir,step;
  if(motor==NEMA17){
    if(LABEL_ON_MOTOR) Serial.println("Motor- UART1 conectado al driver del motor Nema17");
    rx= RxNema17;
    tx= TxNema17; 
    enable= EnNema17;  
    step=  StepNema17; 
    dir= DirNema17;
    }
  if(motor==MICROMOTOR){
    if(LABEL_ON_MOTOR) Serial.println("Motor- UART1 conectado al driver del Micromotor");
    rx= RxMicroMotor;
    tx= TxMicroMotor; 
    enable= EnMicroMotor;  
    step=  StepMicroMotor; 
    dir= DirMicroMotor;
   }
  // inicializar el Serial a los pines
  TMCSerial.begin(UART_TMC_BPS,SERIAL_8N1, rx, tx);
  while (!TMCSerial);
  if (LABEL_ON_MOTOR and motor==0)  Serial.println("Se configuró la UART 1 para el motor Nema17");
  if (LABEL_ON_MOTOR and motor==1)  Serial.println("Se configuró la UART 1 para el Micromotor");
  delay(500);
}
/* La función configDriverTMC2208(), configura el driver que está conectada a la UART1
   Por lo tanto para configurar un TMC2208:
        *dentro de esta función primero llamo a la función configDriverMotor() para decir qué 
         driver está conectado a la UART1
         *Luego configuro los parámetros del driver.
*/
void configDriverTMC2208(uint8_t motor, uint8_t toff, uint16_t rms_current,uint8_t microsteps, uint8_t pwm_autoescale){
  configDriverMotor (motor);
  driverTMC.toff(toff);                        // Activa el driver (toff > 0)
  driverTMC.rms_current(rms_current);          // Corriente RMS en mA
  driverTMC.microsteps(microsteps);            // Microstepping
  driverTMC.en_spreadCycle(false);             // ---> Usa modo stealthChop (más silencioso)
  driverTMC.pdn_disable(true);                 // ---> Habilita UART
  driverTMC.pwm_autoscale(pwm_autoescale);     // Recomendado para StealthChop
  driverTMC.I_scale_analog(false);             // ---> Control por registro, no por pin físico
}
/* Configura los micropasos para el motor */
void configMicropasos(uint8_t motor,uint8_t micropasos){
  configDriverMotor (motor);
  driverTMC.microsteps(micropasos);            // Microstepping
}
/* Configura la corriente rms máxima para el motor */
void configRmsMax(uint8_t motor,uint16_t rms_current){
  configDriverMotor (motor);
  driverTMC.rms_current(rms_current);          // Corriente RMS en mA
}
/* Lee el estdo del Driver del motor seleccionado*/
void statusDriver(uint8_t motor){
  configDriverMotor (motor);
  uint32_t drv_status = driverTMC.DRV_STATUS();

  bool otpw  = drv_status & (1 <<  0);  // Pre-aviso sobrecalentamiento
  bool ot    = drv_status & (1 <<  1);  // Sobrecalentamiento real
  bool s2ga  = drv_status & (1 <<  2);  // Corto a GND (motor A)
  bool s2gb  = drv_status & (1 <<  3);  // Corto a GND (motor B)
  bool ola   = drv_status & (1 <<  4);  // Carga abierta (motor A)
  bool olb   = drv_status & (1 <<  5);  // Carga abierta (motor B)
  bool stall = drv_status & (1 <<  6);  // StallGuard (si estuviera activado)
  bool uv_cp = drv_status & (1 <<  7);  // Subvoltaje en driver

  uint8_t temp = (drv_status >> 8) & 0xFF;                // Temperatura aproximada
  uint8_t corriente_actual = (drv_status >> 24) & 0xFF;   // Corriente actual aproximada
  if(LABEL_ON_MOTOR){
    Serial.println("== Estado del TMC2208 ==");
    Serial.print("Temperatura aproximada: "); Serial.print(temp); Serial.println(" grados C");
    Serial.print("Corruente actual aproximada: "); Serial.print(corriente_actual); Serial.println(" mA");
    if (otpw)  Serial.println("⚠️  Preaviso de sobrecalentamiento");
    if (ot)    Serial.println("🔥 Sobrecalentamiento real");
    if (s2ga)  Serial.println("❌ Corto en motor A");
    if (s2gb)  Serial.println("❌ Corto en motor B");
    if (ola)   Serial.println("⚠️  Motor A desconectado");
    if (olb)   Serial.println("⚠️  Motor B desconectado");
    if (stall) Serial.println("🛑 Stall detectado");
    if (uv_cp) Serial.println("⚡ Subvoltaje en alimentación");
  }
  delay(1000);
}
/* Configuro el Time Step de los motores */
void configTimeStep(void){
  uint16_t tempTimeStep=comandosRx[0][4]*0xFF+comandosRx[0][3];
  if(comandosRx[0][2]==NEMA17 && tempTimeStep<=TIME_STEP_MAX){
    timeStepNema17=comandosRx[0][4]*0xFF+comandosRx[0][3];
    if(LABEL_ON_MOTOR) Serial.printf("CONFIG Motor- Nema17 Time Step: %d useg\r\n",tempTimeStep);
    } 
  if(comandosRx[0][2]==NEMA17 && tempTimeStep>TIME_STEP_MAX){
    timeStepNema17=TIME_STEP_MAX;
    if(LABEL_ON_MOTOR) Serial.printf("CONFIG Motor-Nema17- Se limitó el Time Step: %d useg\r\n",TIME_STEP_MAX);
    } 
  if(comandosRx[0][2]==MICROMOTOR && tempTimeStep<=TIME_STEP_MAX){
    timeStepMicromotor=comandosRx[0][4]*0xFF+comandosRx[0][3];
    if(LABEL_ON_MOTOR) Serial.printf("CONFIG Motor- MicroMotor Time Step: %d useg\r\n",tempTimeStep);
    } 
  if(comandosRx[0][2]==MICROMOTOR && tempTimeStep>TIME_STEP_MAX){
    timeStepNema17=TIME_STEP_MAX;
    if(LABEL_ON_MOTOR) Serial.printf("CONFIG Motor-Micromotor- Se limitó el Time Step: %d useg\r\n",TIME_STEP_MAX);
    } 
}
void pasosMotor(uint8_t motor,uint8_t direccion,uint16_t cantidadPulsos){ 
  if(motor==NEMA17){
    if(LABEL_ON_MOTOR) Serial.println("Se acciona el motor NEMA17" );
    digitalWrite(DirNema17, direccion);
    digitalWrite(EnNema17, LOW);                // habilito el driver
    for(uint16_t conta=0;conta<cantidadPulsos;conta++){
      digitalWrite(StepNema17, HIGH);
      delayMicroseconds(timeStepNema17);
      digitalWrite(StepNema17, LOW);
      delayMicroseconds(timeStepNema17);
    }
  }
  if(motor==MICROMOTOR){
    if(LABEL_ON_MOTOR) Serial.println("Se acciona el motor MICROMOTOR" );
    digitalWrite(DirMicroMotor, direccion);
    digitalWrite(EnMicroMotor, LOW);                // habilito el driver
    for(uint16_t conta=0;conta<cantidadPulsos;conta++){
      digitalWrite(StepMicroMotor, HIGH);
      delayMicroseconds(timeStepMicromotor);
      digitalWrite(StepMicroMotor, LOW);
      delayMicroseconds(timeStepMicromotor);
    }
  }  
}
//###############  Funciones de bajo nivel #########################
/* Leer el registro CHOPCONF del TMC 2208 (dirección 0x6C)
   posee los siguientes campos:
             Campo       Bits             Función
      MRES	            24–28	  Resolución de micropasos (de 1 a 256 microsteps)
      INTPOL	           28	    Interpolación a 256 microsteps
      TPFD, TOFF,HSTRT	varios	Parámetros del chopper para ajuste fino
      CHM	               14     Modo chopper (spreadCycle vs constant off-time)
      TBL	              15–16	  Configura el tiempo de blanking
      DISS2G, DISS2VS	  30–31	  Desactiva protecciones (cuidado)
Se envía la trama: [0x07] [registro en este caso 0x6C] [CRC]
                  07 6C 6B
El driver responde: 0xFF 0x6C d0 d1 d2 d3 CRC
                    0xFF: start byte
                    0x6C: dirección del registro
                    d0–d3: contenido del registro CHOPCONF, little endian
                    CRC: checksum

   */
/* Sincroniza la trama del TMC2208
  Devuelve:
     0 sino encontró el inicio de trama después de 100useg 
     1 si encontró el inicio de trama*/
uint8_t find_frame_start_TMC2208(void) { 
  // Acordaese qu el LIDAR constantemente me está tirando tramas
  int64_t start,end;
  start= esp_timer_get_time();  //Mido tiempo inicial
  while(TRUE){
    end= esp_timer_get_time();  //Mido tiempo final
    if (end-start>=1000) return 0;
    if(TMCSerial.read()==0x05) return 1;
      }
  }
uint32_t readChopConfTMC2208(void){
  uint8_t start,header,addr,d0,d1,d2,d3,crc,checksum;
  //uint8_t read_CHOPCONF[] = {0x05,0x00, 0xED, 0x87};
  uint8_t read_CHOPCONF[] = {0x05,0x00, 0x6F, 0x84};
  TMCSerial.write(read_CHOPCONF,4);     // Consulto registro
  delay(100);
  if(find_frame_start_TMC2208()&& TMCSerial.available() >= 7){
        start  =0x05; 
        header = Serial.read();   // Start frame, tendría que ser 0XFF
        addr   = Serial.read();   // 0X6f ( tendría que ser 0x6C)
        d0     = Serial.read();
        d1     = Serial.read();
        d2     = Serial.read();
        d3     = Serial.read();
        crc    = Serial.read();
        checksum=start ^ header ^ addr ^ d0^ d1^ d2^ d3;
        if(checksum==crc){
          if(LABEL_ON_TMC2208) Serial.println("TRAMA TMC2208- OK");
          return  d0 | (d1 << 8) | (d2 << 16) | (d3 << 24);
        } else{
          if(LABEL_ON_TMC2208) Serial.println("TRAMA TMC2208- CHECKSUM ERROR");
          return 0;
        }
    } else {
      if(LABEL_ON_TMC2208) Serial.println("TRAMA TMC2208- NO se encotro Start Frame o cantidad de datos < 7");
      return 0;
    }  
}  
void statusTMC2208(uint8_t motor){
  configDriverMotor (motor);
  uint32_t drv_status =readChopConfTMC2208();

  bool otpw  = drv_status & (1 <<  0);  // Pre-aviso sobrecalentamiento
  bool ot    = drv_status & (1 <<  1);  // Sobrecalentamiento real
  bool s2ga  = drv_status & (1 <<  2);  // Corto a GND (motor A)
  bool s2gb  = drv_status & (1 <<  3);  // Corto a GND (motor B)
  bool ola   = drv_status & (1 <<  4);  // Carga abierta (motor A)
  bool olb   = drv_status & (1 <<  5);  // Carga abierta (motor B)
  bool stall = drv_status & (1 <<  6);  // StallGuard (si estuviera activado)
  bool uv_cp = drv_status & (1 <<  7);  // Subvoltaje en driver

  uint8_t temp = (drv_status >> 8) & 0xFF;  // Temperatura aproximada
  uint8_t corriente_actual = (drv_status >> 24) & 0xFF;  // Corriente actual aproximada
  if(LABEL_ON_TMC2208){
    Serial.println("== Estado del TMC2208 ==");
    Serial.print("Temperatura aproximada: "); Serial.print(temp); Serial.println(" grados C");
    Serial.print("Corruente actual aprox.: "); Serial.print(corriente_actual); Serial.println(" mA");
    if (otpw)  Serial.println("⚠️  Preaviso de sobrecalentamiento");
    if (ot)    Serial.println("🔥 Sobrecalentamiento real");
    if (s2ga)  Serial.println("❌ Corto en motor A");
    if (s2gb)  Serial.println("❌ Corto en motor B");
    if (ola)   Serial.println("⚠️  Motor A desconectado");
    if (olb)   Serial.println("⚠️  Motor B desconectado");
    if (stall) Serial.println("🛑 Stall detectado");
    if (uv_cp) Serial.println("⚡ Subvoltaje en alimentación");
  }
  delay(1000);
}

// Configuración inicial
void setup() {
  // Todo esto se hace para entrar en modo UART los TMC2208
  pinMode(VIO_TMC2208,OUTPUT);     // El VIO_TMC2208 alimenta el Módulo TMC2208
  digitalWrite(VIO_TMC2208, LOW);
  delay(100);                      // Espero 100mseg   
  pinMode(RxNema17,OUTPUT);        // Pongo el PDN_UART en cero
  digitalWrite(RxNema17, LOW);
  delay(100);                      // Espero 100mseg 
  pinMode(VIO_TMC2208,OUTPUT);     // Alimento el Módulo TMC2208
  digitalWrite(VIO_TMC2208, HIGH); // El módulo TMC consume como máximo 4mA
                                   // y el ESP 32 por GPIO puede entregar
                                   // hasta 12mA.
  
  // configuración de pines generales
  pinMode(PIN, OUTPUT);
  // configuración de pines para el micromotor
  pinMode(EnMicroMotor, OUTPUT);
  pinMode(StepMicroMotor, OUTPUT);
  pinMode(DirMicroMotor, OUTPUT);
  // configuración de pines para el motor Nema17
  pinMode(EnNema17, OUTPUT);
  pinMode(StepNema17, OUTPUT);
  pinMode(DirNema17, OUTPUT);

  digitalWrite(EnNema17, LOW);                // habilito el driver
  digitalWrite(EnMicroMotor,LOW);                // habilito el driver


  //Limpio el buffer de comandos
  limpiarBufferComandos();

  Serial.begin(UART0_BPS);                              // Inicio UART0 del USB
  while (!Serial);                                      // Espero que el serie esté ok
  delay(100);
  Serial.println("Iniciando...");
  if (LABEL_ON_TXRX_EXTERIOR)  Serial.println("Iniciando UART's");
  if (LABEL_ON_TXRX_EXTERIOR)  Serial.println("UART USB iniciada");
  Serial2.begin(115200, SERIAL_8N1, RxLidar, TxLidar);  // Inicio UART2 del LIDAR
  while (!Serial2);                                     // Espero que el serie esté ok
  if (LABEL_ON_TXRX_LIDAR)  Serial.println("UART para TF-Mini-S iniciado");
  configBaudRateTFmini(1003);                           // Se confirgura 1000 muestras por segundo
  // Configuro el driver y el motor por defecto
  // Configuro la UART1 para conectarme al driver el motor Nema17
  configDriverTMC2208(NEMA17,TOFF,RMS_CURRENT,MICROSTEPS, PWM_AUTOESCALE_ON);
}

void loop() { 
  uint16_t distancia,intensidad,temperatura;
  uint8_t datoTF[12];

  // Espero dato desde el exterior(PC o Placa Master)
  if(Serial.available()) dataRxExterior();
  // Verifico si hay un comando a procesar
  if(indiceComando!=0){
    switch (comandosRx[0][0]){
      case 0x01:                    // Pregunto si es el LIDAR
      limpiarBufferRxUART2();
      switch (comandosRx[0][1]){    //Proceso el comando
      case 0x01:                  // Comando 01 del LIDAR
        if (LABEL_ON_TXRX_LIDAR)  Serial.printf("Dispositivo:%02X Comando:%02X  \r\n",comandosRx[0][0],comandosRx[0][1]);
        // Ejecuto el comando e informo a TF-Mini que se lo llama por el comando 0x01
        dataRxTFmini(0x01,&distancia,&intensidad,&temperatura);
        datoTF[0]=0x59;                       // Encabezado
        datoTF[1]=0x59;
        datoTF[2]=0x0B;                       // Longitud
        datoTF[3]=0x01;                       // Del LIDAR
        datoTF[4]=0x01;                       // Comando
        datoTF[5]= (uint8_t) (distancia & 0xFF);
        datoTF[6]=(uint8_t) (distancia>>8)&0xFF;
        datoTF[7]=(uint8_t)(intensidad & 0xFF);
        datoTF[8]=(uint8_t) (intensidad>>8)&0xFF;
        datoTF[9]= (uint8_t) (temperatura & 0xFF);
        datoTF[10]=(uint8_t) (temperatura>>8)&0xFF;
        datoTF[11]=(datoTF[0]+datoTF[1]+datoTF[2]+datoTF[3]+datoTF[4]+datoTF[5]+datoTF[6]+datoTF[7]+datoTF[8]+datoTF[9]+datoTF[10])&0xFF;
        // Devuelve --> 59 59 0B 01(LIDAR) 01(comando) Dist_L Dist_h Intems_L Intems_H Temp_L Temp_H CHECKSUM
        Serial.write(datoTF,11);
        if (LABEL_ON_TXRX_LIDAR){
          Serial.printf("Distancia= %dcm, Intensidad= %d , Temperatura= %d Centigrados\r\n",distancia,intensidad, temperatura);
          Serial.printf("Distancia= %dcm, Intensidad= %d , Temperatura= %d Centigrados\r\n",datoTF[6]*0xFF+datoTF[5],datoTF[8]*0xFF+datoTF[7], datoTF[10]*0xFF+datoTF[9]);
          Serial.printf("Dist_L= %02X \r\n",datoTF[5]);
          Serial.printf("Dist_H= %02X  \r\n",datoTF[6]);
          Serial.printf("Intens_L= %02X  \r\n",datoTF[7]);
          Serial.printf("Intens_H= %02X  \r\n",datoTF[8]);
          Serial.printf("Temp_L= %02X  \r\n",datoTF[9]);
          Serial.printf("Temp_H= %02X  \r\n",datoTF[10]);
        }  
        break;
      case 0x02:                      // Resetea el LIDAR
        resetLidar();
        if (LABEL_ON_TXRX_LIDAR) Serial.println("Se resetea el LIDAR");
        delay(1000);                  // para darle tiempoa que se estabilice
        configBaudRateTFmini(1003);   // Se confirgura 1000 muestras por segundo
        break;
      case 0x03:                    // Comando 03 del LIDAR, configurar muestras por segundo 
        configBaudRateTFmini(comandosRx[0][3]*0xFF+comandosRx[0][2]); 
        datoTF[0]=0x59;                       // Encabezado
        datoTF[1]=0x59;
        datoTF[2]=0x08;                       // Longitud
        datoTF[3]=0x01;                       // Del LIDAR
        datoTF[4]=0x03;                       // Comando
        datoTF[5]=comandosRx[0][2];
        datoTF[6]=comandosRx[0][3];
        datoTF[7]=(datoTF[0]+datoTF[1]+datoTF[2]+datoTF[3]+datoTF[4]+datoTF[5]+datoTF[6])&0xFF;
        // Devuelve --> 59 59 08 01(LIDAR) 03(comando) LL HH CHECKSUM
        Serial.write(datoTF,8);
        break;

      default:
        if (LABEL_ON_TXRX_LIDAR)  Serial.printf("Dispositivo:%02X Comando:%02X--NO soportado  \r\n",comandosRx[0][0],comandosRx[0][1]);
        break;
      }
      popBufferComandos();          // Actualizo la FIFO comandosRx
      break;

      case 0x02:                    // Pregunto si es para los Motores
      limpiarBufferRxUART2();
      if (LABEL_ON_MOTOR)  Serial.printf("Dispositivo:%02X Comando:%02X  \r\n",comandosRx[0][0],comandosRx[0][1]);
      switch (comandosRx[0][1]){    //Proceso el comando
        case 0x01:                  // Solicitud de estado
          statusDriver(comandosRx[0][2]); 
          break;
        case 0x02:                  // Configuro el time step
          configTimeStep();
          break;
        case 0x04:                  // Configuro micropasos
          if (comandosRx[0][3]==1||comandosRx[0][3]==2||comandosRx[0][3]==4||comandosRx[0][3]==8||comandosRx[0][3]==16
              ||comandosRx[0][3]==32||comandosRx[0][3]==64||comandosRx[0][3]==128||comandosRx[0][3]==246) {
                configMicropasos(comandosRx[0][2],comandosRx[0][3]);
                if(LABEL_ON_MOTOR){
                  Serial.printf("CONFIG Motor- Se configuró para el motor: ");
                  if (comandosRx[0][2]==NEMA17) Serial.printf(" Nema17 ");
                  if (comandosRx[0][2]==MICROMOTOR) Serial.printf(" Micromotor ");
                  Serial.printf("%3d micropasos \r\n",comandosRx[0][3]);
                } 
              } else {
                if(LABEL_ON_MOTOR) Serial.println("CONFIG Motor- Error de Micropasos ");
              }
          break;
        case 0x05:                  // Configuro corriente máxima en rms
          if(comandosRx[0][4]*0xFF+comandosRx[0][3]<=RMS_MAX){
            configRmsMax(comandosRx[0][2],comandosRx[0][4]*0xFF+comandosRx[0][3]);
            if(LABEL_ON_MOTOR) Serial.printf("CONFIG Motor- Irms maxima: %d mA\r\n",comandosRx[0][4]*0xFF+comandosRx[0][3]);
          } else {
            configRmsMax(comandosRx[0][2],RMS_MAX);
            if(LABEL_ON_MOTOR) Serial.printf("CONFIG Motor- Se limito Irms maxima: %d mA\r\n",RMS_MAX);
          }
          break;
        case 0x06:
          pasosMotor(comandosRx[0][2],comandosRx[0][3],comandosRx[0][5]*0xFF+comandosRx[0][4]);
          if(LABEL_ON_MOTOR) Serial.printf("Pasos Motor-: %d pasos\r\n",comandosRx[0][5]*0xFF+comandosRx[0][4]);
          break;

        default:
          if (LABEL_ON_MOTOR)  Serial.printf("Dispositivo:%02X Comando:%02X--NO soportado  \r\n",comandosRx[0][0],comandosRx[0][1]);
          break;       
      }
      popBufferComandos();          // Actualizo la FIFO comandosRx
      break;

    default:
      limpiarBufferRxUART2();
      popBufferComandos();          // Actualizo la FIFO comandosRx
      break;
    }
  }

  digitalWrite(PIN, LOW); // LOW = LED ENCENDIDO
  delay(1000);
  if (LABEL_ON) Serial.printf("%4d-Hola...\r\n",contadorHola);
  digitalWrite(PIN, HIGH); // LOW = LED ENCENDIDO
  delay(1000);
 contadorHola++;
 
}
