#pragma once

#include <iostream>
#include <cmath>
#include <opencv2/opencv.hpp>
#include <fstream>
#include <random>
#include <time.h>
#include "exif.h"

using namespace std;
using namespace cv;

struct point {
	int row;
	int col;
	int r;
	int g;
	int b;
	void operator = (const point& p) {
		row = p.row;
		col = p.col;
		r = p.r;
		g = p.g;
		b = p.b;
	}
};

class HDR {
	public:
		static Mat translateImg(Mat &img, int offsetx, int offsety);
		static Mat getMedianBitmap(Mat org, uchar& threshold);
		static vector<pair<int, int>> MTBAlgo(vector<Mat>& Images, vector<Mat>& grayImages, vector<uchar>& thresholds);
		static double weightingMethod(int z);

		static int iteration;
		static double lambda;
		static int sampleNum;
};

