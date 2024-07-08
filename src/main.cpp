#include <Arduino.h>

const int ledPin = 2; // Onboard LED is connected to GPIO2

const int NODE = 4;

unsigned long ledStarted = 0;
long interval = 1000;

int ledState = LOW;

int display_mode = 0;
int display_count = 5;
int current_count = 0;

#include "painlessMesh.h"

#define MESH_PREFIX "Mesh_username"
#define MESH_PASSWORD "mesh_password"
#define MESH_PORT 5555

bool amController = false; // flag to designate that this is the current controller

Scheduler userScheduler;
painlessMesh mesh;

// void sendmsg();

// Task taskSendmsg(TASK_SECOND * 1, TASK_FOREVER, &sendmsg);

void sendmsg(int mode)
{
  mesh.sendBroadcast(String(mode));
  // String msg = "Message from Node " + String(NODE) + ", Id: ";
  // msg += mesh.getNodeId();
  // msg += " I am controller: " + String(amController);
  // mesh.sendBroadcast(msg);
  // taskSendmsg.setInterval(random(TASK_SECOND * 1, TASK_SECOND * 5));
}

void receivedCallback(uint32_t from, String &msg)
{
  Serial.printf("Node: %d IsCtrl: %d Received from %u msg=%s\n", NODE, amController, from, msg.c_str());
  display_mode = msg.toInt();
  current_count = 0;
  ledState = HIGH;
}

void newConnectionCallback(uint32_t nodeId)
{
  Serial.printf("New Connection, nodeId = %u\n", nodeId);
}

void changedConnectionCallback()
{
  Serial.printf("changedConnectionCallback\n");
  SimpleList<uint32_t> nodes;
  uint32_t myNodeID = mesh.getNodeId();
  uint32_t lowestNodeID = myNodeID;

  Serial.printf("Changed connections %s\n", mesh.subConnectionJson().c_str());

  nodes = mesh.getNodeList();
  Serial.printf("Num nodes: %d\n", nodes.size());
  Serial.printf("Connection list:");

  for (SimpleList<uint32_t>::iterator node = nodes.begin(); node != nodes.end(); ++node)
  {
    Serial.printf(" %u", *node);
    if (*node < lowestNodeID)
      lowestNodeID = *node;
  }

  Serial.println();

  if (lowestNodeID == myNodeID)
  {
    Serial.printf("Node: %d Id: %u I am the controller now", NODE, myNodeID);
    Serial.println();
    amController = true;
    // restart the current animation - to chatty - remove - better to wait for next animation
    // sendMessage(display_mode);
    // display_step = 0;
    // ul_PreviousMillis = 0UL; // make sure animation triggers on first step
  }
  else
  {
    Serial.printf("Node: %d Id: %u is the controller now", NODE, lowestNodeID);
    Serial.println();
    amController = false;
  }
}

void nodeTimeAdjustedCallback(int32_t offset)
{
  Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(), offset);
}

void setup()
{
  Serial.begin(115200);

  pinMode(ledPin, OUTPUT); // Set the LED pin as an output
  digitalWrite(ledPin, LOW);

  mesh.setDebugMsgTypes(ERROR | STARTUP);

  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

  // userScheduler.addTask(taskSendmsg);
  // taskSendmsg.enable();
}

void loop()
{
  mesh.update();

  unsigned long currentMillis = millis();

  switch (display_mode)
  {
  case 0:
    interval = 1000;
    display_count = 5;
    break;
  case 1:
    interval = 500;
    display_count = 10;
    break;
  case 2:
    interval = 100;
    display_count = 50;
    break;
  default:
    interval = 1000;
    display_count = 5;
    display_mode = 0;
    break;
  }

  if (currentMillis - ledStarted >= interval)
  {
    ledStarted = currentMillis;

    if (ledState == LOW)
    {
      ledState = HIGH;
    }
    else
    {
      ledState = LOW;
    }
    digitalWrite(ledPin, ledState);

    current_count++;

    if (amController && current_count >= display_count)
    {
      display_mode++;
      current_count = 0;
      ledState = HIGH;
      sendmsg(display_mode);
    }
  }
}