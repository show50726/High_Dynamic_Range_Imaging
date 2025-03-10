#include "stdafx.h"
#pragma warning(disable:4996)

#include "HDRMethod.h"
#include "Util.h"
#include "ToneMapping.h"
#include <stdio.h> 
#include <stdlib.h>
#include <opencv2\imgproc\types_c.h>

//#define AUTOFILL
//#define FILLSAMPLE
//#define ALINEMENT

using namespace std;
using namespace cv;

int _tmain(int argc, _TCHAR* argv[])
{
	srand(time(NULL));

	int n, row, col;
	string baseName, type;

#ifdef AUTOFILL

	n = 6;
	baseName = "D";
	type = ".jpg";

#else

	cout << "Please enter the number of the pictures: ";
	cin >> n;
	cout << "Please enter the base name of the pictures: ";
	cin >> baseName;
	cout << "Please enter the base file type (EX. \".jpg\"...) of the pictures: ";
	cin >> type;

#endif // AUTOFILL


	vector<Mat> Images;
	vector<Mat> grayImages;

	vector<double> exposureTimes(n);
	vector<double> lnexposureTimes;

	vector<pair<int, int>> offsetXY;
	vector<Mat> alineImages;

	for (int i = 0; i < n; i++) {
		string s = baseName + to_string(i) + type;
		
		double exposureT = getExposureTime(s);
		exposureTimes[i] = exposureT;
		lnexposureTimes.push_back(log(exposureTimes[i]) / log(2.718285));

		cout << s << endl;
		
		Mat img = imread(s);
		
		if (!img.data) {
			cout << "Could not open or find image "<< s << endl;
			return -1;
		}
		
		Images.push_back(img);
		alineImages.push_back(img.clone());
		grayImages.push_back(img.clone());
		cvtColor(grayImages[i], grayImages[i], CV_BGR2GRAY);
	}


	row = Images[0].rows;
	col = Images[0].cols;

	vector<Mat> medianImages;
	vector<Mat> noise(n);
	for (int i = 0; i < n; i++) {
		Mat img;
		noise[i] = HDR::getMedianBitmap(grayImages[i], img);
		medianImages.push_back(img);
	}

	

#ifdef ALINEMENT
	offsetXY = HDR::MTBAlgo(medianImages, grayImages, alineImages, noise);
#endif // ALINEMENT

	/*
	for (int i = 0; i < offsetXY.size(); i++) {
		cout << offsetXY[i].first << " " << offsetXY[i].second << endl;
		imwrite("aline" + to_string(i)+".jpg", alineImages[i]);
	}
	*/

	vector<vector<int>> rZ(n);
	vector<vector<int>> gZ(n);
	vector<vector<int>> bZ(n);


	Mat showImg = Images[0].clone();
	for (int j = 0; j < HDR::sampleNum; j++) {
		int randR;
		int randC;

#ifdef FILLSAMPLE
		cin >> randC >> randR;
#else
		randR = rand() % row;
		randC = rand() % col;
#endif // FILLSAMPLE

		circle(showImg, Point(randC, randR), 0.1, Scalar(0, 0, 255), 2);
		for (int i = 0; i < n; i++) {
			bZ[i].push_back(Images[i].at<Vec3b>(randR, randC)[0]);
			gZ[i].push_back(Images[i].at<Vec3b>(randR, randC)[1]);
			rZ[i].push_back(Images[i].at<Vec3b>(randR, randC)[2]);
		}
	}


	// start filling the Ax=b format
	vector<vector<double>> rA(HDR::sampleNum*n + 255, vector<double>(256 + HDR::sampleNum, 0));
	vector<double> rb(HDR::sampleNum*n + 255, 0);
	vector<vector<double>> gA(HDR::sampleNum*n + 255, vector<double>(256 + HDR::sampleNum, 0));
	vector<double> gb(HDR::sampleNum*n + 255, 0);
	vector<vector<double>> bA(HDR::sampleNum*n + 255, vector<double>(256 + HDR::sampleNum, 0));
	vector<double> bb(HDR::sampleNum*n + 255, 0);
	//cout << rA.size() << " " << rA[0].size() << endl;
	
	for (int i = 0; i < n; i++) {
		for (int j = 0; j < HDR::sampleNum; j++) {
			double rw = HDR::weightingMethod(rZ[i][j]);
			rA[i*HDR::sampleNum + j][rZ[i][j]] = 1 * rw;
			rA[i*HDR::sampleNum + j][256 + j] = -1 * rw;
			rb[i*HDR::sampleNum + j] = -lnexposureTimes[i] * rw;

			double gw = HDR::weightingMethod(gZ[i][j]);
			gA[i*HDR::sampleNum + j][gZ[i][j]] = 1 * gw;
			gA[i*HDR::sampleNum + j][256 + j] = -1 * gw;
			gb[i*HDR::sampleNum + j] = -lnexposureTimes[i] * gw;

			double bw = HDR::weightingMethod(bZ[i][j]);
			bA[i*HDR::sampleNum + j][bZ[i][j]] = 1 * bw;
			bA[i*HDR::sampleNum + j][256 + j] = -1 * bw;
			bb[i*HDR::sampleNum + j] = -lnexposureTimes[i] * bw;

			//cout << rZ[i][j] << " " << gZ[i][j] << " " << bZ[i][j] << endl;
		}
	}


	// g(127) = 0
	rA[HDR::sampleNum*n][127] = 1;
	rb[HDR::sampleNum*n] = 0.0;
	gA[HDR::sampleNum*n][127] = 1;
	gb[HDR::sampleNum*n] = 0.0;
	bA[HDR::sampleNum*n][127] = 1;
	bb[HDR::sampleNum*n] = 0.0;

	for (int i = 1; i < 255; i++) {
		double w = HDR::weightingMethod(i);
		rA[HDR::sampleNum*n + i][i - 1] = HDR::lambda * 1 * w;
		rA[HDR::sampleNum*n + i][i] = HDR::lambda * -2 * w;
		rA[HDR::sampleNum*n + i][i + 1] = HDR::lambda * 1 * w;

		gA[HDR::sampleNum*n + i][i - 1] = HDR::lambda * 1 * w;
		gA[HDR::sampleNum*n + i][i] = HDR::lambda * -2 * w;
		gA[HDR::sampleNum*n + i][i + 1] = HDR::lambda * 1 * w;

		bA[HDR::sampleNum*n + i][i - 1] = HDR::lambda * 1 * w;
		bA[HDR::sampleNum*n + i][i] = HDR::lambda * -2 * w;
		bA[HDR::sampleNum*n + i][i + 1] = HDR::lambda * 1 * w;
	}


	Mat rx(256 + HDR::sampleNum, 1, CV_64F);
	Mat gx(256 + HDR::sampleNum, 1, CV_64F);
	Mat bx(256 + HDR::sampleNum, 1, CV_64F);

	Mat A(rA.size(), rA[0].size(), CV_64F);
	Mat b(rb.size(), 1, CV_64F);

	for (int i = 0; i < A.rows; i++) {
		for (int j = 0; j < A.cols; j++) {
			A.at<double>(i, j) = rA[i][j];
		}
		b.at<double>(i, 0) = rb[i];
		//cout << b.at<double>(i, 0) << endl;
	}

	solve(A, b, rx, DECOMP_SVD);
	for (int i = 0; i < 256; i++) {
		rx.at<double>(i, 0) *= -1;
	}

	for (int i = 0; i < A.rows; i++) {
		for (int j = 0; j < A.cols; j++) {
			A.at<double>(i, j) = gA[i][j];
		}
		b.at<double>(i, 0) = gb[i];
	}
	solve(A, b, gx, DECOMP_SVD);
	for (int i = 0; i < 256; i++) {
		gx.at<double>(i, 0) *= -1;
	}

	for (int i = 0; i < A.rows; i++) {
		for (int j = 0; j < A.cols; j++) {
			A.at<double>(i, j) = bA[i][j];
		}
		b.at<double>(i, 0) = bb[i];
	}
	solve(A, b, bx, DECOMP_SVD);
	for (int i = 0; i < 256; i++) {
		bx.at<double>(i, 0) *= -1;
	}

	for (int i = 0; i < 256; i++) {
		cout << i << ": " << rx.at<double>(i, 0) << " " << gx.at<double>(i, 0) << " " << bx.at<double>(i, 0) << endl;
	}

	ofstream outputFile1;
	outputFile1.open("response_curve.csv");
	ofstream fsOutput1("response_curve.csv", ios::app);

	for (int i = 0; i < 256; i++) {
		fsOutput1 << i << "," << rx.at<double>(i, 0) << "," << gx.at<double>(i, 0) << "," << bx.at<double>(i, 0) << endl;
	}
	outputFile1.close();

	Mat hdr_image(row, col, CV_32FC3);
	for (int i = 0; i < row; i++) {
		for (int j = 0; j < col; j++) {
			double rsum = 0, rweightSum = 0;
			double gsum = 0, gweightSum = 0;
			double bsum = 0, bweightSum = 0;
			for (int k = 0; k < n; k++) {
				double rw = HDR::weightingMethod(Images[k].at<Vec3b>(i, j)[2]);
				rweightSum += rw;
				rsum += rw * (rx.at<double>((int)Images[k].at<Vec3b>(i, j)[2], 0) - lnexposureTimes[k]);

				double gw = HDR::weightingMethod(Images[k].at<Vec3b>(i, j)[1]);
				gweightSum += gw;
				gsum += gw * (gx.at<double>((int)Images[k].at<Vec3b>(i, j)[1], 0) - lnexposureTimes[k]);

				double bw = HDR::weightingMethod(Images[k].at<Vec3b>(i, j)[0]);
				bweightSum += bw;
				bsum += bw * (bx.at<double>((int)Images[k].at<Vec3b>(i, j)[0], 0) - lnexposureTimes[k]);
			}

			if (abs(rweightSum) == 0.0 || !isFinite((float)exp(rsum / rweightSum))) {
				hdr_image.at<Vec3f>(i, j)[2] = 0.0;
			}
			else {
				hdr_image.at<Vec3f>(i, j)[2] = (float)exp(rsum / rweightSum);
			}

			if (abs(gweightSum) == 0.0 || !isFinite((float)exp(gsum / gweightSum))) {
				hdr_image.at<Vec3f>(i, j)[1] = 0.0;
			}
			else {
				hdr_image.at<Vec3f>(i, j)[1] = (float)exp(gsum / gweightSum);
			}

			if (abs(bweightSum) == 0.0 || !isFinite((float)exp(bsum / bweightSum))) {
				hdr_image.at<Vec3f>(i, j)[0] = 0.0;
			}
			else {
				hdr_image.at<Vec3f>(i, j)[0] = (float)exp(bsum / bweightSum);
			}
			//cout << hdr_image.at<Vec3f>(i, j)[0] << endl;

		}
	}

	imwrite("hdrimg.exr", hdr_image);

	Mat rein, bilaterial, gradient;
	ToneMapping::Reinhard(hdr_image, rein);
	ToneMapping::Bilateral(hdr_image, bilaterial);
	//ToneMapping::Gradient(hdr_image, gradient);

	imshow("hdr", rein);
	imshow("hdr_1", bilaterial);
	//imshow("hdr_2", gradient);
	imwrite("hdr_rein.jpg", rein);
	imwrite("hdr_bilaterial.jpg", bilaterial);
	//imwrite("hdr_gradient.jpg", gradient);
	
	
	waitKey(0);

	return 0;
}
