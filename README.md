## this is experimental code with SEEED XIAO brand ESP32S3 Sense board, with their own camera module
 - To get started, see [XIAO official doc](https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/)
 - To go wild and develop your own framework, see [ESP32 starter code](https://github.com/espressif/arduino-esp32#using-through-arduino-ide)

## Rationale:

I realized the easiest proof-of concept is camera based hand gesture detection using existing openCV algorithms, so the matter
falls back to how to get it done on esp32 devices instead of on a computer. The tentative plan is to use a computer/cloud computer
as the source of computation, and use esp32 devices as data collection. (imo running neural network on compact embeded devices is pretty dumb)
<br></br>
the next problem is that esp32 support on non-arduino idea is poor. The lib for arduino is too big anyways
so this repository started as an effort to trim-down the extensive esp32 framework from arduino core
and also make it work specifically on the XIAOesp32S3 board I got from Amazon, which is tiny and relatively cheap.
ANd that's where I am right now!

## Roadmap:
| progress                                                                                                                | Implemented            |
|-------------------------------------------------------------------------------------------------------------------------|------------------------|
| [sample code functional](https://github.com/espressif/arduino-esp32/tree/master/libraries/ESP32/examples/Camera/CameraWebServer) | :white_check_mark:     |
| [opencv native implementation](https://github.com/opencv/opencv-python)                                                 | :large_orange_diamond: |
| Tests&CI pipeline                                                                                                       | -                      |
| Add sensors                                                                                                             | -                      |

## A Tour to Existing files:
[camera module pin mapping](include/camera_pins.h)<br>
This is the pinfoot mapping going from the XIAO native camera into XIAO esp32s3 board
in the future if we acquire better camera (they are not that cheap lol) we need to add the
additional mappings in this file.

[http server code](src/app_httpd.cpp)<br>
This is almost exact copy-paste of the arduino core esp32 http server code,
it is an insanely assiduous/stupid frontend written in c++ for a web server that
streams camera image onto the subnet IP address of your esp32 board, with http!
which means:
- there is **no encryption** and the camera is de-facto free for grabs if you are lame enough to 
connect it to actual internet (we need firewall rules for it, which I am too lazy to write)
- standard **2.4GHz** connection, won't work with school wifi. 
- check the file itself for API endpoints. You can also network-monitor this stuff. 
to save your time, video is at port ``81``, route ```/stream```. Image is at port ``80``, ``/capture`` 
- AAAAND the file is friggin humongous(800+ lines). Someone smarter than me plz trim it down further.
<br></br>
[driver code/camera-esp32 pin connection code](src/main.cpp)<br>
This is the driver code, and maps the pinout from the camera onto the esp32 board.
If we get a fancier processor (like STM32 or something, and you guessed it! Not cheap!) 
we wil need to re-route the connection.
<br></br>
## Lastly, please note that the arduino core migration to platformio is horribly outdated.
Plz manually copy-paste the source code should you need any new fancy feature, or use Arduino IDE
