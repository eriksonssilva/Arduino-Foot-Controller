# Arduino-Foot-Controller
<br>
#An arduino mega foot controller with 10 presets
#his project may or may not swit your needings.
#It does everything that many foot controllers do, but I have setup it to work with dpdt (permanent) switch.
#It also will have different actions on the LED's / button state depending on how  they were setup.
#E.g.: If you have setup the buttons 1, 2, 3, 4 and 5 to send PC messages and 6, 7, 8, 9, 10, 11 and 12 to send CC's,
#things will work somewhat like this:
#You will setup the initial state of the CC buttons (from 6 to 12), according to each PC button.
#If you step button 1 you may setup as follows:
#Button 6 - On
#Button 7 - On
#Button 8 - Off
#Button 9 - Off
#Button 10 - Off
#Button 11 - On
#Button 12 - On
#
#This way, if you step the button 6, it will send a midi OFF message.
#If you step the button 8, it will send a midi ON message.
#
#I made this foot controller to use with my Atomic Amplifire, which doesn't send/receive Sysex messages
#(or at least they don't have it documented anywhere).
#So for it to work as synced as possible, the button's states have to be the same as the unity's states.
#E.g: If you are using the button 1 to send PC 90 and go to preset 90
#and in this preset you are using button 6 to turn Boost On/Off, you must setup the button 6 to the same
#state it was setup in the unit. If Boost is Off in the unity, button 6 must be off in the controller and so on.
