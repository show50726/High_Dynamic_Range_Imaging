#pragma once

#include "HDRMethod.h"

class ToneMapping{
	public:
		static void Bilaterial(Mat& input, Mat& output);
		static void Reinhard(Mat &input, Mat &output);
};