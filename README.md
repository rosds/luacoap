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

the output its a `libluacoap.so` shared library that can be loaded into lua.

### Usage

Currently it is only possible to send GET, PUT and POST request using the CoAP 
client.

#### Example

```lua
require("libluacoap")
client = coap.Client()
client:get(coap.CON, "coap://coap.me/test")
```

The current available functions are

```lua
client:get([ ConnectionType ,] url, [ ContentType, Payload ])
client:put([ ConnectionType ,] url, [ ContentType, Payload ])
client:post([ ConnectionType ,] url, [ ContentType, Payload ])
```

where:

* ConnectionType is either `coap.CON` or `coap.NON` for confirmable or non-confirmable, 
* url is the address to the resource
* ContentType is any of the CoAP supported content types
* Payload is the data you want to send
