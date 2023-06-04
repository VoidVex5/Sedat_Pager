# Remote Control LCD
This program allows us to control a 2x16 LCD screen remotely via websockets. The websocket server code for the ESP32 was taken from the socket server example from [esp-idf](https://github.com/espressif/esp-idf) found in the [examples/protocols/sockets/tcp_server](https://github.com/espressif/esp-idf/tree/903af13e847cd301e476d8b16b4ee1c21b30b5c6/examples/protocols/sockets/tcp_server) subdirectory. 

# How to build
* Clone repository (`git clone https://github.com/VoidVex5/sedat_Pager.git`)
* Clone the esp-idf in the same directory as the previous repository (`git clone --recursive https://github.com/espressif/esp-idf.git)
* Change your directory to the clone of the esp-idf repository (`cd esp-idf`)
* Run the install script (`./install.sh` on Linux/Mac `./install.bat` on Windows).
* Add the esp-idf directory to your PATH or equivalent (`export PATH=/path/to/esp-idf:$PATH`)
* Now change your directory to this repositories local clone.
* Navigate to `tcp_server/`
* Run the export script on Windows (`export.bat`) or source it on Unix (`source export.sh`), and do this in every shell environmnt before using ESP-IDF.
* Run the build command (`idf.py build`)
* Run the flash & monitor command (`idf.py -p /port/to/esp32 flash monitor`)
* Using netcat connect to the server (`nc W.X.Y.Z PORT` replace `W.X.Y.Z` with the IP address of the ESP32 server, and PORT with the port number)
* Just use it!

## Configure the project
```
idf.py menuconfig
```

Set following parameters under Example Configuration Options:

* Set `IP version` of the example to be IPV4 or IPV6.

* Set `Port` number of the socket, that server example will create.

* Set `TCP keep-alive idle time(s)` value of TCP keep alive idle time. This time is the time between the last data transmission.

* Set `TCP keep-alive interval time(s)` value of TCP keep alive interval time. This time is the interval time of keepalive probe packets.

* Set `TCP keep-alive packet retry send counts` value of TCP keep alive packet retry send counts. This is the number of retries of the keepalive probe packet.

## Hardware Required
This example can be run on any commonly available ESP32 development board.
