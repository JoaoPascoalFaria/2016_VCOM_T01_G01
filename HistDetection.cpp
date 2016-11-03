
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
#include <vector>

using namespace cv;
using namespace std;

#define ARRAY_SIZE(array) (sizeof((array))/sizeof((array[0])))

int getNumberOfFreeSpots(Mat &img);

int base_spot_x = 37;
int base_spot_y = 88;

Mat initial_img, img;

vector<int> ccx;
vector<int> ccy;

int car_width = 17;
int car_len = 27;
const int number_of_parking_slots = 5;

int n;

int STATE = 0;

void CallBackFunc(int event, int x, int y, int flags, void* userdata)
{
		if  ( event == EVENT_LBUTTONDOWN )
		{
			if (STATE == 0) {
				x -= car_width/2;
				y -= car_len/2;
				base_spot_x = x;
				base_spot_y = y;
				rectangle( img, Point(x,y), Point(x+car_width,y+car_len), Scalar(255,0,255),0.3, 8);
				imshow("Parking System", img);

				cout << "Base empty parking spot sucessfully selected." << endl 
					<< "----------------" << endl
					<< "Now please select the parking spots with MOUSE1." << endl
					<< "Afterwards press MOUSE2 to process the parking lot for free spots." << endl;

				STATE = 1;

			} else if (STATE == 1) {
				x -= car_width/2;
				y -= car_len/2;
				ccx.push_back(x);
				ccy.push_back(y);
				cout << "Added parking spot at " << "(" << x << ", " << y << ")" << endl;
				rectangle( img, Point(x,y), Point(x+car_width,y+car_len), Scalar(255,255,0),0.3, 8);
				imshow("Parking System", img);
			}

			
		}
		else if  ( event == EVENT_RBUTTONDOWN )
		{
			if (STATE == 1) {

				cout << "Processing..." << endl;
				initial_img.copyTo(img);
				getNumberOfFreeSpots(img);
				imshow("Parking System", img);

				STATE = 2;

			}
		}
		else if  ( event == EVENT_MBUTTONDOWN )
		{
			ccx.clear();
			ccy.clear();
			initial_img.copyTo(img);
			imshow("Parking System", img);
			cout << "Cleared Spots." << endl;
			cout << endl << "----------------" << endl << "Select the base empty parking spot with MOUSE1." << endl;

			STATE = 0;
		}
		/*else if ( event == EVENT_MOUSEMOVE )
		{
			cout << "Mouse move over the window - position (" << x << ", " << y << ")" << endl;

		}/**/
}

int slider = 17;
const int slider_max = 40;
void on_trackbar( int val, void* )
{
 car_width = (int) val;
 car_len = (int)(val *1.59);
}

int main( int argc, char** argv ) {

	string imgurl;

	cout << "Select an image: ";
	cin >> imgurl;
	cout << endl;

	initial_img = cv::imread(imgurl, 1);

	initial_img.copyTo(img);

	cout << "Select the base empty parking spot with MOUSE1." << endl;

	namedWindow("Parking System");

	createTrackbar( "ROI SIZE", "Parking System", &slider, slider_max, on_trackbar );
	on_trackbar(slider, 0);


	imshow("Parking System", img);
	setMouseCallback("Parking System", CallBackFunc, NULL);

	
	waitKey(0);

	return 0;
}

int getNumberOfFreeSpots(Mat &img) {

	//GaussianBlur( img, img, Size(3,3),0,0);

	Mat roiImg, roiImage;

	// base roi of empty parking slot
	Rect rec(base_spot_x, base_spot_y, car_width, car_len); 
	roiImage=img(rec);
	cvtColor(roiImage, roiImage, CV_BGR2GRAY);
	equalizeHist(roiImage, roiImage);
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
	for (n=0; n < ccx.size(); n++) {

		Rect roi(ccx.at(n), ccy.at(n), car_width, car_len);
		roiImg=img(roi);
		cvtColor(roiImg,roiImg, CV_BGR2GRAY);

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
			printf(" [%d],  %f \\n", n, hist_comp);
			rectangle(img, Point(ccx.at(n),ccy.at(n)), Point(ccx.at(n)+car_width,ccy.at(n)+car_len), Scalar(0,0,255),0.3, 8);
			putText(img,to_string(n), Point(ccx.at(n),ccy.at(n)-5), FONT_HERSHEY_COMPLEX_SMALL, 0.5, Scalar(0,0,255),0.3, 8);
			busy++;
		} else {
			printf(" [%d],  %f \n", n, hist_comp );
			rectangle(img, Point(ccx.at(n),ccy.at(n)), Point(ccx.at(n)+car_width,ccy.at(n)+car_len), Scalar(0,255,0),0.3, 8);
			putText(img,to_string(n), Point(ccx.at(n),ccy.at(n)-5), FONT_HERSHEY_COMPLEX_SMALL, 0.5, Scalar(0,255,0),0.3, 8);
		}

	}

	int free_slots = number_of_parking_slots - busy;

	cout << endl << free_slots << " out of " << busy << " parking slots are free." << endl;

	return free_slots;
}