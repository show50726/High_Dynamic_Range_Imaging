#include "stdafx.h"
#include <opencv2/photo.hpp>
#include "HDRMethod.h"

int HDR::iteration = 7;
double HDR::lambda = 50;
int HDR::sampleNum = 200;

const int Z_MAX = 255;
const int Z_MIN = 0;

Mat HDR::translateImg(Mat &img, int offsetx, int offsety) {
	if (offsetx == 0 && offsety == 0) {
		return img.clone();
	}
	Mat trans_mat = (Mat_<double>(2, 3) << 1, 0, offsetx, 0, 1, offsety);
	warpAffine(img, img, trans_mat, img.size());
	return img;
}

Mat HDR::getMedianBitmap(Mat& org, Mat& dst) {
	Mat mbp = org.clone();
	Mat noise = org.clone();
	int histogram[256];
	memset(histogram, 0, sizeof(histogram));
	int row = org.rows, col = org.cols;
	int maxn = -1, minn = 256;
	for (int i = 0; i < row; i++) {
		for (int j = 0; j < col; j++) {
			// cout << (int)org.at<uchar>(i, j) << endl;
			histogram[(int)org.at<uchar>(i, j)]++;
			maxn = max(maxn, (int)org.at<uchar>(i, j));
			minn = min(minn, (int)org.at<uchar>(i, j));
		}
	}

	int l;

	float total = row * col;
	float now = 0;
	int threshold = -1;
	for (int i = 0; i < 256; i++) {
		now += histogram[i];
		if (now >= total * 3 / 10) {
			l = i;
		}
		if (now >= total / 2) {
			threshold = i;
			break;
		}
	}
	l = threshold - l;

	for (int i = 0; i < row; i++) {
		for (int j = 0; j < col; j++) {
			mbp.at<uchar>(i, j) = org.at<uchar>(i, j) > threshold ? 255 : 0;
			noise.at<uchar>(i, j) = (org.at<uchar>(i, j) >= threshold - l && org.at<uchar>(i, j) <= threshold + l) ? 0 : 255;
		}
	}

	//cout << (int)threshold << endl;
	dst = mbp;
	return noise;
}


vector<pair<int, int>> HDR::MTBAlgo(vector<Mat>& Images, vector<Mat>& grayImages, vector<Mat>& src, vector<Mat>& noise) {
	vector<pair<int, int>> res;
	res.push_back(make_pair(0, 0));

	for (int i = 1; i < Images.size(); i++) {
		int xOffset = 0, yOffset = 0;
		for (int j = iteration; j >= 1; j--) {
			Mat ref0 = Images[i-1], ref;
			Mat now0 = Images[i], now;
			Mat refg0 = grayImages[i-1], refg;
			Mat nowg0 = grayImages[i], nowg;
			Mat noise0 = noise[i - 1], noise1 = noise[i];

			int displacement = pow(2, j);
			int newRow = ref0.rows / displacement, newCol = ref0.cols / displacement;

			resize(ref0, ref, Size(newCol, newRow), 0, 0, INTER_NEAREST);
			resize(now0, now, Size(newCol, newRow), 0, 0, INTER_NEAREST);
			resize(refg0, refg, Size(newCol, newRow), 0, 0, INTER_NEAREST);
			resize(nowg0, nowg, Size(newCol, newRow), 0, 0, INTER_NEAREST);
			resize(noise0, noise0, Size(newCol, newRow), 0, 0, INTER_NEAREST);
			resize(noise1, noise1, Size(newCol, newRow), 0, 0, INTER_NEAREST);
			
			 // imwrite("down1"+to_string(i)+".jpg", ref);
			 // imwrite("down2" + to_string(i) + ".jpg", noise0);
			 // imwrite("down3" + to_string(i) + ".jpg", refg);
			 // imwrite("down4" + to_string(i) + ".jpg", nowg);
			

			int tmpOffsetX = 0, tmpOffsetY = 0;
			double centerError = 0;
			double totalError = 0, minError = 2147483640.0;
			for (int y = -1; y <= 1; y++) {
				for (int x = -1; x <= 1; x++) {
					totalError = 0;
					for (int ii = 0; ii < newRow; ii++) {
						for (int jj = 0; jj < newCol; jj++) {
							int xx = jj + x + xOffset, yy = ii + y + yOffset;
							if (xx < 0 || xx >= newCol || yy < 0 || yy >= newRow) {
								continue;
							}
							if (noise0.at<uchar>(ii, jj) == 0 || noise1.at<uchar>(yy, xx) == 0) {
								continue;
							}
							totalError += (ref.at<uchar>(ii, jj) == now.at<uchar>(yy, xx) ? 0 : 1);
						}
					}
					if (y == 0 & x == 0) {
						centerError = totalError;
					}

					if (totalError < minError) {
						tmpOffsetX = -x;
						tmpOffsetY = -y;
						minError = totalError;
					}
					// cout << totalError<<"("<<minError<<")" << " ";
				}
			}

			bool modifiedX = false, modifiedY = false;
			
			if (minError*1.025 >= centerError) {
				tmpOffsetX = 0;
				tmpOffsetY = 0;
			}
			else if (tmpOffsetX != 0 || tmpOffsetY != 0) {
				if (tmpOffsetX != 0) {
					modifiedX = true;
				}
				if (tmpOffsetY != 0) {
					modifiedY = true;
				}
			}

			//cout << "tmp: " << tmpOffsetX << " " << tmpOffsetY << " " << xOffset << " " << yOffset << endl;
			
			if (modifiedX) {
				xOffset += tmpOffsetX;
				xOffset *= 2;
			}
			if (modifiedY) {
				yOffset += tmpOffsetY;
				yOffset *= 2;
			}

		}
		res.push_back(make_pair(xOffset, yOffset));
		Images[i] = translateImg(Images[i], xOffset, yOffset);
		src[i] = translateImg(src[i], xOffset, yOffset);
		noise[i] = translateImg(noise[i], xOffset, yOffset);
		grayImages[i] = translateImg(grayImages[i], xOffset, yOffset);
	}

	return res;
}

double HDR::weightingMethod(int z) {
	if (z <= (Z_MIN + Z_MAX + 1) / 2) {
		return ((double)z - Z_MIN) < 8 ? 8 : ((double)z - Z_MIN);
	}
	else {
		return ((double)Z_MAX - z) < 8 ? 8 : ((double)Z_MAX - z);
	}
}