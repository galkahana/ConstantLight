ConstantLight
=============

The script in tests/lightAdjustment.js togahter with surrounding modules, one lifx bulb, and one ip cam, 
can be used as a mechanism to maintain a constant brightness level. In other words, they can turn your lights on and off
automatically based on how much light is out there. Since it's using a light sensor (my ip cam) it doesn't matter if it's dark 
becasue it's night, or just because it's cloudy, the lights will go on. And when it's bright again - off.

you can calibrate the system by calling "node lightAdjustment -r",    
then run it once with "node lightAdjustment"    
or run it continously with "node lightAdjustment -c".

[more to follow...this is just something quick cause i don't want to lose the code]

essay about this is [here](https://kierkegaardandme.blogspot.com/2014/03/automatic-lighting-adjustment-with-lifx.html)
