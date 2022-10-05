# Satellite-Communication-Subsystem
___
One of the main requirements of any satellite is to communicate with the ground station reliably by sending and receiving data. A design and a simulation of the communication system between the Ground Control Station (GCS) and the CubeSat for transmitting and receiving the telemetry data are introduced. Two different protocols are used for communication which are: Simple Serial Protocol (SSP) for the physical connections and AX.25 protocol for radio transmission. Furthermore, the error of the received data is checked using the Cyclic Redundancy Check (CRC) technique. The system is implemented using LabVIEW to verify the communication between CubeSat and GCS with Arduino.

##What is AX.25?
____
The AX.25 protocol is used to ensure link connectivity between the communicating stations; either in half or full-duplex environments.

##What is SSP?
___
Is a simple protocol intended for master slave communication on single-master serial buses, especially within small spacecraft.

##RF Transceiver
___
The Parallax 433 MHz RF Transceiver was used in this project at the Ground Control Station and the CubeSat.
more data can be found [Here]([I²C](https://www.mouser.com/datasheet/2/321/parallax_27982-433-mhz-rf-transceiver-documentatio-1197467.pdf)).
