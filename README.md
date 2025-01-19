# LED strip controller

This is the documentation of LED strip controller, a device to control LED strip via WiFi/MQTT using ESP8266/ESP-201.

The following steps describe the hardware build and the software intallation of the device.

## Hardware build

The controller consists of an ESP-201 module with additional electronic components allowing the module to control the LED strip. Below, you can see the controller circuit scheme (also available in the `.kicad_sch` format):

![LED strip controller circuit scheme](led-strip-controller-circuit-scheme.png "LED strip controller circuit scheme")

In addition to control, the controller is also used to power the LED strip. The used components (resistors and Darlington transistors) allows to supply the LED strip up to 4A for each color. By replacing the voltage regulator `U1` and other components, it's possible to supply the LED strip with higher voltage and current. Buttons `PRG` and `RST` are added to ESP-201 to simply enable programming mode and device resetting.

Below, you can see both sides of the controller PCB design (also available in the `.svg` format, the design uses an universal drilled PCB with 2.54mm hole pitch):

![PCB design - top side](led-strip-controller-pcb-design-top.png "PCB design - top side")

![PCB design - bottom side](led-strip-controller-pcb-design-bottom.png "PCB design - bottom side")

The repository also includes printable 3D models (`.blend` files) of cases for the PCB. For assembling the box, you need 4 additional screws (e.g. 3.5×30mm screws) and a cable gland M12×1.5 needs to be used. Contrary to V1, V2 has additional mounts on the wall or furniture. 

Below, you can see an example of the built hardware:

![LED strip controller example 1](led-strip-controller-example-1.jpg "LED strip controller example 1")

![LED strip controller example 2](led-strip-controller-example-2.jpg "LED strip controller example 2")

![LED strip controller example 3](led-strip-controller-example-3.jpg "LED strip controller example 3")

![LED strip controller example 4](led-strip-controller-example-4.jpg "LED strip controller example 4")

**Note:** The ESP-201 module needed to be modified. The manufacturer places headers on the opposite side to where the components are soldered. I removed the headers and soldered them to the opposite side so that the entire device would fit in the box. In this example, I also use the internal antenne instead of the external. The selection is done by resoldering the 0Ω resitor:

![ESP-201](esp-201.jpg "ESP-201")

## Software installation

1. Open the `led-strip-controller.ino` file using the Arduino IDE program.

2. Customize the configuration part.

3. Connect the LED strip controller using a USB-UART adapter supporting ESP8266 to a computer holding the `PRG` button.

4. Flash the customized program.

## Authors

- [**Eli Nucknack**](mailto:eli.nucknack@gmail.com)
