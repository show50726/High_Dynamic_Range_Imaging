#include "stdafx.h"
#include "ToneMapping.h"

void ToneMapping::Bilateral(Mat& input, Mat& output) {
	int row = input.rows, col = input.cols;
	double sigmaColor = 0.4, sigmaSpace = 0.02 * min(row, col);
	// double sigmaColor = 0.5, sigmaSpace = 0.02 * min(row, col);

	Mat intensity(row, col, CV_32FC1);
	Mat logintensity = intensity.clone();
	for (int i = 0; i < row; i++) {
		for (int j = 0; j < col; j++) {
			intensity.at<float>(i, j) = 0.0722 * input.at<Vec3f>(i, j)[0] + 0.7152 * input.at<Vec3f>(i, j)[1] + 0.2126 * input.at<Vec3f>(i, j)[2];
			logintensity.at<float>(i, j) = log(intensity.at<float>(i, j)) / log(10);
		}
	}

	Mat lowF, highF;
	float minv = 1000010, maxv = -1000010;

	bilateralFilter(logintensity, lowF, -1, sigmaColor, sigmaSpace);
	// imwrite("lowf.jpg", lowF);

	highF = intensity.clone();
	for (int i = 0; i < row; i++) {
		for (int j = 0; j < col; j++) {
			highF.at<float>(i, j) = logintensity.at<float>(i, j) - lowF.at<float>(i, j);
			minv = min(minv, lowF.at<float>(i, j));
			maxv = max(maxv, lowF.at<float>(i, j));
		}
	}

	float compress = (log(8)) / (maxv - minv);
	//float compress = (log(5) / log(10)) / (maxv - minv);
	float scale = compress*maxv;
	//float scale = 1.0 / pow(10, compress*maxv);

	Mat final = lowF.clone();
	for (int i = 0; i < row; i++) {
		for (int j = 0; j < col; j++) {
			final.at<float>(i, j) = (float)pow(10.0, lowF.at<float>(i, j)*compress + highF.at<float>(i, j) - scale);
		}
	}


	Mat res(row, col, CV_8UC3);
	for (int i = 0; i < row; i++) {
		for (int j = 0; j < col; j++) {
			for (int c = 0; c < 3; c++) {
				res.at<Vec3b>(i, j)[c] = saturate_cast<uchar>(input.at<Vec3f>(i, j)[c] / intensity.at<float>(i, j) * final.at<float>(i, j) * 255.0);
				//res.at<Vec3b>(i, j)[c] = saturate_cast<uchar>(pow((double)res.at<Vec3b>(i, j)[c] / 255.0, 2.2)*255.0);
			}
		}
	}

	output = res.clone();
	return;
}


//TODO: local operators
void ToneMapping::Reinhard(Mat &input, Mat &output)
{
	//output = localOperator(input, 1, 1);
	//return;

	double a = 0.15, Lwhite = 100;
	int row = input.rows, col = input.cols;

	Mat intensity(row, col, CV_32FC1);
	for (int i = 0; i < row; i++) {
		for (int j = 0; j < col; j++) {
			intensity.at<float>(i, j) = 0.0722 * input.at<Vec3f>(i, j)[0] + 0.7152 * input.at<Vec3f>(i, j)[1] + 0.2126 * input.at<Vec3f>(i, j)[2];
		}
	}

	double _lw = 0;
	for (int i = 0; i < row; i++) {
		for (int j = 0; j < col; j++) {
			_lw += log(0.000001 + intensity.at<float>(i, j));
		}
	}
	_lw /= (row*col);
	_lw = exp(_lw);

	Mat L = intensity.clone();
	for (int i = 0; i < row; i++) {
		for (int j = 0; j < col; j++) {
			double lm = a / _lw * intensity.at<float>(i, j);
			L.at<float>(i, j) = lm * (1 + lm / (Lwhite*Lwhite)) / (1 + lm);
		}
	}

	Mat res(row, col, CV_8UC3);
	for (int i = 0; i < row; i++) {
		for (int j = 0; j < col; j++) {
			for (int c = 0; c < 3; c++) {
				res.at<Vec3b>(i, j)[c] = saturate_cast<uchar>(input.at<Vec3f>(i, j)[c] * (L.at<float>(i, j)*255.0 / intensity.at<float>(i, j)));
			}
		}
	}

	output = res.clone();
	return;
}

void ToneMapping::Gradient(Mat &input, Mat &output) {
	int row = input.rows, col = input.cols;

	Mat intensity(row, col, CV_32FC1);
	Mat logintensity = intensity.clone();
	for (int i = 0; i < row; i++) {
		for (int j = 0; j < col; j++) {
			intensity.at<float>(i, j) = 0.0722 * input.at<Vec3f>(i, j)[0] + 0.7152 * input.at<Vec3f>(i, j)[1] + 0.2126 * input.at<Vec3f>(i, j)[2];
			logintensity.at<float>(i, j) = log(intensity.at<float>(i, j)) / log(10);
		}
	}

	Mat dervX = logintensity.clone();
	Mat dervY = logintensity.clone();
	dervX.at<float>(0, 0) = 0;
	dervY.at<float>(0, 0) = 0;
	for (int i = 1; i < row; i++) {
		dervX.at<float>(i, 0) = logintensity.at<float>(i, 0) - logintensity.at<float>(i - 1, 0);
	}
	for (int i = 1; i < col; i++) {
		dervY.at<float>(0, i) = logintensity.at<float>(0, i) - logintensity.at<float>(0, i - 1);
	}

	for (int i = 0; i < row; i++) {
		for (int j = 1; j < col; j++) {
			dervX.at<float>(i, j) = logintensity.at<float>(i, j) - logintensity.at<float>(i, j-1);
		}
	}
	for (int i = 1; i < row; i++) {
		for (int j = 0; j < col; j++) {
			dervY.at<float>(i, j) = logintensity.at<float>(i, j) - logintensity.at<float>(i - 1, j);
		}
	}

	Attenuate(logintensity, logintensity);

	Mat A((row - 1)*(col - 1), row*col, CV_32F);
	Mat b(A.cols, 1, CV_32F);
	Mat x(A.cols, 1, CV_32F);
	int index = 0;
	for (int i = 0; i < row; i++) {
		for (int j = 0; j < col; j++) {
			A.at<float>(index, (i + 1)*col + j) = 1;
			A.at<float>(index, (i - 1)*col + j) = 1;
			A.at<float>(index, i*col + j + 1) = 1;
			A.at<float>(index, i*col + j - 1) = 1;
			A.at<float>(index, i*col + j) = -4;

			b.at<float>(index, 0) = dervX.at<float>(i, j) - dervX.at<float>(i - 1, j) + dervY.at<float>(i, j) - dervY.at<float>(i, j - 1);
		}
	}

	solve(A, b, x, DECOMP_SVD);

	Mat res = logintensity.clone();
	for (int i = 0; i < row; i++) {
		for (int j = 0; j < col; j++) {
			res.at<float>(i, j) = exp(x.at<float>(i*col + j, 0));
		}
	}

	Mat final(row, col, CV_8UC3);
	for (int i = 0; i < row; i++) {
		for (int j = 0; j < col; j++) {
			for (int c = 0; c < 3; c++) {
				final.at<Vec3b>(i, j)[c] = saturate_cast<uchar>(input.at<Vec3f>(i, j)[c] * (res.at<float>(i, j) / intensity.at<float>(i, j)*255.0));
			}
		}
	}

	output = final.clone();
	return;
}

Mat ToneMapping::localOperator(Mat& image, double sat, double a) {
	int row = image.rows;
	int col = image.cols;
	int n = row * col;

	double sum = 0;
	Mat intensity(row, col, CV_32FC1);
	Mat logintensity(row, col, CV_32FC1);
	for (int i = 0; i < row; i++) {
		for (int j = 0; j < col; j++) {
			intensity.at<float>(i, j) = 0.0722 * image.at<Vec3f>(i, j)[0] + 0.7152 * image.at<Vec3f>(i, j)[1] + 0.2126 * image.at<Vec3f>(i, j)[2];
			logintensity.at<float>(i, j) = log(intensity.at<float>(i, j)) / log(2.718285);
			sum += logintensity.at<float>(i, j);
		}
	}
	sum /= (row*col);

	for (int i = 0; i < row; i++) {
		for (int j = 0; j < col; j++) {
			intensity.at<float>(i, j) /= exp(sum);
		}
	}

	double thres = 0.0001;

	double level = 8, phi = 1;
	vector<Mat> Vs;
	vector<Mat> Ls_blur;
	Mat now = intensity.clone();
	for (int i = 0; i < level; i++) {
		GaussianBlur(now, now, Size(43, 43), 1*i, 1*i);
		//imwrite("now" + to_string(i) + ".jpg", now);
		Ls_blur.push_back(now.clone());
	}

	Mat final = intensity.clone();
	
	for (int i = 0; i < row; i++) {
		for (int j = 0; j < col; j++) {
			for (int i = level - 2; i >= 0; i--) {
				double now = (Ls_blur[i].at<float>(i, j) - Ls_blur[i + 1].at<float>(i, j)) / (0.00001 + Ls_blur[i].at<float>(i, j));
				//double now = (Ls_blur[i].at<float>(i, j) - Ls_blur[i + 1].at<float>(i, j)) / (pow(2, phi)*a / (i*i) + Ls_blur[i].at<float>(i, j));
				//cout << now << endl;
				if (abs(now) < thres) {
					final.at<float>(i, j) = intensity.at<float>(i, j) / (1 + Ls_blur[i].at<float>(i, j));
					break;
				}
			}
		}
	}

	Mat res(row, col, CV_8UC3);
	for (int i = 0; i < row; i++) {
		for (int j = 0; j < col; j++) {
			for (int c = 0; c < 3; c++) {
				res.at<Vec3b>(i, j)[c] = saturate_cast<uchar>(5*image.at<Vec3f>(i, j)[c] * (final.at<float>(i, j)*255 / intensity.at<float>(i, j)));
			}
		}
	}

	return res;

}

Mat convolution(Mat& image, Mat& mask) {
	Mat input = image.clone();
	int row = input.rows;
	int col = input.cols;

	for (int i = 0; i < row; i++) {
		for (int j = 0; j < col; j++) {
			double sum = 0;
			for (int ii = 0; ii < mask.rows; ii++) {
				for (int jj = 0; jj < mask.cols; jj++) {
					int r = i + ii, c = j + jj;
					if (r >= 0 && r < row && c >= 0 && c < col) {
						sum += input.at<float>(r, c)*mask.at<float>(ii, jj);
					}
				}
			}
			input.at<float>(i, j) = sum;
		}
	}
	return input;
}


void ToneMapping::Attenuate(Mat& input, Mat& output) {
	int row = input.rows, col = input.cols;
	Mat res = input.clone();

	for (int i = 0; i < row; i++) {
		for (int j = 0; j < col; j++) {
			res.at<float>(i, j) *= 0.4;
		}
	}

	output = res.clone();
}
