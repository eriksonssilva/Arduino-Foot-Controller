# Arduino-Foot-Controller
<br>
An arduino mega foot controller with 10 presets<br>
This project may or may not swit your needings.<br>
It does everything that many foot controllers do, but I have setup it to work with dpdt (permanent) switch.<br>
It also will have different actions on the LED's / button state depending on how  they were setup.<br>
E.g.: If you have setup the buttons 1, 2, 3, 4 and 5 to send PC messages and 6, 7, 8, 9, 10, 11 and 12 to send CC's,<br>
things will work somewhat like this:<br>
You will setup the initial state of the CC buttons (from 6 to 12), according to each PC button.<br>
If you step button 1 you may setup as follows:<br>
Button 6 - On<br>
Button 7 - On<br>
Button 8 - Off<br>
Button 9 - Off<br>
Button 10 - Off<br>
Button 11 - On<br>
Button 12 - On<br>
<br>
This way, if you step the button 6, it will send a midi OFF message.<br>
If you step the button 8, it will send a midi ON message.<br>
<br>
I made this foot controller to use with my Atomic Amplifire, which doesn't send/receive Sysex messages<br>
(or at least they don't have it documented anywhere).<br>
So for it to work as synced as possible, the button's states have to be the same as the unity's states.<br>
E.g: If you are using the button 1 to send PC 90 and go to preset 90<br>
and in this preset you are using button 6 to turn Boost On/Off, you must setup the button 6 to the same<br>
state it was setup in the unit. If Boost is Off in the unity, button 6 must be off in the controller and so on.<br>
