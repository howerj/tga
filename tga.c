/* Convert text file containing image data with one character
 * representing a pixel, one line of text representing a row,
 * into a TGA file. 
 *
 * Example input for a boxed question mark character:
 *
 *     01111110
 *     11000011
 *     10011001
 *     10011001
 *     11110011
 *     11100111
 *     11100111
 *     11111111
 *     11100111
 *     11100111
 *     01111110
 *
 * xxd -c 32 -g 4 -s 18 font.tga > font.tga.txt
 *
 * Vertical to horizontal:
 * awk "{a[i%12]=a[i%12]$0;i++}END{for(x=0;x<12;x++){print a[x]}}" font.bin > font2.bin
 *
 * @todo Convert to Black and White TGA instead of color?
 * That would require some bit buffer routines and some changes
 * in the format header */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <stdarg.h>

#define ON       (0xff)
#define OFF      (0x00)
#define ON_CHAR  ('1')
#define OFF_CHAR ('0')

typedef struct {
	uint8_t h[18];
} header_t;

static void die(const char *fmt, ...)
{
	va_list args;
	assert(fmt);
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	fputc('\n', stderr);
	fflush(stderr);
	exit(EXIT_FAILURE);
}

static const char *reason(void)
{
	static const char *unknown = "unknown reason";
	const char *r;
	if(errno == 0)
		return unknown;
	r = strerror(errno);
	if(!r)
		return unknown;
	return r;
}

static void *allocate_or_die(size_t length)
{
	void *r;
	errno = 0;
	r = calloc(1, length);
	if(!r)
		die("allocation of size %zu failed: %s", length, reason());
	return r;
}

static FILE *fopen_or_die(const char *file, const char *mode)
{
	FILE *f = NULL;
	assert(file);
	assert(mode);
	errno = 0;
	f = fopen(file, mode);
	if(!f)
		die("failed to open file '%s' (mode %s): %s", file, mode, reason());
	return f;
}

static header_t header(uint16_t width, uint16_t height)
{
	header_t h;
	const header_t template = {
		.h = {
			0,          /* ID Length, 0 = No Id */
			0,          /* 0 = No color map */
			2,          /* 2 = Uncompressed, true color */
			0, 0, 0, 0, /* Color Specification */
			0,          /* Color Specification */
			0, 0,       /* X Origin Lo/Hi */
			0, 0,       /* Y origin Lo/Hi */
			0, 0,       /* Width    Lo/Hi */
			0, 0,       /* Height   Lo/Hi */
			32,         /* Pixel Depth (8, 16, 24, 32) */
			2 << 4      /* Image Descriptor, Top Left Pixel Order */
		}
	};
	h = template;
	h.h[12] = width   & 0xFF;
	h.h[13] = width  >> 8;
	h.h[14] = height  & 0xFF;
	h.h[15] = height >> 8;
	/*fprintf(stderr, "w:%x h:%x\n", (unsigned)width, (unsigned)height);*/
	return h;
}

static void reset(FILE *f)
{
	int r = 0;
	errno = 0;
	assert(f);
	r = fseek(f, 0, SEEK_SET);
	if(r < 0)
		die("fseek failed on %p: %s", f, reason());
}

static inline line_width(const char *s)
{
	assert(s);
	return strlen(s) - 1;
}

static uint16_t width(FILE *in)
{
	int c;
	uint32_t w = 0;
	assert(in);
	reset(in);
	errno = 0;
	for(;EOF != (c = fgetc(in));) {
		if(c == '\n' || c == '\r')
			break;
		w++;
		assert(w < UINT16_MAX);
	}
	reset(in);
	return w;
}

static uint16_t height(FILE *in)
{
	uint32_t h = 0;
	uint32_t w = width(in);
	char *line = allocate_or_die(w+3); /* +3 = LF+CR+NUL */
	reset(in);

	for(;fgets(line, w + 2, in);) { /* +2 = LF+CR */
		size_t lw = line_width(line);
		if(lw != w)
			die("invalid line width: %u/%u/%s", (unsigned)lw, (unsigned)w, line);
		h++;
	}
	assert(h < UINT16_MAX);
	reset(in);
	free(line);
	return h;
}

static size_t fwrite_or_die(const void *ptr, size_t size, FILE *out)
{
	assert(out);
	assert(ptr);
	errno = 0;
	if(size != fwrite(ptr, 1, size, out))
		die("write of size %u failed: %s", (unsigned)size, reason());
	return size;
}

static void write_pixel(FILE *out, char c)
{
	/* Colors:        B    G    R    A    */
	char pixel[4] = { OFF, OFF, OFF, ON };
	assert(out);
	if(c == ON_CHAR)
		memset(pixel, ON, sizeof(pixel) - 1);
	else if(c != OFF_CHAR)
		die("invalid pixel value: %c/%x", c, c);
	fwrite_or_die(pixel, sizeof(pixel), out);
}

static int binary2tga(FILE *in, FILE *out)
{
	uint16_t w, h, lc;
	char *line;
	header_t head;
	assert(in);
	assert(out);
	w    = width(in);
	h    = height(in);
	head = header(w, h);
	fwrite_or_die(head.h, sizeof(head.h), out);
	line = allocate_or_die(w+3);  /* +3 = LF+CR+NUL */
	
	for(lc = 0;fgets(line, w + 2, in); lc++) { /* +2 = LF+CR */
		size_t i;
		assert(w == line_width(line));
		for(i = 0; i < w; i++)
			write_pixel(out, line[i]);
	}
	assert(lc == h);
	free(line);
	fflush(out);
	return 0;
}

int main(int argc, char **argv)
{
	FILE *in, *out;
	uint16_t w, h;
	header_t hed;
	if(argc != 3)
		die("usage: %s image.bin image.tga", argv[0]);
	in  = fopen_or_die(argv[1], "r");
	out = fopen_or_die(argv[2], "wb");
	binary2tga(in, out);
	fclose(in);
	fclose(out);
	return EXIT_SUCCESS;
}

