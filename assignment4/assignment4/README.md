# Assignment 4 (Vandaluino2)

Using FreeRTOS, construct a task to count down a counter from  42 to 00 each count every ½  second, then reverse to 00 to 42 and repeat   Think of using a queue to communicate between tasks.
Blink the D13 LED on each count NOT USING DELAY!   Write a task to do this and communicate to this task from the counter task.  Semaphores or queues can work for this.
Now the tricky part…….
As you see in the schematics, both 7 segment LEDs are controlled by the same D0-D7 data pins.  That means to see both digits, we must multiplex the displays.   The Test program for the 7 Segment program will give you the data pins (plus it is ALL ON THE SCHEMATIC)
You can build an additional two tasks to do this.  One to display the left digit of the display and one to display the right digit.   Doing this over 30 times a second will allow the mind to see two digits lite up. (persistence of vision.  Same way a TV works) .  You have to figure out how to synchronize the tasks.  Semaphore?  Hmm.I leave it to you to figure out what you have to do to make this work in each task.  Can you do the 7 Segment drivers in one task? Your Counting logic must be in a separate task
