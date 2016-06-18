
// CONFIGURAÇÕES INICIAIS
int alt_DBA = 10000;         // Define a altitude limite da determinação do MV em metros (padrão 10.000 m)
int MV_delay1 = 30;          // Define o delay em "SUBIDA" em segundos (padrão 30 segundos)
int MV_delay2 = 30;          // Define o delay em "DESCIDA" em segundos (padrão 30 segundos)
int MV_delay3 = 10;          // Define o delay em "DBA" em segundos (padrão 10 segundos)
int MV_delay4 = 60;          // Define o delay em "SOLO" em segundos (padrão 60 segundos)
int alt_erro = 5;            // Margem de erro da altitude GPS (padrão 5 m)
String ID = "GAMMA";         // ID do time;

// Geral
#include <SoftwareSerial.h>  // Bibliote usada no GPS e no RFD 900 (Disponivel junto ao IDE)
#include <Wire.h>            // Biblioteca usada nos sensores de pressão(Disponivel junto ao IDE) 
#include <Narcoleptic.h>     // Biblioteca usada pra diminuir o consumo de energia do arduino
float MV_alt;                // Altitude anterior
int MV = 4;                  // Define o momento de voo inicial como sendo 4 - Solo


// GPS NEO‐6M
#include "TinyGPS.h"                                 // Importa a biblioteca TinyGPS
SoftwareSerial GPS(2, 3);                            // Define a porta serial do GPS como 2 e 3
TinyGPS gps;                                         // Inicia o GPS
static void print_date(TinyGPS &gps);                
int gps_dados;
int gps_ano, gps_mes, gps_dia; 
int gps_hora, gps_minuto, gps_segundo;               // Variaveis de tempo
int gps_sat;                                         // Numero de Satelites conectados
float gps_lat, gps_lon, gps_alt, gps_vel, gps_curso; // Dados de latitude, longitude, altitude, velocidade(km/h) e curso;

// Sensor de Pressão MS5607B
#include "IntersemaBaro.h"                     // Biblioteca disponivel em http://learn.parallax.com/KickStart/29124z
Intersema::BaroPressure_MS5607B baro(true);    // Inicia o MS5607B
float ms_temp, ms_pres, ms_alt;                // Dados de temperatura (°C), pressão (Pa) e altitude(m)

// Sensor de Pressão BMP180
#include <Adafruit_BMP085.h>         // Biblioteca disponivel em 
Adafruit_BMP085 bmp180;              // Inicia o BMP180
float bmp_temp, bmp_press, bmp_alt;  // Dados de temperatura (°C), pressão (Pa) e altitude(m)

// Sensor de Temperatura LM35
float temp_int, temp_ext;                                   // Inicia as variaveis 
const int lm35_int = A0;                                    // Pino analogico do sensor de temperatura interno.
const int lm35_ext = A1;                                    // Pino analogico do sensor de temperatura externo.
const float CONST_CELSIUS = 0.4887585532746823069403714565; // Constante de conversão do valor lido para graus celsius ((5/102))

// Sensor de Luminosidade TEMT6000
float temt_lum;                     // Dados de luminosidade externa (lux);
const int temt6000 = A2;            // Nomeio o pino analogico do sensor de luminosidade
const float CONST_LUX = 0.9765625;  // Constante retirada do forum http://forum.arduino.cc/index.php?topic=185158.0

// Sensor de Luminosidade BH1750
#include <BH1750FVI.h>
BH1750FVI bh1750;
float bh_lum; 

// Cartão SD
#include <SPI.h>
#include <SD.h>
File arquivo;

// Modulo de Radio RFD 900 
SoftwareSerial RFD(4, 5);

void setup() {
  Serial.begin(9600);   // Inicia a comunicação Serial.
  GPS.begin(9600);      // Inicia a comunicação com o GPS.
  RFD.begin(9600);      // Inicia a comunicação com o modulo RFD.
  bmp180.begin();       // Inicia a comunicação com o sensor de pressão bmp180
  bh1750.begin();       // Inicia a comunicação com o sensor de luminosidade BH1750
  baro.init();          // Inicia a comunicação com o Barometro.
  
  // Primeira aquisição e armazenamento de dados
  aquisicao();
  armazenamento();
  
}
void loop() {
  aquisicao();
  armazenamento();
  MV = momentovoo();
  comunicacao();

  // Switch case aplica o delay correspondente ao momento de voo
  switch (MV) {
    case 1: // Subida
      delay(MV_delay1 * 1000);
      break;
    case 2: // Descida 
      delay(MV_delay2 * 1000);
      break;
    case 3: // DBA
      delay(MV_delay3 * 1000);
      break;
    case 4: // Solo
      delay(MV_delay4 * 1000);
      break;
  }
}
void aquisicao() {
  // GPS
  gps_dados = GPS.read(); // Lê os dados da porta serial do GPS
  gps.encode(gps_dados); // Faz a tradução do valores
  gps.f_get_position(&gps_lat, &gps_lon);
  gps.crack_datetime(&gps_ano, &gps_mes, &gps_dia, &gps_hora, &gps_minuto, &gps_segundo);
  MV_alt = gps_alt; // Armazena a altitude anterior
  gps_alt = gps.f_altitude();
  gps_curso = gps.f_course();
  gps_vel = gps.f_speed_kmph();
  gps_sat = gps.satellites();

  // Sensor de Pressão MS5607B
  ms_pres = baro.getPressure();
  ms_temp =  baro.getTemperature();
  ms_alt = baro.getHeightMeters();

  // Sensor de Pressão BMP180
  bmp_press = bmp180.readPressure();
  bmp_temp = bmp180.readTemperature();
  bmp_alt = bmp180.readAltitude();

  // Sensor de Temperatura LM35
  temp_int = analogRead(lm35_int) * CONST_CELSIUS; // Le as informações do sensor interno e converte para celcius.   
  temp_ext = analogRead(lm35_ext) * CONST_CELSIUS; // Le as informações do sensor externo e converte para celcius.

  // Sensor de luminosidade TEMT6000
  temt_lum = analogRead(temt6000) * CONST_LUX; // Le e converte os dados de luminosidade

  // Sensor de luminosidade BH1750
  bh_lum = bh1750.readLightLevel();  // Le e converte os dados de luminosidade
  
}

void armazenamento() {
  // Cartão SD
  //   Nessa parte do código os dados serão salvos no cartão SD.
  //   A ordem e a estrutura dos dados salvos ainda não foi definida
  //     pois depende de alguns fatores como por exemplo o numero de digitos
  //     de cada variavel.
  //   A mesma estrutura devera ser usada para o envio dos dados para o
  //     Segmento solo.

  arquivo = SD.open("dados.txt", FILE_WRITE);  // Abre o arquivo em modo de edição
  
  arquivo.print(ID);arquivo.print("; "); // ID do time
  // Data e hora
  arquivo.print(gps_ano); arquivo.print("/");
  arquivo.print(gps_mes); arquivo.print("/");
  arquivo.print(gps_dia); arquivo.print("/");
  arquivo.print(gps_hora); arquivo.print("/");
  arquivo.print(gps_minuto); arquivo.print("/");
  arquivo.print(gps_segundo); arquivo.print("; ");
  // Temperatura
  arquivo.print(temp_int); arquivo.print("; ");   
  arquivo.print(temp_ext); arquivo.print("; ");
  arquivo.print(ms_temp); arquivo.print("; ");
  arquivo.print(bmp_temp); arquivo.print("; ");
  //Luminosidade
  arquivo.print(temt_lum); arquivo.print("; ");
  arquivo.print(bh_lum); arquivo.print("; ");
  //Pressão
  arquivo.print(ms_pres); arquivo.print("; ");
  arquivo.print(bmp_pres); arquivo.print("; ");
  // GPS
  arquivo.print(gps_lat); arquivo.print("; ");
  arquivo.print(gps_lon); arquivo.print("; ");
  arquivo.print(gps_alt); arquivo.println("; ");
  
  arquivo.close();      // Fecha o arquivo 
}

int momentovoo() {
  // Caso a diferença de altitude estaja dentro da faixa de erro (de -erro até + erro):
  if ((gps_alt - MV_alt) => -alt_erro && (gps_alt - MV_alt) = < alt_erro) {
    return 4; // Solo
  }
  // Caso a altitude atual seja menor de a altitude limite de DBA e o cansat esteaja descendo:
  if ( gps_alt < alt_DBA && (gps_alt - MV_alt) < -alt_erro) {
    return 3; // DBA
  }
  // Caso a altitude atual menos a altitude anterior seja menor que o erro:
  if ((gps_alt - MV_alt) < -alt_erro) {
    return 2; // Descida
  }
  // Caso a altitude atual menos a altitude anterior seja maior que o erro:
  if ((gps_alt - MV_alt) > alt_erro {
    return 1; // Subida
  }
}
void comunicacao() {
  RFD.print("<"); // Começo dos dados
  RFD.print(ID);RFD.print("; "); // ID do time
  // Data e hora
  RFD.print(gps_ano); RFD.print("/");
  RFD.print(gps_mes); RFD.print("/");
  RFD.print(gps_dia); RFD.print("/");
  RFD.print(gps_hora); RFD.print("/");
  RFD.print(gps_minuto); RFD.print("/");
  RFD.print(gps_segundo); RFD.print("; ");
  // Temperatura
  RFD.print(temp_int); RFD.print("; ");   
  RFD.print(temp_ext); RFD.print("; ");
  //Luminosidade
  RFD.print(temt_lum); RFD.print("; ");
  //Pressão
  RFD.print(ms_pres); RFD.print("; ");
  // GPS
  RFD.print(gps_lat); RFD.print("; ");
  RFD.print(gps_lon); RFD.print("; ");
  RFD.print(gps_alt); RFD.print("; ");
  
  RFD.println(">");  // Termina o envio de dados
}
