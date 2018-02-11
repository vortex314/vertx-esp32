# vertx-esp8266

The purpose is to have a framework for small embedded devices that have the ease of integration and development

Historically I started with MQTT framework which forces everything in a publish/subscribe pattern. 

The Vertx framework is like node.js and is fully event driven;

The target is to integrate into a bigger Vertx cluster , Java based that contains the more complex logic. 

A basic ingredient is the Eventbus that spans all devices and PC participants

Inside a Java context the Eventbus is using JSONObject messages to communicate.

Within a microcontroller I considered other serialization methods, with the possibility to translate to JSON when needed.

2 Serailization forms :
. CBOR with a map key/values which is very much type aware
. FlatBuffer which helps to parse withou creating copies. ( To investigate )

On the TODO :
. create a Worker Verticle based on a dedicated thread of FreeRTOS
. create a Verticle based on the coroutines of FreeRTOS which share a common thread
. port to ESP32 
. Create an internal Eventbus high-speed
. Gateway to/from JSON Vertx JSON based Eventbus
# vertx-esp32 
# vertx-esp32 
