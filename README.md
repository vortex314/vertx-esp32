# vertx-esp32

The purpose is to have a framework for small embedded devices that have the ease of integration and development.

This framework is based on the ideas that also live  in Vertx and Node.js.

Elements included :
. Loosely coupled and autonomous objects
. An eventbus to send message between different objects 
. Eventbus addressing is based on addresses formulated as strings, internally these are converted to unique id's ( 16 bit ) which are more performant to do the routing
. Use of lambda's to specify event handlers 
. Verticles / Objects are eventLoop tasks ( VerticleCoRoutine ) or independent FreeRTos tasks ( VerticleTask ). Attention need to be paid when handlers are asyn invoked between Freertos tasks
. Serialization of messages on the Eventbus are based on CBOR binary serialization which proves to be efficient 
. The framework comes with a list of Objects :
.. Wifi : to signal connection setup and disconnection
.. Mqtt : assure publishing of events to mqtt server and maintain connection
.. Config : assures store of persistent config items
.. Telnet : CLI interface for chaning config
.. Monitor : reports task state to logger
.. Hardware : abstraction interface for peripherals