#include<ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <EEPROM.h>

char* ssid = "***";       //“wifi热点名称”            
char* passwd = "***";  //”wifi热点密码”

const char* mqtt_server = "152.136.177.90";//云服务器ip
//自行添加的订阅
const char* pubTopic = "topic/reply";
WiFiClient espClient;
PubSubClient client(espClient);

//储存文件大小
int len = 4096;

unsigned long startMillis = 0;
const long reply_open_duration = 10000;
const long reply_close_duration = 60000;
int reply = HIGH;
boolean isAuto = true;



void initWifiSta()
{
  WiFi.mode(WIFI_STA);         // 设置STA模式
  WiFi.begin(ssid, passwd);   //连接网络
  while (WiFi.status() != WL_CONNECTED) {
         Serial.print(".");
        delay(500);
   }
    Serial.println(WiFi.localIP());  //通过串口打印wemos的IP地址
    Serial.println("连接成功");  //通过串口打印wemos的IP地址
  
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  StaticJsonDocument<200> doc;
  deserializeJson(doc, payload);
  JsonObject root = doc.as<JsonObject>();

  String msg_type = root["msg_type"];
   Serial.println(msg_type == "on");
  if (msg_type == "on") {
    Serial.println(msg_type);
    digitalWrite(D2, LOW);
    client.publish(pubTopic, "{\"msg_type\":\"manual_on\"}");
    isAuto = false;
  }else if(msg_type == "off"){
     Serial.println(msg_type);
     digitalWrite(D2, HIGH);
     client.publish(pubTopic, "{\"msg_type\":\"manual_off\"}");
     isAuto = true;
  }else{
    Serial.println("我是else");
    Serial.println(msg_type);
   
  }

// using C++11 syntax (preferred):
//for (JsonPair kv : root) {
//    Serial.println(kv.key().c_str());
//    Serial.println(kv.value().as<char*>());
//}


}


void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish(pubTopic, "hello world");
      // ... and resubscribe
      client.subscribe("topic/reply_control");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


void setup() {
  Serial.begin(115200);
  EEPROM.begin(len);
  initWifiSta();
  pinMode(D2, OUTPUT); //设置引脚为输出引脚

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  //通电后及启动一次
  unsigned long cMillis = millis();
  startMillis = cMillis;
  digitalWrite(D2, LOW);
  reply = LOW;
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  if(isAuto == false){
    return;
  }
  
  unsigned long cMillis = millis();
  //Serial.println(cMillis);
  if(startMillis > cMillis){
     startMillis = cMillis;
  }
  if(cMillis - startMillis >= reply_open_duration){
    if(reply == LOW){
      digitalWrite(D2, HIGH);
      reply = HIGH;
       client.publish(pubTopic, "{\"msg_type\":\"auto_off\"}");
    }
  }
  
  if(cMillis - startMillis >= reply_close_duration){
    if(reply == HIGH){
      startMillis = cMillis;
      digitalWrite(D2, LOW);
      reply = LOW;
       client.publish(pubTopic, "{\"msg_type\":\"auto_on\"}");
    }
  }
}
