# cat-doorbell
Wireless, Programmable, Arduino based, motion triggered doorbell
<br>
<br>
Copy your regular 433Mhz doorbell clicker's signal with a push of a button.<br>
When programmed with a call signal, the cat-doorbell will ring the bell after motion is detected within a one minute window.<br>
The grace period is to avoid the bell ringing every time something/someone moves in front of the motion sensor. A cat that would like to get in will hang around for a while triggering the doorbell.<br>
The cat-doorbell will go to sleep if no movement is detected within the time frame. Motion will wake it again.

## Status indicator during usage
* Constant flashing - You need to [program](#programming) the cat-doorbell
* Tree or five short flashes - Sleep | Wake<br>
* Single short flash - Ding Dong! Your cat(s) want in.

## Status indicator in program mode
* Two flashes - Ready to receive signal from doorbell clicker
* Single flash - Successful copy stored/updated to EEPROM
* Five flashes - Timeout or failure to copy signal

## Programming
Hold down program button until status led flashes twice. You now have 5 seconds to push your regular doorbell clicker. The status led will flash five times if it times out and once if it succeeded.

## Hardware (BOM)
* 1 Arduino/ATMega328P
* 1 16Mhz Crystal
* 2 22 pF Ceramic capacitors
* 2 10K Resistor
* 1 220 Ohm Resistor
* 1 LED of your choice
* 1 Push button
* 1 Power switch
* 1 2AA Battery pack
* 1 HC-SR501 PIR motion sensor
* 1 5V DC-DC booster
* 1 433.92Mhz Transmitter
* 1 433.92Mhz Receiver

