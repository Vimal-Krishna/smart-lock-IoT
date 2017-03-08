/*
 *  This sketch demonstrates how to set up a simple HTTP-like server.
 *  The server will set a GPIO pin depending on the request
 *    http://server_ip/gpio/0 will set the GPIO2 low,
 *    http://server_ip/gpio/1 will set the GPIO2 high
 *  server_ip is the IP address of the ESP8266 module, will be 
 *  printed to Serial when the module is connected.
 */

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// setup a hotspot and enter credentials below
const char* ssid = "";
const char* password = "";

// input format
// http://<ip>/device_action?param1=<>&param2=<>

// http://<ip>/lock_lock?ph=9902444588
// http://<ip>/lock_unlock?ph=9902444588
// http://<ip>/lock_status?ph=9902444588

// Create an instance of the server
// specify the port to listen on as an argument

ESP8266WebServer server(80);

int lockPin   = D5; // blue, IN3 of relay board
int unlockPin = D6; // yellow, IN1 of relay board
String acl[10] = {"","","","","","","","","",""}; // must have atleast one phone number (10 digit)

int high = 0;
int low  = 1;
bool authorize()
{  
  for(uint8_t i = 0; i < server.args(); ++i)
  {
    if (server.argName(i) == "ph")
    {
      for(uint8_t j = 0; j < 10; ++j)
      {
        if (acl[j] == server.arg(i))
        {
          Serial.println("Authorized user " + server.arg(i) + " at " + j);
          return true;
        }
      }  
      Serial.println("Un-authorized user " + server.arg(i));
      return false;    
    }    
  }
  Serial.println("Un-authorized: user information not available");
  return false;
}

void send_status()
{
  // get the status - return status of BUILTIN_LED

  String status;
  int pinState = digitalRead(BUILTIN_LED);
  status = (pinState==high) ? "locked" : "unlocked" ;
  Serial.println("Status is: " + status);
  server.send(200, "application/json", "{\"status\":\"" + status + "\"}"); 
}

void do_lock_status()
{
  Serial.println("lock status requested");
  
  if (!authorize()) 
  {
    server.send(500, "text/json", "{\"error\":\"unauthorized\"}"  );
    return;
  }

  send_status(); 
}

void do_lock_lock()
{
  // get the parameters
  // authorize request

  Serial.println("lock action requested");
  
  if (!authorize()) 
  {
    server.send(500, "text/json", "{\"error\":\"unauthorized\"}"  );
    return;
  }

  // do actual locking - set the led pin to ON to indicate status
  digitalWrite(lockPin, high);
  delay(500);
  digitalWrite(lockPin, low);
  
  digitalWrite(BUILTIN_LED, high);
  
  // return the status
  send_status();
}

void do_lock_unlock()
{
  // get the parameters
  // authorize request

  Serial.println("unlock action requested");
  
  if (!authorize()) 
  {
    server.send(500, "text/json", "{\"error\":\"unauthorized\"}"  );
    return;
  }
  
  // do actual unlocking - set the led pin to OFF to indicate status
  digitalWrite(unlockPin, high);
  delay(500);
  digitalWrite(unlockPin, low);

  digitalWrite(BUILTIN_LED, low);
  
  // return the status
  send_status();
}

void do_acl_add()
{
  if (!authorize()) 
  {
    server.send(500, "text/json", "{\"error\":\"unauthorized\"}"  );
  }

  if (server.hasArg("newuser"))
  {
    String newuser = server.arg("newuser");
    bool added = false;
    for (uint8_t i = 0; i < 10; ++i)
    {
      if (acl[i] != "")
      {
        if (acl[i] == newuser)
        {
          Serial.println("User already exists in acl");
          break;
        }
      }
      else
      {
        acl[i] = newuser;
        Serial.println("User " + newuser + " added to acl");
        added = true;
        break;
      }
    }
    if (!added)
    {
      Serial.println("ACL is full");    
    }
  }
  else
  {
    Serial.println("do_acl_add: Invalid arguments");    
  }
  
  send_acl_list();
}

void do_acl_delete()
{
  if (!authorize()) 
  {
    server.send(500, "text/json", "{\"error\":\"unauthorized\"}"  );
    return;
  }

  if (server.hasArg("deleteuser"))
  {
    String deleteuser = server.arg("deleteuser");
    for (uint8_t i = 0; i < 10; ++i)
    {
      if (acl[i] != "")
      {
        if (acl[i] == deleteuser)
        {
          Serial.println("User " + deleteuser + " present at " + i);
          if ( i == 0)
          {
            Serial.println ("Cannot delete superuser");
            server.send(500, "application/json", "{\"error\":\"cannot remove superuser\"}"  );
            return;
            break;
          }
          for (uint8_t j = i; j < 9, acl[j] != ""; ++j)
          {
            acl[j] = acl [j+1];
          }
          acl[9] = "";
          break;
        }
      }
      else
      {
        Serial.println("User " + deleteuser + " not present in the ACL");    
        break;
      }
    }
    
  }
  else
  {
    Serial.println("do_acl_delete: Invalid arguments");    
  }
  send_acl_list();
  
}

void do_acl_read()
{
  if (!authorize()) 
  {
    server.send(500, "application/json", "{\"error\":\"unauthorized\"}"  );
    return;
  }
  send_acl_list();
}

void send_acl_list()
{
  String acl_list = "";
  for (uint8_t i = 0; i < 10; i++)
  { 
    
    acl_list += ("\"" + acl[i] + "\"");
    if (i != 9) 
    {
      acl_list += ",";
    }
  }
  Serial.println("ACL :" + acl_list);
  
  server.send(200, "application/json", "{\"acl_list\":[" + acl_list + "]}");
}

void print_help()
{
  String help_message;
  help_message += "Commands\n";
  help_message += "lock_lock\nlock_unlock\nlock_status\n";
  help_message += "acl_add\nacl_delete\nacl_report\n";

  server.send(200, "text/plain", help_message);
}

void setup() {

  pinMode(lockPin,   OUTPUT);
  pinMode(unlockPin, OUTPUT);
  digitalWrite(lockPin,   low);
  digitalWrite(unlockPin, low);
  
  Serial.begin(115200);
  Serial.println("Starting");
  delay(10);

  pinMode(BUILTIN_LED, OUTPUT);
  digitalWrite(BUILTIN_LED, low);
  
  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  
  // configure server behavior
  server.on("/", print_help);
  server.on("/lock_lock", do_lock_lock);
  server.on("/lock_unlock", do_lock_unlock);
  server.on("/lock_status", do_lock_status);
  server.on("/acl_add", do_acl_add);
  server.on("/acl_delete", do_acl_delete);
  server.on("/acl_report", do_acl_read);
  
  // Start the server
  server.begin();
  Serial.println("Server started");

  // Print the IP address
  Serial.println(WiFi.localIP());
  
  // set to locked state
  digitalWrite(lockPin, high);
  delay(500);
  digitalWrite(lockPin, low);
  
  // set led state to indicate locked state
  digitalWrite(BUILTIN_LED, high);
  
  
}

void loop() {

  server.handleClient();
  
}

