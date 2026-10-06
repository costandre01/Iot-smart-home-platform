#include <WiFi101.h>
#include <ArduinoHttpClient.h>
#include <DHT.h>
#include <NTPClient.h>
#include <WiFiUdp.h> //Pré-instalada com o Arduino IDE
#include <TimeLib.h>
#include <MQUnifiedsensor.h>
#include <Servo.h>

// --- DHT11 ---
#define DHTPIN 2 // Pin Digital onde está ligado o sensor
#define DHTTYPE DHT11 // Tipo de sensor DHT
DHT dht(DHTPIN, DHTTYPE); // Instanciar e declarar a class DHT
 
char SSID[] = "YOUR_WIFI_SSID";
char PASS_WIFI[] = "YOUR_WIFI_PASSWORD";
char URL[] = "iot.dei.estg.ipleiria.pt";
int PORTO = 80; // ou outro porto que esteja definido no servidor
WiFiClient clienteWifi;
HttpClient clienteHTTP = HttpClient(clienteWifi, URL, PORTO);
String valor = "";

WiFiUDP clienteUDP;
//Servidor de NTP do IPLeiria: ntp.ipleiria.pt
//Fora do IPLeiria servidor: 0.pool.ntp.org
char NTP_SERVER[] = "ntp.ipleiria.pt";
NTPClient clienteNTP(clienteUDP, NTP_SERVER, 3600);
 
const int ledPin = 3;

// --- MQ-135 ---
#define RatioMQ135CleanAir 3.6
#define placa "Arduino UNO"
#define Voltage_Resolution 5
#define ADC_Bit_Resolution 10
#define pin A1
#define type "MQ-135"
MQUnifiedsensor MQ135(placa, Voltage_Resolution, ADC_Bit_Resolution, pin, type);
float ppmCO2lido, ppmNH3lido, ppmAlcohollido, ppmToluenelido, ppmBenzenelido, ppmNOxlido;

Servo servoMotor;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  servoMotor.attach(5);
  servoMotor.write(0);
  delay(1000);  // Dá tempo para o servo reagir

  while (!Serial);   // espera que o Serial Terminal seja inicializado
 
  WiFi.begin(SSID, PASS_WIFI);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
 
  Serial.println((IPAddress)WiFi.localIP());
  Serial.println((IPAddress)WiFi.subnetMask());
  Serial.println((IPAddress)WiFi.gatewayIP());
  Serial.println(WiFi.RSSI());
 
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(ledPin, OUTPUT);

  dht.begin();

  MQ135.setRegressionMethod(1);
  MQ135.init();
  //calibrarSensor(MQ135, RatioMQ135CleanAir);
}
 
void loop() {
  // --- Verificar estado da porta via API ---
  clienteHTTP.get("https://iot.dei.estg.ipleiria.pt/ti/ti139/Projeto/query/api.php?nome=porta");
  String estadoPorta = clienteHTTP.responseBody();
  estadoPorta.trim();

  Serial.print("Estado da Porta: [");
  Serial.print(estadoPorta);
  Serial.println("]");

  // Verifica exatamente o valor da string
  if (estadoPorta.equals("Aberta")) {
    servoMotor.write(180);
    Serial.println("Servo a 90º - Porta Aberta");
  } else if (estadoPorta.equals("Fechada")) {
    servoMotor.write(0);
    Serial.println("Servo a 0º - Porta Fechada");
  } else {
    Serial.println("Valor de estadoPorta inesperado!");
  }

  // --- OBTER estado da luz (manual ou auto)
  clienteHTTP.get("https://iot.dei.estg.ipleiria.pt/ti/ti139/Projeto/query/api.php?nome=luz");
  String estadoLuz = clienteHTTP.responseBody();
  estadoLuz.trim();  // Remover espaços e quebras de linha

  Serial.print("Estado luz (manual): ");
  Serial.println(estadoLuz);

  // Se for "Ligado" ou "Desligado", assume controlo manual
  if (estadoLuz == "Ligado") {
    digitalWrite(ledPin, HIGH);
    Serial.println("Luz ligada manualmente");
  }
  else if (estadoLuz == "Desligado") {
    digitalWrite(ledPin, LOW);
    Serial.println("Luz desligada manualmente");
  }
  else {
    // Caso contrário, usar controlo automático por movimento
    clienteHTTP.get("https://iot.dei.estg.ipleiria.pt/ti/ti139/Projeto/query/api.php?nome=movimento");
    String resposta = clienteHTTP.responseBody();
    int movimento = resposta.toInt();

    Serial.print("Movimento (int): ");
    Serial.println(movimento);

    if (movimento <= 10) {
      digitalWrite(ledPin, HIGH);
      post2API("luz", "Ligado", update_time());
      Serial.println("Luz ligada por movimento");
    } else {
      digitalWrite(ledPin, LOW);
      post2API("luz", "Desligado", update_time());
      Serial.println("Luz desligada por movimento");
    }
  }

  // MQ-135
  MQ135.update();
  MQ135.setA(110.47); MQ135.setB(-2.862); ppmCO2lido = MQ135.readSensor();
  // Amoníaco (NH3)
  MQ135.setA(102.2); MQ135.setB(-2.473); ppmNH3lido = MQ135.readSensor();
  // Álcool
  MQ135.setA(77.255); MQ135.setB(-3.18); ppmAlcohollido = MQ135.readSensor();
  // Tolueno (pode representar também fumo)
  MQ135.setA(44.947); MQ135.setB(-3.445); ppmToluenelido = MQ135.readSensor();
  // Benzeno (mesma curva de Tolueno)
  ppmBenzenelido = ppmToluenelido;
  // Estimativa simplificada para NOx (sem curva precisa, apenas proxy)
  ppmNOxlido = (ppmCO2lido + ppmNH3lido) / 2.0;

  float temperaturaLida = dht.readTemperature();
  float humidadeLida = dht.readHumidity();
  Serial.println(temperaturaLida);

  String temperatura = String(temperaturaLida);
  String humidade = String(humidadeLida);
  String dioxidoCarbono = String(ppmCO2lido);
  String amonio = String(ppmNH3lido);
  String alcool = String(ppmAlcohollido);
  String benzeno = String(ppmBenzenelido);
  String fumo = String(ppmToluenelido);
  String oxidoNitrogenio = String(ppmNOxlido);

  String datahoraNew = update_time();
  Serial.print("Data Atual: ");
  Serial.println(datahoraNew);

  post2API("temperatura", temperatura, datahoraNew);
  post2API("humidade", humidade, datahoraNew);

  post2API("dioxidoCarbono", dioxidoCarbono, datahoraNew);
  post2API("amonio", amonio, datahoraNew);
  post2API("alcool", alcool, datahoraNew);
  post2API("benzeno", benzeno, datahoraNew);
  post2API("fumo", fumo, datahoraNew);
  post2API("oxidoNitrogenio", oxidoNitrogenio, datahoraNew);
  
  delay(1000);
}
 
 
void post2API(String nome, String valor, String hora) {
  String URLPath = "/ti/ti139/Projeto/query/api.php"; //altere o grupo
  String contentType = "application/x-www-form-urlencoded";
 
  // String enviaNome = "temperatura";
  // String enviaValor = "67";
  // String enviaHora = "2024-05-03 12:32:00";
 
  String body = "nome="+nome+"&valor="+valor+"&hora="+hora;
 
 
  clienteHTTP.post(URLPath, contentType, body);
  
  //Enquanto a comunicação estiver ativa (connected), aguarda dados ficarem disponíveis(available)
  while(clienteHTTP.connected()){
    if (clienteHTTP.available()){
      int responseStatusCode = clienteHTTP.responseStatusCode();
      String responseBody = clienteHTTP.responseBody();
      //Serial.println("Status Code: "+String(responseStatusCode)+" Resposta: "+responseBody);
    }
  }
}
String update_time() {
  clienteNTP.update();
  unsigned long epochTime = clienteNTP.getEpochTime();

  char datahora[21];
  sprintf(datahora, "%04d-%02d-%02d %02d:%02d:%02d",
          year(epochTime), month(epochTime), day(epochTime),
          hour(epochTime), minute(epochTime), second(epochTime));

  return String(datahora);
}

// --- Calibração do MQ-135 ---
void calibrarSensor(MQUnifiedsensor &sensor, float ratio) {
  Serial.println("Calibrando... mantenha o sensor em ar limpo");
  float r0 = 0;
  for (int i = 0; i < 100; i++) {
    sensor.update();
    r0 += sensor.calibrate(ratio);
    delay(100);
  }
  sensor.setR0(r0 / 100);
  //Serial.print("R0 encontrado: ");
  //Serial.println(sensor.getR0());
}