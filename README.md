# luacoap

This project is a simple lua binding to the 
[SMCP](https://github.com/darconeous/smcp) CoAP stack. The current version only 
implements some client calls.

### Building

To build this project, you require to install cmake and lua

```bash
git clone https://github.com/alfonsoros88/luacoap
mkdir -p luacoap/build
cd luacoap/build
cmake ..
make
```

the output its a `coap.so` shared library that can be loaded into lua.

You can optionally install this module with:

```bash
sudo make install
```

and you can use it independently of your location.

Optionally, you can download and install the [debian 
package](https://github.com/alfonsoros88/luacoap/raw/master/downloads/luacoap-0.0.1%7Ealpha1-Linux.deb).

or `luarocks`

```lua
luarocks install luacoap
```

### Usage

Currently it is only possible to send GET, PUT and POST request using the CoAP 
client.

#### Example

```lua
coap = require("coap")
client = coap.Client()

function callback(playload)
  print(playload)
end

client:get(coap.CON, "coap://coap.me/test", callback)
```

The current available functions are

```lua
client:get([ ConnectionType ,] url [, ContentType, Payload ], [ callback ])
client:put([ ConnectionType ,] url [, ContentType, Payload ], [ callback ])
client:post([ ConnectionType ,] url [, ContentType, Payload ], [ callback ])
client:observe([ ConnectionType ,] url [, ContentType, Payload ], [ callback ])
```

where:

* ConnectionType is either `coap.CON` or `coap.NON` for confirmable or non-confirmable, 
* url is the address to the resource
* ContentType is any of the CoAP supported content types
* Payload is the data you want to send
* callback is a function that will be executed when the response arrives

##### Observe Request

The observe request is different from the others since it returns a `listener` 
object. This object can be used to control the subscription to the target 
resource. The `listener` object implements the following methods.

```lua
listener:callback()   -- Executes the callback function provided to the client
listener:listen()     -- Starts observing the resource
listener:stop()       -- Stops the observation
listener:pause()      -- Suspends the observation
listener:continue()   -- Resumes the observation
```
