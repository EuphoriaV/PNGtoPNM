#include "return_codes.h"
#include <math.h>
#include <stdio.h>
#ifdef ZLIB
	#include <zlib.h>
#elif defined(LIBDEFLATE)
	#include <libdeflate.h>
#elif defined(ISAL)
	#include <include/igzip_lib.h>
#endif

typedef unsigned char byte;

struct chunk
{
	size_t size;
	byte len[4];
	byte name[4];
	byte* data;
	byte crc[4];
};
struct chunk readChunk(FILE* in)
{
	struct chunk curChunk;
	fread(curChunk.len, 4, 1, in);
	curChunk.size = curChunk.len[0] * 256 * 256 * 256 + curChunk.len[1] * 256 * 256 + curChunk.len[2] * 256 + curChunk.len[3];
	fread(curChunk.name, 4, 1, in);
	curChunk.data = malloc(curChunk.size);
	if (curChunk.data == NULL)
	{
		return curChunk;
	}
	fread(curChunk.data, curChunk.size, 1, in);
	fread(curChunk.crc, 4, 1, in);
	return curChunk;
}

byte* filter1(byte* line, size_t len, int type)
{
	if (type == 0)
	{
		byte prev = 0;
		for (size_t i = 0; i < len; i++)
		{
			line[i] = (line[i] + prev) % 256;
			prev = line[i];
		}
	}
	else if (type == 2)
	{
		byte prev1 = 0, prev2 = 0, prev3 = 0;
		for (size_t i = 0; i < len; i += 3)
		{
			line[i] = (line[i] + prev1) % 256;
			prev1 = line[i];
			line[i + 1] = (line[i + 1] + prev2) % 256;
			prev2 = line[i + 1];
			line[i + 2] = (line[i + 2] + prev3) % 256;
			prev3 = line[i + 2];
		}
	}
	return line;
}

byte* filter2(byte* line, byte* prevLine, size_t len, int type)
{
	if (type == 0)
	{
		for (size_t i = 0; i < len; i++)
		{
			line[i] = (line[i] + prevLine[i]) % 256;
		}
	}
	else if (type == 2)
	{
		for (size_t i = 0; i < len; i += 3)
		{
			line[i] = (line[i] + prevLine[i]) % 256;
			line[i + 1] = (line[i + 1] + prevLine[i + 1]) % 256;
			line[i + 2] = (line[i + 2] + prevLine[i + 2]) % 256;
		}
	}
	return line;
}

byte* filter3(byte* line, byte* prevLine, size_t len, int type)
{
	if (type == 0)
	{
		byte prev = 0;
		for (size_t i = 0; i < len; i++)
		{
			line[i] = (line[i] + (prev + prevLine[i]) / 2) % 256;
			prev = line[i];
		}
	}
	else if (type == 2)
	{
		byte prev1 = 0, prev2 = 0, prev3 = 0;
		for (size_t i = 0; i < len; i += 3)
		{
			line[i] = (line[i] + (prev1 + prevLine[i]) / 2) % 256;
			prev1 = line[i];
			line[i + 1] = (line[i + 1] + (prev2 + prevLine[i + 1]) / 2) % 256;
			prev2 = line[i + 1];
			line[i + 2] = (line[i + 2] + (prev3 + prevLine[i + 2]) / 2) % 256;
			prev3 = line[i + 2];
		}
	}
	return line;
}

byte* filter4(byte* line, byte* prevLine, size_t len, int type)
{
	if (type == 0)
	{
		byte prev = 0;
		for (size_t i = 0; i < len; i++)
		{
			int a = (int)prev;
			int b = (int)prevLine[i];
			int c = i == 0 ? 0 : (int)prevLine[i - 1];
			int p = a + b - c;
			int pa = abs(p - a);
			int pb = abs(p - b);
			int pc = abs(p - c);
			if (pa <= pb && pa <= pc)
				line[i] = (line[i] + a) % 256;
			else if (pb <= pc)
				line[i] = (line[i] + b) % 256;
			else
				line[i] = (line[i] + c) % 256;
			prev = line[i];
		}
	}
	else if (type == 2)
	{
		byte prev1 = 0, prev2 = 0, prev3 = 0;
		for (size_t i = 0; i < len; i += 3)
		{
			int a1 = (int)prev1;
			int b1 = (int)prevLine[i];
			int c1 = i < 3 ? 0 : (int)prevLine[i - 3];
			int p1 = a1 + b1 - c1;
			int pa1 = abs(p1 - a1);
			int pb1 = abs(p1 - b1);
			int pc1 = abs(p1 - c1);
			if (pa1 <= pb1 && pa1 <= pc1)
				line[i] = (line[i] + a1) % 256;
			else if (pb1 <= pc1)
				line[i] = (line[i] + b1) % 256;
			else
				line[i] = (line[i] + c1) % 256;
			prev1 = line[i];

			int a2 = (int)prev2;
			int b2 = (int)prevLine[i + 1];
			int c2 = i < 2 ? 0 : (int)prevLine[i - 2];
			int p2 = a2 + b2 - c2;
			int pa2 = abs(p2 - a2);
			int pb2 = abs(p2 - b2);
			int pc2 = abs(p2 - c2);
			if (pa2 <= pb2 && pa2 <= pc2)
				line[i + 1] = (line[i + 1] + a2) % 256;
			else if (pb2 <= pc2)
				line[i + 1] = (line[i + 1] + b2) % 256;
			else
				line[i + 1] = (line[i + 1] + c2) % 256;
			prev2 = line[i + 1];

			int a3 = (int)prev3;
			int b3 = (int)prevLine[i + 2];
			int c3 = i < 1 ? 0 : (int)prevLine[i - 1];
			int p3 = a3 + b3 - c3;
			int pa3 = abs(p3 - a3);
			int pb3 = abs(p3 - b3);
			int pc3 = abs(p3 - c3);
			if (pa3 <= pb3 && pa3 <= pc3)
				line[i + 2] = (line[i + 2] + a3) % 256;
			else if (pb3 <= pc3)
				line[i + 2] = (line[i + 2] + b3) % 256;
			else
				line[i + 2] = (line[i + 2] + c3) % 256;
			prev3 = line[i + 2];
		}
	}
	return line;
}

int main(int argc, char** argv)
{
	if (argc != 3)
	{
		fprintf(stderr, "Wrong number of arguments");
		return ERROR_INVALID_PARAMETER;
	}
	FILE* in = fopen(argv[1], "rb");
	if (in == NULL)
	{
		fprintf(stderr, "Couldn't open input file");
		return ERROR_FILE_NOT_FOUND;
	}

	// Проверка сигнатуры png

	int eight[] = { 137, 80, 78, 71, 13, 10, 26, 10 };
	byte isPng;
	for (int i = 0; i < 8; i++)
	{
		fscanf(in, "%c", &isPng);
		if ((int)isPng != eight[i])
		{
			fclose(in);
			fprintf(stderr, "Invalid file");
			return ERROR_INVALID_DATA;
		}
	}

	//считываем IHDR

	struct chunk head = readChunk(in);
	if (head.data == NULL)
	{
		fclose(in);
		fprintf(stderr, "Couldn't allocate memory");
		return ERROR_NOT_ENOUGH_MEMORY;
	}
	if (strncmp(head.name, "IHDR", 4) || head.size != 13)
	{
		fclose(in);
		free(head.data);
		fprintf(stderr, "Invalid file");
		return ERROR_INVALID_DATA;
	}
	int width = head.data[0] * 256 * 256 * 256 + head.data[1] * 256 * 256 + head.data[2] * 256 + head.data[3];
	int height = head.data[4] * 256 * 256 * 256 + head.data[5] * 256 * 256 + head.data[6] * 256 + head.data[7];
	// Must be 8
	int bitDepth = head.data[8];
	// Must be 0 or 2
	int colorType = head.data[9];
	// Must be 0
	int compressionMethod = head.data[10];
	// Must be 0
	int filterMethod = head.data[11];
	// Must be 0
	int interlaceMethod = head.data[12];
	free(head.data);
	if (compressionMethod || filterMethod || interlaceMethod || bitDepth != 8 || !(colorType == 0 || colorType == 2))
	{
		fclose(in);
		fprintf(stderr, "Invalid file");
		return ERROR_INVALID_DATA;
	}
	int pixel = 1;
	if (colorType == 2)
	{
		pixel *= 3;
	}

	//Считываем IDAT и записываем его в image

	byte* image = malloc(width * height * pixel);
	if (image == NULL)
	{
		fclose(in);
		fprintf(stderr, "Couldn't allocate memory");
		return ERROR_NOT_ENOUGH_MEMORY;
	}
	_Bool ends = 0;
	struct chunk data;
	size_t length = 0;
	while (!feof(in))
	{
		data = readChunk(in);
		if (data.data == NULL)
		{
			fclose(in);
			free(image);
			fprintf(stderr, "Couldn't allocate memory");
			return ERROR_NOT_ENOUGH_MEMORY;
		}
		if (!strncmp(data.name, "IDAT", 4))
		{
			for (size_t i = length; i < length + data.size; i++)
			{
				image[i] = data.data[i - length];
			}
			length += data.size;
		}
		if (!strncmp(data.name, "IEND", 4))
		{
			ends = 1;
			if (data.size != 0)
			{
				fclose(in);
				free(data.data);
				free(image);
				fprintf(stderr, "Invalid file");
			}
		}
	}
	fclose(in);
	free(data.data);
	if (!ends)
	{
		free(image);
		fprintf(stderr, "Invalid file");
		return ERROR_INVALID_DATA;
	}
	size_t sizeOfPnm = width * height;
	if (colorType == 2)
	{
		sizeOfPnm *= 3;
	}
	byte* imageOut = malloc(sizeOfPnm * 2);
	if (imageOut == NULL)
	{
		free(image);
		fprintf(stderr, "Couldn't allocate memory");
		return ERROR_NOT_ENOUGH_MEMORY;
	}
	size_t availableSize = sizeOfPnm * 2;
	int res;

	//Разжатие

#ifdef ZLIB
	res = uncompress(imageOut, &availableSize, image, length);
#elif defined(LIBDEFLATE)
	res = libdeflate_zlib_decompress(libdeflate_alloc_decompressor(), image, length, imageOut, availableSize, &sizeOfPnm);
#elif defined(ISAL)
	struct inflate_state state;
	isal_inflate_init(&state);
	state.next_in = image;
	state.avail_in = length;
	state.next_out = imageOut;
	state.avail_out = availableSize;
	state.crc_flag = IGZIP_ZLIB;
	res = isal_inflate(&state);
#endif
	free(image);
	if (res)
	{
		free(imageOut);
		fprintf(stderr, "Invalid file");
		return ERROR_INVALID_DATA;
	}

	//Записываем данные в массив массивов

	byte** imageMap = malloc(height * (sizeof(byte*)));
	if (imageMap == NULL)
	{
		free(imageOut);
		fprintf(stderr, "Couldn't allocate memory");
		return ERROR_NOT_ENOUGH_MEMORY;
	}
	for (int i = 0; i < height; i++)
	{
		imageMap[i] = malloc(pixel * width + 1);
		if (imageMap[i] == NULL)
		{
			free(imageOut);
			for (int j = 0; j < i; j++)
			{
				free(imageMap[j]);
			}
			free(imageMap);
			fprintf(stderr, "Couldn't allocate memory");
			return ERROR_NOT_ENOUGH_MEMORY;
		}
		memcpy(imageMap[i], imageOut + (pixel * width + 1) * i, pixel * width + 1);
	}
	free(imageOut);
	FILE* out = fopen(argv[2], "wb");
	if (out == NULL)
	{
		for (int i = 0; i < height; i++)
		{
			free(imageMap[i]);
		}
		free(imageMap);
		fprintf(stderr, "Couldn't open/create output file");
		return ERROR_FILE_NOT_FOUND;
	}
	fprintf(out, "P%i\n%i %i\n255\n", colorType == 2 ? 6 : 5, width, height);
	for (int i = 0; i < height; i++)
	{
		int filter = imageMap[i][0];
		switch (filter)
		{
		case 0:
		{
			fwrite(imageMap[i] + 1, width * pixel, 1, out);
			break;
		}
		case 1:
		{
			fwrite(filter1(imageMap[i] + 1, width * pixel, colorType), width * pixel, 1, out);
			break;
		}
		case 2:
		{
			fwrite(filter2(imageMap[i] + 1, imageMap[i - 1] + 1, width * pixel, colorType), width * pixel, 1, out);
			break;
		}
		case 3:
		{
			fwrite(filter3(imageMap[i] + 1, imageMap[i - 1] + 1, width * pixel, colorType), width * pixel, 1, out);
			break;
		}
		case 4:
		{
			fwrite(filter4(imageMap[i] + 1, imageMap[i - 1] + 1, width * pixel, colorType), width * pixel, 1, out);
			break;
		}
		}
	}
	fclose(out);
	for (int i = 0; i < height; i++)
	{
		free(imageMap[i]);
	}
	free(imageMap);
	return ERROR_SUCCESS;
}
