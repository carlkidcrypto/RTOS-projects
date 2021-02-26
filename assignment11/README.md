# Assignment 7 (Vandaluino2)

- Control a Stepper Motor with your RTOS program based on Temperature and Humidity- Put a flag or something on your motor shaft so you can see it rotate
Your  7 Segment Display Task - Modify your Display Task to display Hex Numbers- Modify your control queue to display message for X seconds before advancing- If your Control Queue goes over 10 entries, clear queue and display OF (overflow) for 5 seconds, then continue.

displayed on the 7 segment LEDs.

I would suggest one digit showing the state of the dip switches and one digit showing the direction of the stepper motor (you figure out how to display that).   Feel free to display what you like.

Connect the HDC1080 (HDC1000 compatible) to the I2C Grove Plugs

- You can use the driver on github.com/switchdoclabs or your favorite

- Find a stepper motor driver to modifyDisplay The temperature / Humidity / Stepper Motor Status on your Serial screen

Dip Switches:

- DP1 On – Move stepper on humidity / Off on Temperature

- DP2 On – Overrides 1 and just continuously moves stepper clockwise

- DP3 On – Override's 1 and just continuously moves stepper counterclockwise

- If DP2 and DP3 are On, then do one revolution clockwise and one counterclockwise and repeat

- DP4 On stops everything and overrides DP1, DP2 and DP3