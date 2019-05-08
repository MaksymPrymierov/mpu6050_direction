# MPU6050 Direction driver

The driver is created for single-board computers orange pi one.

## Connection

* MPU6050 -> Orange Pi One
* VCC     -> PIN1
* GND     -> PIN6
* SCL	  -> PIN5
* SDA 	  -> PIN3

## Using

The driver writes information about the inclination direction of the device to a file along the path: `/sys/class/mpu6050/derection_y`
