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

class HDR {
	public:
		static Mat translateImg(Mat &img, int offsetx, int offsety);
		static Mat getMedianBitmap(Mat& org, Mat& dst);
		static vector<pair<int, int>> MTBAlgo(vector<Mat>& Images, vector<Mat>& grayImages, vector<Mat>& src, vector<Mat>& noise);
		static double weightingMethod(int z);

		static int iteration;
		static double lambda;
		static int sampleNum;
};

