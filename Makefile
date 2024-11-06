all: build flash monitor

build:
	idf.py build

flash:
	idf.py -p /dev/ttyACM0 flash

monitor:
	idf.py -p /dev/ttyACM0 monitor

clean:
	idf.py fullclean

