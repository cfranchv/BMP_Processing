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
#include <pthread.h>

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
typedef struct {
	Pixel* pixelmap;
	size_t radius;
	size_t numpix;
	int center_x;
	int center_y;
} BinArg;		//binarization argument

/*
 * Some basic files set up to read and write BMP Images. All other functions will be declared in future functions
 */
Pixel* _get_Pixel_area(int x, int y, size_t r, BMPImage* image) {
	int numpix = (2*r+1)*(2*r+1); //gets the number of pixels in a square around the central pixel
	int idx = (x - (2*r+1)) + (image -> header.width_px * (y - (2*r+1)));
	Pixel* ret = malloc(sizeof(Pixel) * numpix);

	int xnew = 0;
	int ynew = 0;

	for(int i = 0; i < numpix; i++) {
		if(xnew == (2*r +1)) {
			xnew = 0;
			ynew++;
		}
		ret[i] = image -> pixelmap[idx + i];
		ret[i].x = xnew;
		ret[i].y = ynew;
		xnew++;
	}
	return ret;
}
int    _edge_case(int n, int min, int max) {
	if(n < min)      return min;
	else if(n > max) return max;
	else             return n;
}
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
void   _set_Pixel(int x, inty, unsigned char r, unsigned char g, unsigned char b, BMPImage* image) {
	int idx = x + (image -> header.width_px * y);
	image -> pixelmap[idx].r = r;
	image -> pixelmap[idx].g = g;
	image -> pixelmap[idx].b = b;

	return;
}
Pixel  _avg_Pixel(Pixel pix) {
	Pixel ret = pix;

	unsigned char avg = pix.r + pix.g + pix.b;
	avg = avg / 3;

	ret.r = avg;
	ret.g = avg;
	ret.b = avg;

	return ret;
}
Pixel  _lum_Pixel(Pixel pix) {
	Pixel ret = pix;

	unsigned char lum = (unsigned char) (0.3 * pix.r + 0.59 * pix.g + 0.11 * pix.b);
	ret.r = lum;
	ret.g = lum;
	ret.b = lum;

	return ret;
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


	/* author: Charles Franchville
	 * cfranchv@purdue.edu
	 * cfranch477@gmail.com
	 *
	 * Purpose:
	 * Cropping BMP Image
	 *
	 * Inputs:
	 * image -> image that one wishes to crop
	 * a_error-> string for error handling
	 * x1 -> initial x margin
	 * x2 -> final x margin
	 * y1 -> initial y margin
	 * y2 -> final y margin
	 *
	 */

	BMPImage* Cropped = malloc(sizeof(BMPImage));

	if(Cropped == NULL) {
		*a_error = "Unable to reserve memory for cropped image.";
		return NULL;
	}
	Cropped -> header = image -> header;
	Cropped -> header.width_px = x2 - x1;
	Cropped -> header.height_px = y2 - y1;
	Cropped -> header.image_size_bytes = ((x2 - x1) * (y2 - y1) * 3);
	Cropped -> header.size = Cropped -> header.image_size_bytes + BMP_HEADER_SIZE;

	if(!CheckHead(&(Cropped -> header))) {
		FreeBMP(Cropped);
		*a_error = "Problem with image header";
		return NULL;
	}

	int x = x1;
	int y = y1;

	int numpix = Cropped -> header.image_size_bytes / BPP;
	Cropped -> pixelmap = malloc(sizeof(Pixel) * numpix);

	if(Cropped -> pixelmap == NULL) {
		a_error = "Cannot reserve memory for pixelmap.";
		FreeBMP(Cropped);
		return NULL;
	}


	for(int idx = 0; idx < numpix; idx++) {
		int x_new = x - x1;
		int y_new = y - y1;
		Cropped -> pixelmap[idx] = _get_Pixel(x, y, image -> pixelmap, image -> header);
		Cropped -> pixelmap[idx].x = x_new;
		Cropped -> pixelmap[idx].y = y_new;
		x++;
		if(x == (x2)) {
			y++;
			x = x1;
		}
	}
	return Cropped;
}
BMPImage* GrayBMP  (const BMPImage* image, const char** a_error, char method) {
	/*
	 * Author: Charles Franchville
	 * cfranchv@purdue.edu
	 * cfranch477@gmail.com
	 *
	 * Inputs:
	 *  image -> image to be edited
	 *  a_error -> error message
	 *  method -> which method to calculate the grayscale of the image
	 *  			'a' for AVG method -> gives sharper definition
	 *  			'l' for luminosity method -> gives more pleasant picture quality
	 *
	 */

	BMPImage* Gray = malloc(sizeof(*Gray));

	if(Gray == NULL) {
		*a_error = "Unable to B/W current image";
		return NULL;
	}
	Gray -> header = image -> header;
	if(!CheckHead(&(Gray -> header))) {
		*a_error = "Unable to read image header";
		FreeBMP(Gray);
		return NULL;
	}

	long int numpix = Gray -> header.image_size_bytes / BPP;
	Gray -> pixelmap = malloc(sizeof(Pixel) * numpix);

	if(Gray -> pixelmap == NULL) {
		*a_error = "Unable to write image pixel data";
		FreeBMP(Gray);
		return NULL;
	}

	for(int idx = 0; idx < numpix; idx++) {
		if(method == 'a') {
			Gray -> pixelmap[idx] = _avg_Pixel(image -> pixelmap[idx]);
		}
		else if(method == 'l') {
			Gray -> pixelmap[idx] = _lum_Pixel(image -> pixelmap[idx]);
		}
	}

	return Gray;

}
void* _binWorker(void* arg){}
BMPImage* BinarizeBMP(const BMPImage* image, const char** a_error, int rad, int threads) {
	BMPImage* Binarized = malloc(sizeof(BMPImage));

	BinArg* args = malloc(sizeof(BinArg) * threads);
	size_t numpix = image -> header.height_px * image -> header.width_px;
	size_t numpix_thread = numpix / threads;

	int x = rad;
	int y = rad;

	pthread_t* t = malloc(sizeof(pthread_t) * threads);

	for(int idx = 0; idx + numpix_thread - 1 < numpix; idx += numpix_thread) {
		int thread_idx = idx / numpix_thread;
		args[thread_idx] = (BinArg) {
			.pixelmap = _get_Pixel_area(x, y, rad, image);
			.radius = rad;
			.numpix = numpix_thread;
			.center_x = x;
			.center_y = y;
		};

		x += (2*rad + 1);
		if(x + rad >= image -> header.width_px) {
			x = rad;
			y += (2 * rad + 1);
		}

	}


	return Binarized;
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
