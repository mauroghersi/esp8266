#include <EEPROM.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <qrcode.h>
#include <SSD1306.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>


#define EEPROM_SIZE 4 //SIZE EEPROM DEFINE

int address = 0; //ADDRESS QUANTITY CREDIT VARIABLE
int address2 = 8; //ADDRESS CUSTOMER NUMBER VARIABLE
int contador = 1; //COUNTER CREDIT SELECTION VARIABLE
int creditos; //CREDIT QUANTITY VARIABLE
float contadorCliente; //CUSTOMER NUMBER VARIABLE
float valorUnitario = 0.01; //UNIT PRICE VARIABLE
float valorTotal = 0; //TOTAL PRICE VARIABLE

SSD1306  display(0x3c, D6, D7); //OLED DISPLAY ADDRESS (NOT IMPLEMENTED ON THIS CODE)
QRcode qrcode (&display); //QRCODE GENERATOR LIBRARY INITIALIZATION (NOT IMPLEMENTED ON THIS CODE)

const char* item_qrCode; //QRCODE STRING VARIABLE
const char* STATUS_PAGAMENTO; //PAYMENT STATUS VARIABLE

//WIFI SETUP
const char* ssid = "YOUR NETWORK NAME"; //NETWORK NAME
const char* password = "YOUR PASSWORD"; // PASSWORD

//API do MP
const char* api_url = "https://api.mercadopago.com/v1/payments"; //URL OF MERCADO PAGO PAYMENTS
const char* access_token = "ADD YOUR ACCESS TOKEN"; //TOKEN FROM MERCADO PAGO ACCOUNT


LiquidCrystal_I2C lcd(0x27,16,2); //LCD ADDRESS AND TYPE (ADDRESS CAN BE 0x3F) 

void setup() {
  
  Serial.begin(115200); //SERIAL PORT INITIALIZATION
  EEPROM.begin(EEPROM_SIZE); //EEPROM INITIALIZATION
  creditos = EEPROM.read(address); //LOAD CREDITS ON VARIABLE
  contadorCliente = EEPROM.read(address2); //LOAD CURRENT CUSTOMER NUMBER

  lcd.init(); //LCD INITIALIZATION
  lcd.backlight(); //LCD BACKLIGHT ON
  
  //display.init(); //OLED DISPLAY (NOT IMPLEMENTED)
  //display.clear();
  //display.display();

  pinMode(D5, INPUT_PULLUP); //CREDIT SELECTION BUTTON DECLARATION
  pinMode(LED_BUILTIN, OUTPUT); //STATUS LED DECLARATION
  pinMode(D6, INPUT_PULLUP); //PURCHASE BUTTON DECLARATION
  pinMode(D7, INPUT_PULLUP); //USE CREDIT BUTTON DECLARATION

  WiFi.mode(WIFI_STA); //WIFI MODE

  WiFi.begin(ssid, password); //WIFI INITIALIZATION
  Serial.print("Connecting to WiFi..."); //TRY TO CONNECT 
  while (WiFi.status() != WL_CONNECTED) { //WIAT FOR CONNECTION PRINT DOT
    delay(1000);
    Serial.print(".");
  }
  Serial.println("Connected to WiFi"); //IF CONNECTED PRINT TO SERIAL PORT
  
}

//CREDIT SELECTION FUNCTION
void contadorJogos() {
  
  int flagContador = digitalRead(D5); //READ INPUT
  if(flagContador==LOW){ //IF BUTTON PRESSED COUNT + 1
    lcd.clear();
    contador = contador + 1;
    if(contador > 10){
      contador = 1;
    } 
    delay(500);
    Serial.println(contador); //PRINT ON SERIAL PORT CREDIT SELECTION
    Serial.println(contadorCliente);   // PRINT ON SERIAL PORT CURRENT CUSTOMER NUMBER
  }
}

//PURCHASE AND QR CODE GENERATOR FUNCTION
void createQR() {

  WiFiClientSecure client; //WIFI HTTPS CLIENT DECLARATION

  contadorCliente = contadorCliente + 1; //GENERATE NEW CUSTOMER NUMBER AND SAVE ON EEPROM
  EEPROM.write(address2, contadorCliente);
  EEPROM.commit();
  
  client.setInsecure(); //COMMAND FOR SKIP SSL (NO CERTIFICATE)
  client.connect(api_url, 443); //CONNECT TO URL ON PORT 443 (HTTPS)
  HTTPClient http; //DECLARE HTTP CLIENT

  http.begin(client, api_url); //INITIALIZE HTTP CLIENT
  
  http.addHeader("authorization", String("Bearer ") + access_token); //ADD HEADERS TO AUTORIZATION ON MERCADO PAGO
  http.addHeader("accept", "application/json");
  http.addHeader("Content-Type", "application/json");
  http.addHeader("x-idempotency-key", String(contadorCliente));

  valorTotal = valorUnitario * contador; //CALCULATE THE TOTAL PRICE
  delay(100);
  
  //JSON STRING TO POST WITH TOTAL PRICE AND EMAIL CUSTOMER
  String jsonBody = "{\"transaction_amount\": " + String(valorTotal) + ", \"payment_method_id\": \"pix\", \"payer\": {\"email\": \"TYPE A VALID EMAIL\"}}";
  
  int httpResponseCode = http.POST(jsonBody); //POST JSON TO SERVER

  delay(200);
  String response = http.getString(); //GET RESPONSE FROM SERVER
  int resp1 = http.GET(); //GET HTTP RESPONSE CODE FROM SERVER
  Serial.println(response);  //PRINT RESPONSE
  Serial.println(httpResponseCode); //HTTP RESPONSE CODE PRINT
  Serial.println(resp1);

  JsonDocument filter; //FILTER TO GET QRCODE STRING
  JsonDocument filter1; //FILTER TO GET STATUS PAYMENT
  filter["point_of_interaction"]["transaction_data"]["qr_code"] = true; //FILTER QRCODE DESCRIPTION
  filter1["status"] = true; //FILTER STATUS DESCRIPTION
  
  JsonDocument doc; //JSON DOCUMENT TO GET QRCODE FIELD
  JsonDocument stat; //JSON DOCUEMNT TO GET STATUS FIELD
  deserializeJson(doc, response, DeserializationOption::Filter(filter)); //APPLY FILTER ON JSON DOC 
  deserializeJson(stat, response, DeserializationOption::Filter(filter1)); //APPLY FILTER ON JSON STAT
  serializeJsonPretty(doc, Serial); //GET DOC(QRCODE) ON SERIAL
  serializeJsonPretty(stat, Serial); //GET STAT(STATUS) ON SERIAL

  item_qrCode = doc["point_of_interaction"]["transaction_data"]["qr_code"]; //LOAD QRCODE STRING ON VARIABLE
  STATUS_PAGAMENTO = stat["status"]; //LOAD STATUS PAYMENT STRING ON VARIABLE

  Serial.println(item_qrCode); //PRINT ON SERIAL JUST THE QRCODE WITHOUT BRACKETS
  Serial.println(STATUS_PAGAMENTO); //PRINT ON SERIAL JUST THE STATUS PAYMENT WITHOUT BRACKETS
  Serial.println(contadorCliente);
  
  //qrcode.init(); //QRCODE GENERATOR (NOT IMPLEMENT ON THIS CODE)
  delay(200);
  Serial.println("Aguarde..."); //WAITING FOR THE CUSTOMER PAYMENT
  delay(30000); //DELAY FOR WAITING CUSTOMER PAYMENT


  //INQUIRY FOR PAYMENT STATUS
  client.setInsecure(); //SKIP SSL
  client.connect(api_url, 443);

  http.begin(client, api_url);
  
  http.addHeader("authorization", String("Bearer ") + access_token);
  http.addHeader("accept", "application/json");
  http.addHeader("Content-Type", "application/json");
  http.addHeader("x-idempotency-key", String(contadorCliente));

  //String(contadorCliente)
  valorTotal = valorUnitario*contador;
  
  jsonBody = "{\"transaction_amount\": " + String(valorTotal) + ", \"payment_method_id\": \"pix\", \"payer\": {\"email\": \"TYPE A VALID EMAIL\"}}";
  
  httpResponseCode = http.POST(jsonBody);
  //qrcode.create(item_qrCode);
  response = http.getString();
  JsonDocument stat2;
  deserializeJson(stat2, response, DeserializationOption::Filter(filter1));
  serializeJsonPretty(stat2, Serial);
  STATUS_PAGAMENTO = stat2["status"];
  Serial.println(STATUS_PAGAMENTO);
  ////////////////

  //IF APPROVED GET CREDIT
  if(STATUS_PAGAMENTO == "approved"){
    creditos = creditos + contador;
    STATUS_PAGAMENTO="IDLE";
    lcd.clear();
  }else{
    delay(20000); //WAIT FOR THE PAYMENT ONE MORE TIME IF NOT SUCCESS

    //SECOND INQUIRY FOR PAYMENT STATUS
    client.setInsecure(); //SKIP SSL
    client.connect(api_url, 443);

    http.begin(client, api_url);
  
    http.addHeader("authorization", String("Bearer ") + access_token);
    http.addHeader("accept", "application/json");
    http.addHeader("Content-Type", "application/json");
    http.addHeader("x-idempotency-key", String(contadorCliente));

    //String(contadorCliente)
    valorTotal = valorUnitario*contador;
  
    jsonBody = "{\"transaction_amount\": " + String(valorTotal) + ", \"payment_method_id\": \"pix\", \"payer\": {\"email\": \"TYPE A VALID EMAIL\"}}";
  
    httpResponseCode = http.POST(jsonBody);
    //qrcode.create(item_qrCode);
    response = http.getString();
    JsonDocument stat3;
    deserializeJson(stat3, response, DeserializationOption::Filter(filter1));
    serializeJsonPretty(stat3, Serial);
    STATUS_PAGAMENTO = stat3["status"];
    Serial.println(STATUS_PAGAMENTO);

    //IF APPROVED GET CREDIT
    if(STATUS_PAGAMENTO == "approved"){
      creditos = creditos + contador;
      STATUS_PAGAMENTO="IDLE";
      lcd.clear();
    }    
  }
  Serial.println(creditos); //PRINT NUMBER OF CREDITS ON SERIAL
  STATUS_PAGAMENTO="IDLE"; //SET IDLE TO PAYMENT STATUS VARIABLE AND FINISH THE FUNCTION
}

//FUNCTION TO USE CREDIT
void usarCredito(){
  int flagCredito = digitalRead(D7);
  if(flagCredito == 0){
    
    if(creditos > 0){
      creditos = creditos - 1;
      delay(200);
      digitalWrite(LED_BUILTIN, LOW);
      delay(800);
      digitalWrite(LED_BUILTIN, HIGH);
      lcd.clear();
      EEPROM.write(address, creditos);
      EEPROM.commit();
    }
  }
}

void loop() {

  lcd.setCursor(0,0);
  lcd.print(String("Sel qtd: ") + String(contador));
  lcd.setCursor(0,1);
  lcd.print(String("Creditos: ") + String(creditos));
  contadorJogos();
  //creditosJogos();
  if(WiFi.status()== WL_CONNECTED){ 
    int flag = digitalRead(D6);
    if (STATUS_PAGAMENTO=="pending"){
      
    }else{
      if (flag == 0){  
      digitalWrite(LED_BUILTIN, 0);
      createQR();
      }
      else{
        digitalWrite(LED_BUILTIN, 1);
      } 
    }
  }
  
  usarCredito();
  
}
