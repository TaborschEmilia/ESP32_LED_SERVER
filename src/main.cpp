#include "WiFi.h"

TaskHandle_t primulHandle; // defining the rtos task handle

int ledc_channel_0 = 0;
int ledc_times_13_bit = 13;
int brightness = 0; // how bright the LED is
int fadeAmount = 5; // how many points to fade the LED by
double ledc_base_freq = 5000;
const char *ssid = "MyEsp32";      //Wifi network name
const char *password = "Password"; // Wifi password

WiFiServer server(80);

template <class T>
const T &min(const T &a, const T &b)
{
  return (a < b) ? a : b; //implementation of "min" function template
}                         //told ya' that i hate defines :)

void ledcAnalogWrite(int channel, int value, int valueMax = 255)
{
  // calculate duty, 8191 from 2 ^ 13 - 1
  int duty = (8191 / valueMax) * min(value, valueMax);
  // write duty to LEDC
  ledcWrite(channel, duty);
}

void PrimulTask(void *parameter) // the rtos task
{
  pinMode(LED_BUILTIN, OUTPUT); // set the LED pin mode
  server.begin();               // starting the web server

  while (1) // starting the endless loop on rtos task
  {
    WiFiClient client = server.available(); // listen for incoming clients

    if (client) // if you get a client,
    {
      String currentLine = "";   // make a String to hold incoming data from the client
      while (client.connected()) // loop while the client's connected
      {
        if (client.available()) // if there's bytes to read from the client,
        {
          char c = client.read(); // read a byte, then
          if (c == '\n')          // if the byte is a newline character
          {
            // if the current line is blank, you got two newline characters in a row.
            // that's the end of the client HTTP request, so send a response:
            if (currentLine.length() == 0)
            {
              // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
              // and a content-type so the client knows what's coming, then a blank line:
              client.println("HTTP/1.1 200 OK");
              client.println("Content-type:text/html");
              client.println();
              // the content of the HTTP response follows the header:
              client.print("<head><title>Test Led http protocol</title></head>");
              client.print("<center><h1><b>ESP32 Web Control!!</b></h1></center>");
              client.print("<center><p><b>Blue LED</b><a href=\"ON1\"><button>ON</button></a>&nbsp;<a href=\"OFF1\"><button>OFF</button></a></p></center>");
              client.print("<center><h1>Effects!!!!</h1></center>");
              client.print("<center><p>Blink<b>(2 Secs Aprox)</b><a href=\"BLINK\"><button>ON</button></center>");
              client.print("<center><p>Wave<b>(2 Secs Aprox)</b><a href=\"WAVE\"><button>ON</button></center>");
              // The HTTP response ends with another blank line:
              client.println();
              // break out of the while loop:
              break;
            }
            else
            { // if you got a newline, then clear currentLine:
              currentLine = "";
            }
          }
          else if (c != '\r') // if you got anything else but a carriage return character,
          {
            currentLine += c; // add it to the end of the currentLine
          }
          // Check to see if the client request was "GET /ON" or "GET /OFF":
          if (currentLine.endsWith("GET /ON1"))
          {
            digitalWrite(LED_BUILTIN, HIGH); // GET /ON turns the LED on
          }
          if (currentLine.endsWith("GET /OFF1"))
          {
            digitalWrite(LED_BUILTIN, LOW); // GET /OFF turns the LED off
          }
          if (currentLine.endsWith("GET /BLINK"))
          {
            for (int i = 0; i < 4; i++) // Blink the led 4 times
            {
              digitalWrite(LED_BUILTIN, HIGH);
              delay(500);
              digitalWrite(LED_BUILTIN, LOW);
              delay(500);
            }
          }
          if (currentLine.endsWith("GET /WAVE"))
          {
            ledcSetup(ledc_channel_0, ledc_base_freq, ledc_times_13_bit); //set the analogwrite channel
            ledcAttachPin(LED_BUILTIN, ledc_channel_0);                   // attach the analog out channel to LED pin
            for (int i = 0; i < 200; i++)                                 //fade in and out the led few times
            {
              ledcAnalogWrite(ledc_channel_0, brightness); //write the data to LED channel
              // change the brightness for next time through the loop:
              brightness = brightness + fadeAmount;
              // reverse the direction of the fading at the ends of the fade:
              if (brightness <= 0 || brightness >= 255)
              {
                fadeAmount = -fadeAmount;
              }
              // wait for 30 milliseconds to see the dimming effect
              delay(30);
            }
            ledcDetachPin(LED_BUILTIN); // detach the analog channel so we can access the digitalwrite
          }
        }
      }
      // close the connection:
      client.stop();
    }
  }
  vTaskDelete(NULL); // Should never get here
                     // if we ever get here by error, delete the task to free memory
}

void setup()
{
  Serial.begin(115200);
  WiFi.softAP(ssid, password); // start the Wifi ap mode

  xTaskCreatePinnedToCore( // starting the rtos task
      PrimulTask,    // Task function. 
      "PrimulTask",  // String with name of task, a name just for humans
      10000,         // Stack size in WORDS!!! NOT bytes 
      NULL,          // Parameter passed as input of the task
      1,             // Priority of the task. 
      &primulHandle, // Task handle. 
      1);            // our task run on core 1, wifi engine run on core 0
}

void loop()
{
  //just for fun, the loop return the task stack depth every 3 seconds
  Serial.println(uxTaskGetStackHighWaterMark(primulHandle));
  delay(3000);
}