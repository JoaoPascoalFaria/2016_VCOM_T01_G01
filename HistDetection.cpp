
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
void startSecondPhase();
Mat getTopView(Mat src);

int base_spot_x = 37;
int base_spot_y = 88;

Mat initial_img, img;

vector<int> ccx;
vector<int> ccy;

int car_width = 17;
int car_len = 27;

int n;

int STATE = -1;

vector<Point2f> the_roi(4);
int c = 0;

void CallBackFunc(int event, int x, int y, int flags, void* userdata)
{
		if  ( event == EVENT_LBUTTONDOWN )
		{
			if (STATE == -1) {
				if(c<4) {
					the_roi[c++] = Point2f(x,y);
					cout << "Point added!" << endl;

					if (c >= 4) {
						initial_img = getTopView(initial_img);
						cout << "Region of interest sucessfully selected." << endl;
						STATE = 0;
						startSecondPhase();
					}

				} 
			} else if (STATE == 0) {

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

Mat getTopView(Mat src) {
	vector<Point2f> output(4);

	output[0] = Point2f(0,0);
	output[1] = Point2f(0,src.rows);
	output[2] = Point2f(src.cols,src.rows);
	output[3] = Point2f(src.cols,0);

    Mat M( 2, 4, CV_32FC1 );
	M = Mat::zeros( src.cols, src.rows, src.type());
	M = getPerspectiveTransform(the_roi,output);/**/
	//Mat M = findHomography(vertices,input,0 );
	Mat dst; warpPerspective(src, dst, M, src.size());
	imshow("Parking System", dst);
	return dst;
}

int slider = 17;
const int slider_max = 40;
void on_trackbar( int val, void* )
{
 car_width = (int) val;
 car_len = (int)(val *1.59);
}

void startSecondPhase() {
	initial_img.copyTo(img);

	cout << "Select a base pavement spot with MOUSE1." << endl;

	namedWindow("Parking System");

	createTrackbar( "ROI SIZE", "Parking System", &slider, slider_max, on_trackbar );
	on_trackbar(slider, 0);


	imshow("Parking System", img);
}

int main( int argc, char** argv ) {
	namedWindow("Parking System");

	setMouseCallback("Parking System", CallBackFunc, NULL);

	string imgurl;

	cout << "Select an image: ";
	cin >> imgurl;
	cout << endl;

	initial_img = cv::imread(imgurl, 1);
	imshow("Parking System", initial_img);
	waitKey(100);

	string answer;
	cout << "Do you want to select a region of interest of the image ( perspective will be warped if the region isn't rectangular )  [y/n]? ";
	cin >> answer;
	cout << endl;

	if (answer == "y") {
		imshow("Parking System", initial_img);
		cout << "Please select the 4 points of the region of interest with MOUSE1 (COUNTER CLOCK WISE)." << endl;
		
	} else {
		STATE = 0;
		startSecondPhase();
	}
	
	waitKey(0);

	return 0;
}

float howBlack(Mat image) {
 
    unsigned blacks=0, c=0;
    MatIterator_<uchar> it, end;
    for( it = image.begin<uchar>(), end = image.end<uchar>(); it != end; ++it) {
        if(*it != 255) blacks++;
            c++;
    }
   
    return blacks*100.0/c;
}

int getNumberOfFreeSpots(Mat &img) {

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

		
		
        Mat set_image_g, roiImage_t, set_image;
		img.copyTo(set_image_g);
		img.copyTo(roiImage_t);
		img.copyTo(set_image);
        GaussianBlur( set_image, set_image, Size(3,3),0,0);
        cvtColor( set_image, set_image_g, COLOR_BGR2GRAY);
        adaptiveThreshold( set_image_g, roiImage_t, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY, 11, 5);
        Mat open = getStructuringElement(MORPH_CROSS, Size(3, 3), Point(1, 1));
        Mat close = getStructuringElement(MORPH_CROSS, Size(3,2), Point(1, 0));
        morphologyEx(roiImage_t, roiImage_t, MORPH_OPEN, open);
        morphologyEx(roiImage_t, roiImage_t, MORPH_CLOSE, close);
        Mat set_image_hsv, s;
       
        //this second value defines how sensitive it is to color. 127,5 should be enough
        Scalar min_s(0, 255/3, 255/10);
        Scalar max_s(179, 255, 255);
        cvtColor( set_image(roi), set_image_hsv, CV_BGR2HSV);
        inRange( set_image_hsv, min_s, max_s, s);
 
		bool certain = false;
        // check if ROI has vivid colors
        if(100.0-howBlack(s) > 15) {
           certain = true;
        }

		if (certain) {
			printf(" [%d],  %f \n", n, hist_comp);
			rectangle(img, Point(ccx.at(n),ccy.at(n)), Point(ccx.at(n)+car_width,ccy.at(n)+car_len), Scalar(0,0,255),0.3, 8);
			busy++;
			continue;
		}
       
        int val = howBlack(roiImage_t(roi));
		if (val < 10) {
			printf(" [%d],  %f \n", n, hist_comp );
			rectangle(img, Point(ccx.at(n),ccy.at(n)), Point(ccx.at(n)+car_width,ccy.at(n)+car_len), Scalar(0,255,0),0.3, 8);
			continue;
		}

		if (hist_comp > 7 || val > 30) {
			printf(" [%d],  %f \n", n, hist_comp);
			rectangle(img, Point(ccx.at(n),ccy.at(n)), Point(ccx.at(n)+car_width,ccy.at(n)+car_len), Scalar(0,0,255),0.3, 8);
			busy++;
		} else {
			printf(" [%d],  %f \n", n, hist_comp );
			rectangle(img, Point(ccx.at(n),ccy.at(n)), Point(ccx.at(n)+car_width,ccy.at(n)+car_len), Scalar(0,255,0),0.3, 8);
		}


	}

	int free_slots = ccx.size() - busy;

	cout << endl << free_slots << " out of " << ccx.size() << " parking slots are free." << endl;

	return free_slots;
}