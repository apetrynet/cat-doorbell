# cat-doorbell
Wireless, Programmable, Arduino based, motion triggered doorbell
<br>
Copy your regular 433Mhz doorbell clicker's signal with a push of a button.<br>
When programmed with a call signal, the cat-doorbell will ring the bell after motion is detected within a 1.5 minutes window.<br>
The grace period is to avoid the bell ringing every time something/someone moves in front of the motion sensor. A cat that would like to get in will hang around for a while triggering the doorbell.

## Status Indicator
* Constant - You need to [program](#programming) the cat-doorbell
* Multiple blinks out of program mode. - Sleep | Wake<br>
The cat-doorbell will go to sleep if no movement is detected within the time frame. Motion will wake it again.

* Short blink - Ding Dong! Your cat wants in.
 
## Programming
Hold down program button until status led blinks twice. You now have 5 seconds to push your regular doorbell clicker. The status led will blink multiple times if it times out and once if it succeeded.

## Hardware (BOM)
* 1 Arduino/ATMega328P
* 1 16Mhz Crystal
* 2 XX Ceramic capacitors
* 1 1K Resistor
* 1 10K Resistor
* 1 220 Ohm Resistor
* 1 xxx Ohm Resistor (for reset prevetion on arduino)
* 1 LED of your choice
* 1 2P2222 NPN Transistor
* 1 Push button
* 1 Power switch
* 1 2AA Battery pack
* 1 HC-SR501 PIR motion sensor
* 1 5V DC-DC booster
* 1 433.92Mhz Transmitter
* 1 433.92Mhz Receiver

## Notes
To save power and run the cat-dorbell on two AA batteries you'll have to [bypass the 5v](http://randomnerdtutorials.com/modifying-cheap-pir-motion-sensor-to-work-at-3-3v/) regulator on the motion sensor board.<br> 
The transistor gives power to the booster which in turn powers the transmitter and receiver.
