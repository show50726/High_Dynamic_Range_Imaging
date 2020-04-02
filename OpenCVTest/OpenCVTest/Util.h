#pragma once
#pragma warning(disable:4996)

#include <iostream>
#include <cmath>
#include <opencv2/opencv.hpp>
#include <fstream>
#include <random>
#include <time.h>
#include "exif.h"

using namespace std;
using namespace cv;


bool isFinite(double x);
double getExposureTime(string fileName);
