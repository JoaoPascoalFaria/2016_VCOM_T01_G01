#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>

using namespace std;
using namespace cv;

/**
*	Load Images
*/
Mat loadImage(String context){
	string img_name;
	Mat img;

	cout << "Please select the image for " << context << endl;
	cin >> img_name;
	img = imread(img_name);
	while(!img.data){
		cout << "Invalid image! please select again or 0 to exit\n";
		cin >> img_name;
		if(img_name=="0") _exit;
		img = imread(img_name);
	}
}

int main(){

	/**
	*	Define vars
	*/
	Mat empty_park, test_image, ioi;
	Rect roio;

	/**
	*	Load image
	*/
	empty_park = loadImage("empty park");
	//test_image = loadImage("test image");
	
	/**
	*	Pre-process images
	*/
	// Smooth image
	GaussianBlur( empty_park, empty_park, Size(2,2),0);
	//GaussianBlur( test_image, test_image, Size(3,3),0);

	/**
	*	Get ROI of parking lots from empty_park image
	*/
	empty_park;

	/**
	*	Process test_image or a set of images and detect cars and empty spots
	*/

	/**
	*	Draw Solution
	*/
	namedWindow( "empty park", WINDOW_AUTOSIZE );
	imshow( "empty park", empty_park);
	namedWindow( "roi", WINDOW_AUTOSIZE );
	imshow( "roi", ioi);

	return 0;
}