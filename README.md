# luacoap

This project is a simple lua binding over the 
[SMCP](https://github.com/darconeous/smcp) CoAP stack. The current version only 
implements some client calls.

### Building

To build this project, you require to install cmake

```bash
git clone https://github.com/alfonsoros88/luacoap
mkdir -p luacoap/build
cd luacoap/build
cmake ..
make
```

the output its a `coap.so` shared library that can be loaded into lua.

You can optionally install this library on your `lib/lua/5.2` with:

```bash
sudo make install
```

and you can use this module independently of your location.

### Usage

Currently it is only possible to send GET, PUT and POST request using the CoAP 
client.

#### Example

```lua
coap = require("coap")
client = coap.Client()
client:get(coap.CON, "coap://coap.me/test")
```

The current available functions are

```lua
client:get([ ConnectionType ,] url [, ContentType, Payload ])
client:put([ ConnectionType ,] url [, ContentType, Payload ])
client:post([ ConnectionType ,] url [, ContentType, Payload ])
```

where:

* ConnectionType is either `coap.CON` or `coap.NON` for confirmable or non-confirmable, 
* url is the address to the resource
* ContentType is any of the CoAP supported content types
* Payload is the data you want to send

#### Observe

Currently observe is available only without payload

```lua
coap = require("coap")
client = coap.Client()
client:observe("coap://coap.me/test")
```
