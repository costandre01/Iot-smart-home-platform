#include <WiFi101.h>
#include <ArduinoHttpClient.h>
#include <DHT.h>
#include <NTPClient.h>
#include <WiFiUdp.h> //Pré-instalada com o Arduino IDE
#include <TimeLib.h>
#include <MQUnifiedsensor.h>

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

 
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
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
  calibrarSensor(MQ135, RatioMQ135CleanAir);
}
 
void loop() {
  clienteHTTP.get("https://iot.dei.estg.ipleiria.pt/ti/ti139/Projeto/query/api.php?nome=movimento");
  String resposta = clienteHTTP.responseBody();
  int movimento = resposta.toInt(); // converte para inteiro
  Serial.print("Movimento (int): ");
  Serial.println(movimento);
 
  if (movimento <= 10) {
    digitalWrite(ledPin, HIGH);
    post2API("luz", "Ligado", "2024-05-03 12:32:00");
  } else {
    digitalWrite(ledPin, LOW);
    post2API("luz", "Desligado", "2024-05-03 12:32:00");
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

  // Serial.println("----- MQ-135 Leitura -----");
  // Serial.print("CO2: "); Serial.print(ppmCO2lido); Serial.println(" ppm");
  // Serial.print("NH3 (Amoníaco): "); Serial.print(ppmNH3lido); Serial.println(" ppm");
  // Serial.print("Álcool: "); Serial.print(ppmAlcohollido); Serial.println(" ppm");
  // Serial.print("Benzeno: "); Serial.print(ppmBenzenelido); Serial.println(" ppm");
  // Serial.print("Tolueno/Fumo: "); Serial.print(ppmToluenelido); Serial.println(" ppm");
  // Serial.print("NOx (estimado): "); Serial.print(ppmNOxlido); Serial.println(" ppm");
  // Serial.println("--------------------------\n");






  float temperaturaLida = dht.readTemperature();
  float humidadeLida = dht.readHumidity();

  String temperatura = String(temperaturaLida);
  String humidade = String(humidadeLida);


  String dioxidoCarbono = String(ppmCO2lido);
  String amonio = String(ppmNH3lido);
  String alcool = String(ppmAlcohollido);
  String benzeno = String(ppmBenzenelido);
  String fumo = String(ppmToluenelido);
  String oxidoNitrogenio = String(ppmNOxlido);


  char datahora[20];
  update_time(datahora);
  Serial.print("Data Atual: ");
  Serial.println(datahora);

  String datahoraNew = String(datahora);

  post2API("temperatura", temperatura, "datahoraNew");
  post2API("humidade", humidade, "datahoraNew");

  post2API("dioxidoCarbono", dioxidoCarbono, "datahoraNew");
  post2API("amonio", amonio, "2024-05-03 12:32:00");
  post2API("alcool", alcool, "2024-05-03 12:32:00");
  post2API("benzeno", benzeno, "2024-05-03 12:32:00");
  post2API("fumo", fumo, "2024-05-03 12:32:00");
  post2API("oxidoNitrogenio", oxidoNitrogenio, "2024-05-03 12:32:00");
  

  delay(3000);
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
void update_time(char *datahora){
  clienteNTP.update();
  unsigned long epochTime = clienteNTP.getEpochTime();
  //sprintf(datahora, "%02d-%02d-%02d %02d:%02d:%02d", year(epochTime), month(epochTime), day(epochTime), hour(epochTime), minute(epochTime), second(epochTime));
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