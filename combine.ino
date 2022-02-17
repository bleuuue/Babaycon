#include <ESP8266WiFi.h>
#include <WiFiClient.h> 
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <time.h>

/* Set these to your desired credentials. */
const char *ssid = "MJ";            //  iPhone  /  Reborn  /    MJ
const char *password = "mj0218!!!!";      // 12345678 / 19990510 / mj0218!!!!
 
//Web/Server address to read/write from 
const char *host = "192.168.43.62";
WiFiServer server(80); 
WiFiClient client;

//======================== id, ip ===========================
String id, ip;

//========================== 시간 ============================
String afterD;
String afterT;
String afterS;
String today;

int timezone = 3;
int dst = 0;
int h = 0;
int n = 0;
int m = 0;

String idS;
String startT;
String finishT;
String mod = "19/11/01 21:00";
//======================== 전류측정 ===========================
/*릴레이를 항상 온 시켜주는 코드*/
/*전류측정*/
const int sensorIn = A0;
int mVperAmp = 185; // use 100 for 20A Module and 66 for 30A Module
double Voltage = 0;
double VRMS = 0;
double AmpsRMS = 0;
/*보내야 하는 데이터 저장*/
double mem = 0; //사용한 전력량을 저장하는 코드
int plusCount = 0;
double dummy=0;
double w = 0;

//========================== led ============================
int doToggle = 0;
int ledPin = 2;
int blue = 12;//6
int green = 13;//7
int red = 15;//8


//===========================================================
//                          setup
//===========================================================
 
void setup() {
//========================== wifi ===========================
  delay(1000);
  Serial.begin(250000);
  WiFi.mode(WIFI_OFF);        //Prevents reconnection issue (taking too long to connect)
  delay(1000);
  WiFi.mode(WIFI_STA);        //This line hides the viewing of ESP as wifi hotspot

  
  WiFi.begin(ssid, password);     //Connect to your WiFi router
  Serial.println("");
 
  Serial.print("Connecting");
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
 
  //If connection successful show IP address in serial monitor
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());  //IP address assigned to your ESP

//========================== led ============================
  pinMode(ledPin, OUTPUT);
  pinMode(blue, OUTPUT);
  pinMode(green, OUTPUT);
  pinMode(red, OUTPUT);
  digitalWrite(red,1);//우선 빨간불이 들어온 상태
  digitalWrite(ledPin, 0);

//========================== time ===========================
  configTime(9 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  Serial.println("\nWaiting for time");
  while (!time(nullptr)) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("");
}


//===========================================================
//                           host
//===========================================================
void serv() {
  if (client.connect(host, 5179))
  {
    Serial.print("\n\nConnected to: ");
    Serial.println(host);

    /* Send "connected" to the server so it knows we are ready for data */
    client.println("deviceconnected"); //USE client.println()!!
    Serial.println("Host message: \n");
    
    /* Wait for data for 5 seconds at most before timing out */
    unsigned long timeout = millis();
    while(client.available() == 0)
    {
      if(millis() - timeout > 5000)
      {
        Serial.println("Timeout to server!");
        break;
      }
    }
   }
   else
   {
     client.stop();
   }
}
 

//===========================================================
//                        send ip
//===========================================================
void ipFromArduino() {
  HTTPClient http;    //Declare object of class HTTPClient

  String ADCData,postData;
  int adcvalue=analogRead(A0);  //Read Analog value of LDR
  
  
  http.begin("http://192.168.43.62:8652/MyArduinoSetting");              //Specify request destination
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");    //Specify content-type header
  
  ip = WiFi.localIP().toString();
  
  //int httpCode = http.POST(postData);   //Send the request
  int httpCode = http.POST("equip_ip="+ip+"&equip_id="+id);
  String payload = http.getString();    //Get the response payload
  //http.writeToStream(&Serial);
  
  //Serial.println(httpCode);   //Print HTTP return code
  //Serial.println(payload);    //Print request response payload
 
  http.end();  //Close connection
  
  delay(5000);  //Post Data at every 5 seconds
}


//===========================================================
//                        send info
//===========================================================
void infoFromArduino(String afterS, String sDummy) {
  HTTPClient http;    //Declare object of class HTTPClient

  String ADCData,postData;
  int adcvalue=analogRead(A0);  //Read Analog value of LDR
  
  
  http.begin("http://192.168.43.62:8652/KwhToDatabase");              //Specify request destination
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");    //Specify content-type header
  
  //int httpCode = http.POST(postData);   //Send the request
  int httpCode = http.POST("equip_id="+id+"&curdate="+afterS+"&kwh="+sDummy);
  String payload = http.getString();    //Get the response payload
  //http.writeToStream(&Serial);
  
  //Serial.println(httpCode);   //Print HTTP return code
  Serial.println(payload);    //Print request response payload
 
  http.end();  //Close connection

  //==================== reset ====================
  mem=0;
  dummy=0;
  h=1;
  
  delay(5000);  //Post Data at every 5 seconds
}


//===========================================================
//                        time
//===========================================================
String after(time_t now)
{
  struct tm * timeinfo;
  
  timeinfo = localtime(&now);
  String sendD = avg();
  String afterH;
  String num, day;
  
//=============== day ===============
  afterD = String(timeinfo->tm_year+1900) + "/";

  day = String(timeinfo->tm_mon+1);
  if(day.length()<2)
  {
    day = "0" + day; 
  }
  afterD = afterD + day + "/";

  day = String(timeinfo->tm_mday);
  if(day.length()<2)
  {
    day = "0" + day; 
  }
  afterD = afterD + day;
  //Serial.println(afterD);
  
  
//============== time ===============
  
  num = String(timeinfo->tm_hour);
  if(num.length()<2)
  {
    num = "0" + num;
  }
  afterT = num + ":";

  num = String(timeinfo->tm_min);
  if(num.length()<2)
  {
    num = "0" + num;
  }
  afterT += num;
  //Serial.println(afterT);
  
  today = afterD + " " + afterT;
  
  afterH = String(timeinfo->tm_hour) + String(timeinfo->tm_min) + String(timeinfo->tm_sec);
  //Serial.println(afterH);
  
  int intH = afterH.toInt();

  //afterD = String(timeinfo->tm_year-100) + "/" + String(timeinfo->tm_mon+1) + "/" + String(timeinfo->tm_mday);
  //afterT = String(timeinfo->tm_hour) + ":" + String(timeinfo->tm_min);
  
  if(h==0)
  {
    if( 10<=intH && intH<=110 || 3010<=intH && intH<=3110 || 6010<=intH && intH<=6110 ||
        9010<=intH && intH<=9110 || 12010<=intH && intH<=12110 || 15010<=intH && intH<=15110 ||
        18010<=intH && intH<=18110 || 21010<=intH && intH<=21110)
//    if(1110<=intH && intH<=1210 || 1610<=intH && intH<=1710)
    {
      // 현재 날짜, 시간 출력  
      Serial.println("날짜 : " + afterD);     
      Serial.println("시간 : " + afterT);
      afterS = afterD + " " + afterT;
      Serial.println("after " + afterS + "\n");

      // 전류값 출력
      prt();
      
      // 서버 전송
      infoFromArduino(afterS, sendD);
    }
  }
  
//서버 접속 기록 초기화
  if(h==1)
  {
    if(1210<=intH && intH<=1310 || 31210<=intH && intH<=31310 || 61210<=intH && intH<=61310 ||
       91210<=intH && intH<=91310 || 121210<=intH && intH<=121310 || 151210<=intH && intH<=151310 ||
       181210<=intH && intH<=181310 || 211210<=intH && intH<=211310)
//    if(1310<=intH && intH<=1410 || 1810<=intH && intH<=1910)
    {
      h=0;
    }
  }
}

//===========================================================
//                            avg
//===========================================================
String avg() {
    // put your main code here, to run repeatedly:
    Voltage = getVPP();//전압을 구함
    VRMS = (Voltage/2.0) *0.707; //정확한 값을 측정하기 위해서 
    AmpsRMS = (VRMS * 1000)/mVperAmp;//암페어를 구한다
    w = (AmpsRMS * 230)-12.88;//평균전압 곱하기 암페어 
    if(w>5){
      plusCount=plusCount+1;
      mem = mem + w;
      dummy = mem/(plusCount*1.0);// 누적데이터와 누적카운트를 나눠 평균을 구하고 평균 더미값을 뺀 보낼값 
    }
   
    String sDummy=String(dummy,0);
    return sDummy;
}

//===========================================================
//                          sensor
//===========================================================
float getVPP()
{
  float result;
  
  int readValue;             //value read from the sensor
  int maxValue = 0;          // store max value here

    int minValue = 1024;          // store min value here
  
   uint32_t start_time = millis();
   while((millis()-start_time) < 1000) //sample for 1 Sec
   {
       readValue = analogRead(sensorIn);
       // see if you have a new maxValue
       if (readValue > maxValue) 
       {
           /*record the maximum sensor value*/
           maxValue = readValue;
       }
       if (readValue < minValue) 
       {
           /*record the maximum sensor value*/
           minValue = readValue;
       }
   }
   
   // Subtract min from max
   result = ((maxValue - minValue) * 5.0)/1024.0;
      
   return result;
}


//===========================================================
//                            prt
//===========================================================
void prt(){
    Serial.print("A : ");
    Serial.print(AmpsRMS);
    Serial.print("Amps  /  ");
    
    Serial.print("W : ");
    Serial.print(w);
    Serial.print("W  /  ");
    
    Serial.print("COUNT : ");
    Serial.print(plusCount);
    Serial.print("W  /  ");
    
    Serial.print("memory : ");
    Serial.print(mem);
    Serial.print("W  /  ");
    
    Serial.print("AVG : ");
    Serial.print(dummy);
    Serial.println("W  /  ");
}


//===========================================================
//                           loop
//===========================================================
void loop() {
//================ server ====================
    serv(); //connect server
    
//============ time & electric ===============
    if(id != NULL)
    {
      time_t now = time(nullptr);
      //Serial.println(ctime(&now));
      after(now);
    }
  
//================ server ====================
    String value;
    while(client.available() > 0)
    {
      value = client.readStringUntil('\n');
      Serial.println("value : " + value);   //first, 1|0, schedule
      
//================ get string ================
  
      String idC = client.readStringUntil('\n');  //id
      String s2 = client.readStringUntil('\n');   //start
      String s3 = client.readStringUntil('\n');   //finish
    
//================ id - ip ====================
      if(value.equals("first"))
      {
        id=idC;
        Serial.println("id = " + id);
        if(id != NULL)
        {
          ipFromArduino();
        }
      }
    
//================== led ======================
      if(value.equals("0") || value.equals("1"))
      {
        id=idC;
        Serial.println(id);
        if (value.equals("1")) {
          Serial.println("LED : ON\n");
          digitalWrite(ledPin, 1);
          digitalWrite(green, 1);   //green을 켜라
          digitalWrite(red, 0);   //red는 꺼라
          value = "1";
        } 
        else {
          Serial.println("LED : OFF\n");
          digitalWrite(ledPin, 0);
          digitalWrite(red, 1);   //red를 켜라
          digitalWrite(green, 0);   //그린은 꺼라
          value = "0";
        }
      }
//================== schedule =================         
      if(value.equals("schedule"))
      {
        idS=idC;
        startT = s2;
        finishT = s3;
      }
      client.stop();
    }
    
    if(idS != NULL)
    {
        schedule(startT, finishT);
    }
}

void schedule(String startT, String finishT)
{ 
  if(n == 0)
  {
    if(startT.equals(today)) // 날짜 비교
    {
      Serial.println("\nTime Schedule\n" + startT);
      Serial.println("시간이 되었습니다.");
      Serial.println("기기가 켜집니다.");
      Serial.println("LED : ON");
      digitalWrite(ledPin, 1);
      digitalWrite(green, 1);   //green을 켜라
      digitalWrite(red, 0);   //red는 꺼라
      n = 1;
    }
  }
  if(n == 1)
  {
    if(finishT.equals(today))  // 날짜
    {
      Serial.println("\n" + finishT);
      Serial.println("시간이 되었습니다.");
      Serial.println("기기가 꺼집니다.");
      Serial.println("LED : OFF");
      digitalWrite(ledPin, 0);
      digitalWrite(red, 1);   //red를 켜라
      digitalWrite(green, 0);   //그린은 꺼라
      n = 0;
    }
  }
}
