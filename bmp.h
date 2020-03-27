/*
 * bmp.h
 *
 *  Created on: Mar 27, 2020
 *      Author: cfran
 */

#ifndef BMP_H_
#define BMP_H_

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "file.h"

static const size_t BMP_HEADER_SIZE = 54;
static const size_t DIB_HEADER_SIZE = 40;
static const size_t BPP				=  3; //bytes per pixel

#pragma pack(push)
#pragma pack(1)

typedef struct {
	uint16_t type;
	uint32_t size;
	uint16_t reserved1;
	uint16_t reserved2;
	uint32_t offset;
	uint32_t dib_header_size;
	int32_t width_px;
	int32_t height_px;
	uint16_t num_planes;
	uint16_t bits_per_pixel;
	uint32_t compression;
	uint32_t image_size_bytes;
	uint32_t x_resolution_ppm;
	uint32_t y_resolution_ppm;
	uint32_t num_colors;
	uint32_t important_colors;
} BMPHeader;
typedef struct {
	int x;
	int y;
	unsigned char r;
	unsigned char g;
	unsigned char b;
} Pixel;
typedef struct {
	BMPHeader header;
	Pixel* pixelmap;
} BMPImage;


/*
 * Some basic files set up to read and write BMP Images. All other functions will be declared in future functions
 */

Pixel* _data_to_pixelmap(unsigned char* data, BMPHeader header) {
	Pixel* pixelmap = malloc(sizeof(Pixel) * header.width_px * header.height_px);

	int idx = 0;

	for(int y = 0; y < header.height_px; y++) {
		for(int x = 0; x < header.width_px; x++) {
			int loc = x * 3 + y * header.width_px * 3;
			pixelmap[idx].x = x;
			pixelmap[idx].y = y;
			pixelmap[idx].r = data[loc];
			pixelmap[idx].g = data[loc + 1];
			pixelmap[idx].b = data[loc + 2];
			idx++;
		}
	}
	return pixelmap;
}
Pixel  _get_Pixel(int x, int y, Pixel* pixelmap, BMPHeader header) {
	int idx = (x + header.width_px * y);			//calculate the position of the pixel
	return pixelmap[idx];
}

unsigned char* _pixelmap_to_data(const BMPImage* image) {
	long int size_data = image -> header.image_size_bytes;
	unsigned char* data = malloc(sizeof(unsigned char) * size_data);
	int pix_idx = 0;
	for(int idx = 0; idx < size_data; idx += 3) {
		data[idx] = image -> pixelmap[pix_idx].r;
		data[idx +1] = image -> pixelmap[pix_idx].g;
		data[idx +2] = image -> pixelmap[pix_idx].b;
		pix_idx++;
	}
	return data;
}
void      FreeBMP(BMPImage* image) {
	free(image -> pixelmap);
	free(image);
}
int       CheckHead(BMPHeader* header) {
	if(header == NULL) return 0;
	if(header -> type != 0x4d42) return 0;
	if(header -> offset != BMP_HEADER_SIZE) return 0;
	if(header -> dib_header_size != DIB_HEADER_SIZE) return 0;
	if(header -> num_planes !=1) return 0;
	if(header -> compression != 0) return 0;
	if(header -> num_colors != 0) return 0;
	if(header -> important_colors != 0) return 0;
	if(header -> bits_per_pixel != 24) return 0;
	if(header -> image_size_bytes != header -> size - BMP_HEADER_SIZE) return 0;
	else return 1;
}
BMPImage* ReadImage(FILE* fp, const char** a_error) {
	if(fp == NULL) {
		*a_error = "Cant find file";
		return NULL;
	}
	BMPImage* image = malloc(sizeof(*image));

	long long int size = FileSize(fp);

	if(!fread(&(image -> header), sizeof(image -> header), 1, fp)) {
		*a_error = "Can't read image header";
		FreeBMP(image);
		return NULL;
	}

	if(!CheckHead(&(image -> header))) {
		*a_error = "Problem with image header";
		FreeBMP(image);
		return NULL;
	}

	fseek(fp, BMP_HEADER_SIZE, SEEK_SET);

	unsigned char* data = malloc(sizeof(unsigned char) * (image -> header.image_size_bytes));

	if(data == NULL) {
		*a_error = "Not enough memory for BMP file";
		FreeBMP(image);
		return NULL;
	}

	if(fread(data, sizeof(*data), image -> header.image_size_bytes, fp) != (image -> header.image_size_bytes)){
		*a_error = "Failed to read image data";
		FreeBMP(image);
		return NULL;
	}

	image -> pixelmap = _data_to_pixelmap(data, image -> header);
	return image;

}
BMPImage* CropBMP(const BMPImage* image, const char** a_error, int x1, int x2, int y1, int y2) {

	BMPImage* Cropped = malloc(sizeof(BMPImage));
	Cropped -> header = image -> header;
	Cropped -> header.width_px = x2 - x1;
	Cropped -> header.height_px = y2 - y1;
	Cropped -> header.image_size_bytes = ((x2 - x1) * (y2 - y1) * 3);
	Cropped -> header.size = Cropped -> header.image_size_bytes + BMP_HEADER_SIZE;



	int x = x1;
	int y = y1;

	int numpix = Cropped -> header.image_size_bytes / BPP;
	Cropped -> pixelmap = malloc(sizeof(Pixel) * numpix);


	for(int idx = 0; idx < numpix; idx++) {
		int x_new = x - x1;
		int y_new = y - y1;
		Cropped -> pixelmap[idx] = _get_Pixel(x, y, image -> pixelmap, image -> header);
		Cropped -> pixelmap[idx].x = x_new;
		Cropped -> pixelmap[idx].y = y_new;
		x++;								//O(n) time complexity!!!!
		if(x == (x2)) {
			y++;
			x = x1;
		}
	}
	return Cropped;
}
int       WriteBMP(BMPImage* image, const char** a_error, FILE* fp) {
	unsigned char* data = _pixelmap_to_data(image);
	if(fp == NULL) {
		*a_error = "File location not found";
		FreeBMP(image);
		return 0;
	}
	if(image == NULL) {
		*a_error = "Image not found";
		FreeBMP(image);
		return 0;
	}
	if(fwrite(&(image -> header), sizeof(image -> header), 1, fp) != 1) {
		*a_error = "Unable to write file header";
		FreeBMP(image);
		return 0;
	}
	if(fwrite(data, sizeof(*data), image -> header.image_size_bytes, fp) != image -> header.image_size_bytes) {
		*a_error = "Unable to write pixel data";
		FreeBMP(image);
		return 0;
	}
	return 1;
}

#endif /* BMP_H_ */
