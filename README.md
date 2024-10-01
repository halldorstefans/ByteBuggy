# ByteBuggy

<!-- TABLE OF CONTENTS -->
## Table of Contents

* [About the Project](#bytebuggy-v311)
  * [Built With](#built-with)
* [Contact](#contact)
* [Acknowledgements](#acknowledgements)

<!-- ABOUT THE PROJECT -->
## ByteBuggy V3.1.1

This is a project where a small car is controlled remotely from a website.

The browser sends an MQTT message via the [Eclipse Paho JavaScript client](https://eclipse.dev/paho/index.php?page=clients/js/index.php), it is published to a specific topic on the MQTT server. The Raspberry Pi subscribes to that topic, receives the message, and processes it to control the motors.

[More in this blog post.](www.engineeracar.com/control-rpi-mqtt/ )

### Built With

* C++
* Javascript

<!-- CONTACT -->
## Contact

Halldor Stefansson - [@halldorstefans](https://twitter.com/halldorstefans) - <halldor@engineeracar.com>

<!-- ACKNOWLEDGEMENTS -->
## Acknowledgements

* [Eclipse Mosquitto message broker](https://mosquitto.org/)
* [Eclipse Paho (MQTT Clients)](https://eclipse.dev/paho/index.php?page=downloads.php)
