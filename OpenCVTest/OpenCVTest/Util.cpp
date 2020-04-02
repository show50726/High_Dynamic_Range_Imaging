#include "stdafx.h"
#include "Util.h"

bool isFinite(double x) {
	return (x <= DBL_MAX && x >= -DBL_MAX);
}

double getExposureTime(string fileName) {
	FILE* fp = fopen(fileName.c_str(), "rb");
	fseek(fp, 0, SEEK_END);
	unsigned long size = ftell(fp);
	rewind(fp);
	unsigned char* buffer = new unsigned char[size];
	if (fread(buffer, 1, size, fp) != size) {
		cout << "Error!" << endl;
		return -1;
	}
	fclose(fp);

	EXIFInfo info;
	ParseEXIF(buffer, size, info);

	return info.exposureTime;
}