#pragma once

#include "HDRMethod.h"

class ToneMapping{
	public:
		static void Bilateral(Mat& input, Mat& output);
		static void Reinhard(Mat &input, Mat &output);
		static void Gradient(Mat &input, Mat &output);
		static void Attenuate(Mat &input, Mat &output);
		static Mat localOperator(Mat& image, double sat, double a);
};