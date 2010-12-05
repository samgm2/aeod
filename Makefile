#
# Were the World Be Mine
#
# -std=gnu99 or -ansi?
#

TARGET_BINARY=main
CROSS_COMPILE=avr-
CROSS_LIB=
CROSS_INC=
CROSS_FLAGS=-mmcu=atmega88 

# -Wl,-Tdata,0x800100

OBJCOPY=${CROSS_COMPILE}objcopy
CC=${CROSS_COMPILE}gcc ${CROSS_FLAGS}
CXX=${CROSS_COMPILE}g++ ${CROSS_FLAGS}
CFLAGS=${CROSS_FLAGS} -Wall  -ansi -Os
CPPFLAGS=${CROSS_INC} -I./include
LDFLAGS=${CROSS_LIB}
 
MAKEFLAGS += -rR --no-print-directory -s


OBJECTS=main.o

all: clean_bin clean compile link clean

compile: ${OBJECTS}


burn:
	uisp -dlpt=/dev/parport0  -dprog=bsd --erase
	uisp -dlpt=/dev/parport0  -dprog=bsd --upload if=${TARGET_BINARY}.hex

link:
	${CC} ${LDFLAGS} *.o -o ${TARGET_BINARY}
	${OBJCOPY} -O binary ${TARGET_BINARY} ${TARGET_BINARY}.bin
	${OBJCOPY} -O ihex ${TARGET_BINARY} ${TARGET_BINARY}.hex

clean_bin:
	rm -rf ${TARGET_BINARY}
	rm -rf ${TARGET_BINARY}.bin
	rm -rf ${TARGET_BINARY}.hex
clean:
	rm -rf modules/*.o
	rm -rf *.o
	rm -rf *.bin
	rm -rf *.asm
