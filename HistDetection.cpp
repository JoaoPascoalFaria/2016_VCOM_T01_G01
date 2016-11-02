
#include <string.h>
#include "opencv2/imgproc/imgproc.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <opencv/cv.h>
#include <opencv/cxcore.h>
#include <opencv/cvaux.h>
#include <opencv/highgui.h>
#include <opencv2/opencv.hpp>
#include <math.h>

using namespace cv;
using namespace std;


int CCX[] = {37,58,82,58,217};
int CCY[] = {88,89,129,15,89};

const int car_width = 17;
const int car_len = 40;
const int number_of_parking_slots = 5;

int n;

int main( int argc, char** argv ) {

	Mat img , roiImg, roiImage;

	img = cv::imread(argv[1], 1);

	// base roi of empty parking slot
	Rect rec(37,88,car_width,car_len); 
	roiImage=img(rec);
	cvtColor(roiImage,roiImage,CV_BGR2GRAY);
	equalizeHist(roiImage,roiImage);
	roiImage = img(rec);

	//calculate histogram of base roi
	MatND hist_baseRoi;
	int nbins = 256;
	int histSize[] = {nbins};
	float range[] = {0, 255};
	const float *ranges[] = {range};
	int channels[] = {0};
	calcHist(&roiImage, 1, channels, Mat(), hist_baseRoi, 1, histSize, ranges, true, false);
	normalize(hist_baseRoi, hist_baseRoi, 0, 1,NORM_MINMAX, -1, Mat());

	int busy = 0;

	//loop threw parking spots
	for (n=0; n < number_of_parking_slots; n++) {

		Rect roi(CCX[n], CCY[n], car_width, car_len);
		roiImg=img(roi);
		cvtColor(roiImg,roiImg,CV_BGR2GRAY);

		equalizeHist(roiImg,roiImg);
		MatND hist_roi;

		int nbins = 256;
		int histSize[] = {nbins};
		float range[] = {0, 255};
		const float *ranges[] = {range};
		int channels[] = {0};
		calcHist(&roiImg, 1, channels, Mat(), hist_roi, 1, histSize, ranges, true, false);
		normalize(hist_roi, hist_roi, 0, 1, NORM_MINMAX, -1, Mat());

		//compare histograms of base roi and current roi using Chi-Square
		double hist_comp = compareHist(hist_roi, hist_baseRoi, CV_COMP_CHISQR);

		//determine wether the parking spot is occupied or not based on the return value from compareHist
		if(hist_comp > 7) {
			printf(" [%d],  %f \n", n, hist_comp);
			rectangle(img, Point(CCX[n],CCY[n]), Point(CCX[n]+car_width,CCY[n]+car_len), Scalar(0,0,255),0.3, 8);
			busy++;
		} else {
			printf(" [%d],  %f \n", n, hist_comp );
			rectangle(img, Point(CCX[n],CCY[n]), Point(CCX[n]+car_width,CCY[n]+car_len), Scalar(0,255,0),0.3, 8);
		}

	}

	cout << endl << number_of_parking_slots - busy << " out of " << busy << " parking slots are free." << endl;
	imshow("PARKING SPOT", img);
	waitKey(0);

	return 0;
}