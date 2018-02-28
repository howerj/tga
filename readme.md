# TGA

This is a simple program to convert a file containing a [Netpbm][] like 
format, containing a black and white image in a textual format (with white 
being [ASCII][] '0' and black '1') into a [TGA][] file. 

To build requires *make* and a *C99* compiler:

Type:

	make

To build, and:

	make run

To convert the file [font.bin][] into *font.tga*. And to clean up:

	make clean

[font.bin][] contains an extended [ASCII][] character set font. This file was
originally from this project
<https://opencores.org/project,interface_vga80x40>.

For a specification of the [TGA][] file format, consult [tga.pdf][].
 
[ASCII]: https://en.wikipedia.org/wiki/ASCII
[Netpbm]: https://en.wikipedia.org/wiki/Netpbm
[TGA]: https://en.wikipedia.org/wiki/.tga
[font.bin]: font.bin
[tga.pdf]: tga.pdf
