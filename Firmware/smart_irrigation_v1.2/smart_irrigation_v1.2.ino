#include <String.h>
#include <stdio.h>
#include <stdlib.h>

#include <SPI.h>
#include <Ethernet.h>
#include <DHT.h>
//------------------------------------------------
#define MOIST_PIN A0
#define DHTPIN 7
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

#define PUMP_PIN 2
//------------------------------------------------
byte mac[] = {0x90, 0xA2, 0xDA, 0x00, 0x4A, 0xE0};

char server_ip[] = "192.168.1.10";

IPAddress ip(192, 168, 1, 177);
EthernetServer server(80);

//------------------------------------------------
int DB_send_interval = 120; //2 min;
long int last_DB_send_interval = 0;
bool settings_changed = false;
bool first_boot = true;
bool change_moist_th = false;


//------------------------------------------------
String HTTP_req = "", webPage_MSB = "" , webPage_LSB = "";

bool pump_off = true;
bool pump_auto = false;
bool pump_man = false;
bool prev_pump_stat = pump_off;
bool pump_auto_moist_db = pump_auto;

int temp = 0, hum = 0, moisture = 0, i = 0;

int moisture_max_th = 100, last_moisture_max_th = moisture_max_th, moist_th_db = moisture_max_th;
//==================================================================================
void setup()
{
  Serial.begin(9600);
  dht.begin();

  pinMode(PUMP_PIN, OUTPUT);

  delay(50);

  digitalWrite(PUMP_PIN, LOW);

  //--------------------------------------------
  Ethernet.begin(mac, ip);
  server.begin();
  Serial.print("Server Started...\nLocal IP: ");
  Serial.println(Ethernet.localIP());

  last_DB_send_interval = millis() / 1000;
}
//==================================================================================
//==================================================================================
void send_to_DB()   //CONNECTING WITH MYSQL
{
  EthernetClient client;
  //Serial.println(client.connect(server_ip, 80));

  if (moist_th_db != 0)
  {
    while (1)
    {
      if (client.connect(server_ip, 80)) {
        Serial.println("connected to DB");
        // Make a HTTP request:
        //http://dev.smartirrigation.com/dht.php?pump_stat=0&moist_thresh=70&temp=40&hum=60&moist=40

        //pump
        client.print("GET /smartgardening/dht.php?pump_stat=");
        Serial.print("GET /smartgardening/dht.php?pump_stat=");

        if (pump_off)
        {
          client.print("0");
        }
        else if (!pump_off)
        {
          client.print("1");
        }

        //moist_thresh
        client.print("&moist_thresh=");
        client.print(moist_th_db);
        Serial.print("&moist_thresh=");
        Serial.print(moist_th_db);

        //temp hum moist
        client.print("&temp=");
        client.print(temp);
        Serial.print("&temp=");
        Serial.print(temp);

        client.print("&hum=");
        client.print(hum);
        Serial.print("&hum=");
        Serial.print(hum);

        client.print("&moist=");
        client.print(moisture);
        Serial.print("&moist=");
        Serial.print(moisture);

        client.print(" ");      //SPACE BEFORE HTTP/1.1
        client.print("HTTP/1.1");
        client.println();
        client.println("Host: 192.168.1.10");
        client.println("Connection: close");
        client.println();

        client.stop();

        break;

      } else {
        // if you didn't get a connection to the server:
        Serial.println("connection failed to DB");
        i++;
      }

      if (i > 5)
        break;
    }
  }
}





void AJAX_request(EthernetClient client)
{
  client.println("<h3>Sensor Status</h3>");
  client.println("<p>Temperature:");
  client.println(temp);
  client.println(" C");
  char temp_c[50];
  String s = "";
  sprintf(temp_c, "<progress value=\"%d\" max=\"100\"></progress>", temp);
  for (int i = 0; i < 43; i++) s += temp_c[i];
  client.println(s);
  s = "";
  client.println("</p>");
  client.println("<p>Humidity &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;:");
  client.println(hum);
  client.println(" %");
  char hum_c[50];
  s = "";
  sprintf(hum_c, "<progress value=\"%d\" max=\"100\"></progress>", hum);
  for (int i = 0; i < 43; i++) s += hum_c[i];
  client.println(s);
  s = "";
  client.println("</p>");
  client.println("<p>Moisture &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;:");
  client.println(moisture);
  client.println(" %");
  char moist_c[50];
  s = "";
  sprintf(moist_c, "<progress value=\"%d\" max=\"100\"></progress>", moisture);
  for (int i = 0; i < 43; i++) s += moist_c[i];
  client.println(s);
  s = "";
  client.println("</p>");

  client.println("<h3>Pump</h3>");
  client.println("<p>Pump Status: <output>");

  if ((pump_man || pump_auto) && !pump_off)
  {
    client.println("ON");
  }
  if (pump_off)
  {
    client.println("OFF");
  }
  client.println("</output></p>");

  client.println("<p>Pump Control: <output>");
  if ((pump_man || pump_off || !pump_off) && !pump_auto)
  {
    client.println("Manual <hr>");
  }
  else if ((pump_auto || pump_off || !pump_off) && !pump_man)
  {
    client.println("Automatic <br>");
  }

  client.println("<p>Current Moisture Threshold: ");
  client.println(moisture_max_th);
  client.println(" %</p>");


  client.println("</output></p>");

}

//==================================================================================
void html_webpage()
{
  //HTTP response header
  webPage_MSB = "HTTP/1.1 200 OK\n\rContent-Type: text/html\n\r\n\r";
  //-------------------------------------------------------------
  webPage_MSB += R"***(
  <style>
  *{
    box-sizing: border-box;
  }
  body {
    font-family: Arial, Helvetica, sans-serif;
  }
  .button {
    border: none;
    color: white;
    padding: 15px 32px;
    text-align: center;
    text-decoration: none;
    display: inline-block;
    font-size: 16px;
    margin: 4px 2px;
    cursor: pointer;
  }
  .button1 {background-color: #4CAF50;}
  .button2 {background-color: #000000;}
  .button3 {background-color: #3e4243; padding: 15px 62px;} 
  .button4 {background-color: #3e4243;}
  header {
    background-color: rgb(0, 0, 0);
    padding: 1px;
    text-align: center;
    font-size: 18px;
    color: rgb(255, 255, 255);
  }
  nav {
    float: left;
    width: 30%;
    height: 700px;
    background: #ccc;
    padding: 20px;
  }
  nav ul {
    list-style-type: none;
    padding: 0;
  }
  article {
    float: left;
    padding: 20px;
    width: 70%;
    background-color: #f1f1f1;
    height: 700px; 
  }
  section::after {
    content: "";
    display: table;
    clear: both;
  }
  footer {
    background-color: #777;
    padding: 10px;
    text-align: center;
    color: white;
  }
  input[type=text] {
    width: 50%;
    padding: 12px 20px;
    margin: 8px 0;
    box-sizing: border-box;
  }
  input[type=submit] {
    width: 50%;
    background-color: #4CAF50;
    padding: 12px 20px;
    margin: 8px 0;
    box-sizing: border-box;
  }
  @media (max-width: 400px) {
    nav, article {
      width: 100%;
      height: auto;
    }
  }
  </style> 
  )***";


  webPage_LSB = R"***(
  <!DOCTYPE html>
  <head>
  <title>Smart Irrigation</title>
  <meta charset="utf-8">
  </head>
  <body>
  
  <header>
    <h2>Smart Irrigation System</h2>
  </header>
  
  <section>
    <nav>
      <ul>
          <button class="button button3">Dashboard</button>
          <br>
          <hr>
          <hr>

      </ul>
    </nav>
    
    <article>
      <h2>My Dashboard</h2>
      
      <span id="DHT11Vals">Updating...</span>
      
      
      <h4>Manual Control</h4>
      <button onmousedown=location.href='/?onpump_on'>Pump ON</button>
      <button onmousedown=location.href='/?offpump_off'>Pump OFF</button>
  
      <h4>Automatic Control</h4>
      <form action="/">
          <label for="lname">Moisture Max Threshold</label><br>
          <input type="text" id="lname" name="lname" value=""><br><br>
          <input type="submit" value="Enable Automatic Control">
        </form>
    </article>
  </section>

  <!-------------------------JavaScript---------------------------->
  <script>
    setInterval(function()
    {
      getDHT11Vals();
    }, 2000);
    function getDHT11Vals()
    {
      var DHT11Request = new XMLHttpRequest();
      DHT11Request.onreadystatechange = function()
      {
        if(this.readyState == 4 && this.status == 200)
        {
          document.getElementById("DHT11Vals").innerHTML = this.responseText;
        }
      };
      DHT11Request.open("GET", "readDHT11", true);
      DHT11Request.send();
    }
    </script>
    
  </body>
  </html>
  )***";
}


void loop()
{
  delay(100);
  temp = dht.readTemperature();
  hum = dht.readHumidity();
  moisture = analogRead(MOIST_PIN);
  moisture = map(moisture, 0, 1023, 99, 0);

  Serial.println("loop");

  if (((millis() / 1000) - last_DB_send_interval > DB_send_interval) || settings_changed || first_boot)
  {
    send_to_DB();
    last_DB_send_interval = millis() / 1000;
    settings_changed = false;
    first_boot = false;
  }

  delay(1000);

  EthernetClient client = server.available();
  //----------------------------------------------------------------------------
  if (client)
  {
    boolean currentLineIsBlank = true;
    while (client.connected())
    {
      if (client.available())
      {
        char c = client.read();
        HTTP_req += c;

        if (c == '\n' && currentLineIsBlank)
        {
          if (HTTP_req.indexOf("readDHT11") > -1) //AJAX request for DHT11 values
          {
            AJAX_request(client);
          }
          //--------------------------------------------------------------------
          else
          {

            html_webpage();
            client.println(webPage_MSB);
            client.println(webPage_LSB);
            webPage_MSB = "";
            webPage_LSB = "";

          }
          //--------------------------------------------------------------------
          Serial.print(HTTP_req);

          break;
        }
        //----------------------------------------------------------------------
        if (c == '\n') currentLineIsBlank = true;
        else if (c != '\r') currentLineIsBlank = false;
      }
    }
    delay(10);
    client.stop(); //sever client connection with server

    //manual pump
    if (HTTP_req.indexOf("pump_on") > 0)
    {
      pump_man = true;
      pump_auto = false;
      pump_off = false;
    }
    if (HTTP_req.indexOf("pump_off") > 0)
    {
      pump_off = true;
      pump_man = false;
      pump_auto = false;
    }



    //automatic pump
    //Referer: http://192.168.1.177/action_page.php?lname=10
    if (HTTP_req.indexOf("lname") > 0) //checks for 2
    {
      String data = "";
      int i = 6;
      while (1)
      {
        if (HTTP_req[HTTP_req.indexOf("lname") + i] == '\n')
        {
          break;
        }

        data += HTTP_req[HTTP_req.indexOf("lname") + i];

        if (i == 10)
        {
          data = "0";
          break;
        }

        i++;
      }
      moisture_max_th = data.toInt();

      if ( moisture_max_th  >= last_moisture_max_th + 1 or moisture_max_th <= last_moisture_max_th - 1)
      {
        settings_changed = true;
        moist_th_db = moisture_max_th;


        pump_auto = true;
        pump_man = false;
        pump_off = false;
        AJAX_request(client);
        Serial.println("Pump is now automatic");
        last_moisture_max_th = moisture_max_th;
      }

      Serial.println(moisture_max_th);
      data = "";
    }

    HTTP_req = ""; //reset HTTP request string
  }

  if (moisture <= moisture_max_th && pump_auto && !pump_man && hum > 60)
  {
    digitalWrite(PUMP_PIN, LOW);
    pump_off = false;
    Serial.println("Auto pump on");
  }
  else if (moisture > moisture_max_th && pump_auto && !pump_man)
  {
    digitalWrite(PUMP_PIN, HIGH);
    pump_off = true;
    Serial.println("Auto pump off");
  }
  else if (moisture_max_th <= last_moisture_max_th - 10)
  {
    digitalWrite(PUMP_PIN, LOW);
    pump_off = false;
    Serial.println("Auto pump on");
  }

  if (pump_man && !pump_auto)
  {
    digitalWrite(PUMP_PIN, LOW);
    Serial.println("man pump on");
  }

  if (pump_off) {
    digitalWrite(PUMP_PIN, HIGH);
    Serial.println("man pump off");
    change_moist_th = true;
  }

  if (pump_off != prev_pump_stat)
  {
    settings_changed = true;
    prev_pump_stat = pump_off;
  }



}
