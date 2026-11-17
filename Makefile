all: build/build/STOY.tap test

build/build/STOY.tap:	toy.c
	cd build ; make

cmake:
	rm -rf build ; mkdir build ; cd build ; cmake ..

test:
	/Users/j/Documents/Development/seclorum/retro/oric/development/soundToy/oricutron.seclorum/oricutron /Users/j/Documents/Development/seclorum/retro/oric/development/soundToy/build/build/STOY.tap
