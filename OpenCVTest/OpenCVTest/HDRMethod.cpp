#include "stdafx.h"
#include "HDRMethod.h"

int HDR::iteration = 7;
double HDR::lambda = 50;
int HDR::sampleNum = 99;

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

Mat HDR::getMedianBitmap(Mat org, uchar& threshold) {
	Mat mbp = org.clone();
	int histogram[256];
	memset(histogram, 0, sizeof(histogram));
	int row = org.rows, col = org.cols;
	for (int i = 0; i < row; i++) {
		for (int j = 0; j < col; j++) {
			histogram[(int)org.at<uchar>(i, j)]++;
		}
	}

	int total = row * col;
	int now = 0;
	threshold = -1;
	for (int i = 0; i < 256; i++) {
		now += histogram[i];
		if (now >= total / 2) {
			threshold = i;
			break;
		}
	}

	for (int i = 0; i < row; i++) {
		for (int j = 0; j < col; j++) {
			mbp.at<uchar>(i, j) = org.at<uchar>(i, j) >= threshold ? 255 : 0;
		}
	}

	//cout << (int)threshold << endl;
	return mbp;
}



vector<pair<int, int>> HDR::MTBAlgo(vector<Mat>& Images, vector<Mat>& grayImages, vector<uchar>& thresholds) {
	vector<pair<int, int>> res;
	res.push_back(make_pair(0, 0));
	for (int i = 1; i < Images.size(); i++) {
		int xOffset = 0, yOffset = 0;
		for (int j = iteration; j >= 1; j--) {
			Mat ref0 = Images[i-1], ref;
			Mat now0 = Images[i], now;
			Mat refg0 = grayImages[i-1], refg;
			Mat nowg0 = grayImages[i], nowg;

			int displacement = pow(2, j);
			int newRow = ref0.rows / displacement, newCol = ref0.cols / displacement;

			//cout << newRow << " " << newCol << endl;
			refg0 = translateImg(refg0, res[i - 1].first, res[i - 1].second);
			ref0 = translateImg(ref0, res[i - 1].first, res[i - 1].second);
			resize(ref0, ref, Size(newCol, newRow), 0, 0, INTER_AREA);
			resize(now0, now, Size(newCol, newRow), 0, 0, INTER_AREA);
			resize(refg0, refg, Size(newCol, newRow), 0, 0, INTER_AREA);
			resize(nowg0, nowg, Size(newCol, newRow), 0, 0, INTER_AREA);
			//pyrDown(now0, now, Size(newCol, newRow));
			
			//imwrite("down1"+to_string(i)+".jpg", ref);
			imwrite("down2" + to_string(i) + ".jpg", now);
			//imwrite("down3" + to_string(i) + ".jpg", refg);
			imwrite("down4" + to_string(i) + ".jpg", nowg);
			

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
							// need adjustment
							if (abs(refg.at<uchar>(ii, jj) - thresholds[i]) <= 10) {
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
						//cout << "offset: " << x - xOffset << " " << y - yOffset << endl;
						minError = totalError;
					}
					cout << totalError<<"("<<minError<<")" << " ";
				}
			}
			cout << endl;
			cout << tmpOffsetX << " " << tmpOffsetY << endl;

			bool modifiedX = false, modifiedY = false;
			
			if (minError * 1.5 >= centerError) {
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
				if (j != 0) {
					xOffset *= 2;
				}
			}
			if (modifiedY) {
				yOffset += tmpOffsetY;
				if (j != 0) {
					yOffset *= 2;
				}
			}

		}
		res.push_back(make_pair(xOffset, yOffset));
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