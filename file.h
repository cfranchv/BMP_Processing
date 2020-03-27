/*
 * file.h
 *
 *  Created on: Mar 27, 2020
 *      Author: cfran
 */

#ifndef FILE_H_
#define FILE_H_

#include <stdio.h>
#include <string.h>

char* _filetype(char* str) {
	int length = strlen(str);
	char* ret = malloc(sizeof(*ret) * 4);
	ret[0] = str[length - 3];
	ret[1] = str[length - 2];
	ret[2] = str[length - 1];
	ret[3] = str[length - 0];

	return ret;
}


long long int FileSize(FILE* fp) {
	fseek(fp, 0L, SEEK_END);
	long int size = ftell(fp);
	fseek(fp, 0L, SEEK_SET);
	return size;
}

int ReadTxt(FILE* fp, char* filename, char* contents, char** a_error) {
	char* suffix = ".txt";
	char* filetype = _filetype(filename);

	if(strcmp(suffix, filetype)) {
		*a_error = "Wrong filetype, please only use .txt files";
		return 0;
	}

	long long int filesize = FileSize(fp);
	contents = malloc(sizeof(*contents) * filesize);

	if(contents == NULL) {
		*a_error = "Could not free enough memory";
		return 0;
	}
	if(fread(contents, sizeof(*contents), filesize, fp) != filesize) {
		*a_error = "Failed to read txt file";
		return 0;
	}
	else return 1;
}


#endif /* FILE_H_ */
