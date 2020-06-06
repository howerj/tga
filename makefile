CFLAGS=-Wall -Wextra -std=c99 -pedantic -O2
TARGET=tga

.PHONY: all clean run

all: run

run: ${TARGET} font.tga

%.tga: %.bin ${TARGET}
	./${TARGET} $< $@

%.pbm: %.bin
	echo P1 > $@
	awk "{a++}END{print length, a}" $< >> $@
	cat $< >> $@

clean:
	rm -fv ${TARGET} *.o *.tga *.pbm
