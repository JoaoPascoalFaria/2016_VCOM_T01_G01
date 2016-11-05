#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/calib3d/calib3d.hpp"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
//list files
#include <windows.h>
#include <tchar.h> 
#include <stdio.h>
#include <strsafe.h>
#pragma comment(lib, "User32.lib")

using namespace std;
using namespace cv;

vector<Point2f> the_roi(4);

/**
*	Detect mouse press
*/
void CallBackFunc(int event, int x, int y, int flags, void* userdata)
{
	static int c = 0;
     if  ( event == EVENT_LBUTTONDOWN )
     {
          cout << "Left button of the mouse is clicked - position (" << x << ", " << y << ")" << endl;
		  if(c<4)the_roi[c++] = Point2f(x,y);
     }
     else if  ( event == EVENT_RBUTTONDOWN )
     {
          cout << "Right button of the mouse is clicked - position (" << x << ", " << y << ")" << endl;
     }
     else if  ( event == EVENT_MBUTTONDOWN )
     {
          cout << "Middle button of the mouse is clicked - position (" << x << ", " << y << ")" << endl;
     }
     /*else if ( event == EVENT_MOUSEMOVE )
     {
          cout << "Mouse move over the window - position (" << x << ", " << y << ")" << endl;

     }/**/
}

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
		if(img_name=="0") _exit(-1);
		img = imread(img_name);
	}

	return img;
}

/**
*	extract roi from rotated rectangle
*/
Mat getRoiFromRotatedRect(Mat src, RotatedRect rect ) {

    Mat roi, M, rotated, cropped;
	/*
	Rect rectangle = rect.boundingRect();
	roi = src(rectangle);
	*/
    float angle = rect.angle;
    Size rect_size = rect.size;
    if (rect.angle < -45.) {
        angle += 90.0;
        swap(rect_size.width, rect_size.height);
    }
    M = getRotationMatrix2D(rect.center, angle, 1.0);
    warpAffine(src, rotated, M, src.size(), INTER_CUBIC);
    getRectSubPix(rotated, rect_size, rect.center, cropped);

	return cropped;
}

/**
*	extract roi from vector of points
*/
Mat getRoiFromPoints(Mat src, vector<Point> vertices) {

	Mat image = cvCreateMat(src.rows, src.cols, CV_8UC3);
	Mat mask = cvCreateMat(src.rows, src.cols, CV_8UC1);
	// Create black image with the same size as the original
	for(int i=0; i<mask.cols; i++)
	   for(int j=0; j<mask.rows; j++)
		   mask.at<uchar>(Point(i,j)) = 0;
 
	// Create Polygon from vertices
	vector<Point> ROI_Poly;
	approxPolyDP(vertices, ROI_Poly, 1.0, true);
	
	// Fill polygon white
	fillConvexPoly(mask, &ROI_Poly[0], ROI_Poly.size(), 255, 8, 0);                 
 
	// Cut out ROI and store it
	src.copyTo(image, mask);

	int minX, minY, maxX, maxY; maxX = maxY = 0; minX = image.cols; minY = image.rows;
	for(unsigned i=0;i<ROI_Poly.size(); i++){
		if( ROI_Poly[i].x < minX) minX = ROI_Poly[i].x;
		if( ROI_Poly[i].y < minY) minY = ROI_Poly[i].y;
		if( ROI_Poly[i].x > maxX) maxX = ROI_Poly[i].x;
		if( ROI_Poly[i].y > maxY) maxY = ROI_Poly[i].y;
	}
	Rect rectangle = Rect(minX,minY,maxX-minX,maxY-minY);
	image = image(rectangle);

	return image;
}

/**
*	extract roi from vector of points
*/
Mat getRoiFromPointsSimple(Mat src, Point2f input[4]){
	Point2f output[4];

	output[0] = Point2f(0,						0);
	output[1] = Point2f(input[1].x - input[0].x,0);
	output[2] = Point2f(input[1].x - input[0].x,input[1].y - input[2].y);
	output[3] = Point2f(0,						input[1].y - input[2].y);

    Mat M( 2, 4, CV_32FC1 );
	M = Mat::zeros( src.rows, src.cols, src.type() );
	M = getPerspectiveTransform(input,output);
    Mat dst; warpPerspective(src, dst, M, Size(input[1].x - input[0].x,input[1].y - input[2].y));
	return dst;
}
Mat getRoiFromPointsSimple(Mat src, vector<Point2f> vertices) {
	
	Point2f input[4];
	Point2f output[4];
	
	input[0] = vertices[0];
	input[1] = vertices[1];
	input[2] = vertices[2];
	input[3] = vertices[3];

	vector<float> variances(4);
	variances[0] = abs(vertices[0].x - vertices[1].x);
	variances[1] = abs(vertices[0].y - vertices[1].y);
	variances[2] = abs(vertices[1].x - vertices[2].x);
	variances[3] = abs(vertices[1].y - vertices[2].y);
	float width=0, height=0;
	for(unsigned i=0; i<4; i++) {
		if (variances[i] > height) {
			width = height;
			height = variances[i];
		}
		else if (variances[i] > width) {
			width = variances[i];
		}
	}

	output[0] = Point2f(0,		0);
	output[1] = Point2f(width,	0);
	output[2] = Point2f(width,	height);
	output[3] = Point2f(0,		height);

    Mat M( 2, 4, CV_32FC1 );
	M = Mat::zeros( src.rows, src.cols, src.type() );
	M = getPerspectiveTransform(input,output);
    Mat dst; warpPerspective(src, dst, M, Size(width, height));
	return dst;
}

/**
*	calculates similarity
*/
float getMatching(Mat image1, Mat image2,float threshold = 30.0f) {
	Mat HSVimg1, HSVimg2;
	cvtColor(image1, HSVimg1, CV_BGR2HSV);
	cvtColor(image2, HSVimg2, CV_BGR2HSV);
	Mat diffImage;
    absdiff(HSVimg1, HSVimg2, diffImage);

    Mat foregroundMask = Mat::zeros(diffImage.rows, diffImage.cols, CV_8UC1);

    float dist;
	unsigned blacks = 0, whites = 0;

    for(int j=0; j<diffImage.rows; ++j) {
        for(int i=0; i<diffImage.cols; ++i)  {
            Vec3b pix = diffImage.at<Vec3b>(j,i);

            dist = (pix[0]*pix[0] + pix[1]*pix[1] + pix[2]*pix[2]);
            dist = sqrt(dist);

            if(dist>threshold) {
                foregroundMask.at<unsigned char>(j,i) = 255;
				whites++;
            }
			else {
				blacks++;
			}
        }
	}
	/*static int c=0;
	namedWindow("test "+c);
	imshow("test "+c++, diffImage);
	cout<<whites<<"/"<<whites+blacks<<endl;*/
	return whites*100/(whites+blacks);
}
float getMatchingGray(Mat image1, Mat image2) {

	unsigned differences=0, c=0;
	MatIterator_<uchar> it, it2, end, end2;
	for( it = image1.begin<uchar>(), end = image1.end<uchar>(), it2 = image2.begin<uchar>(), end2 = image2.end<uchar>(); it != end && it2 != end2; ++it, ++it2) {
		if(*it != *it2) differences++;
			c++;
	}
    
	/*static int z=0;
	cout<<differences<<"/"<<c<<endl;
	namedWindow("a"+ to_string(z));
	imshow("a"+ to_string(z),image1);
	namedWindow("b"+ to_string(z));
	imshow("b"+ to_string(z++),image2);
	cout<<"percentagem de diferença: "<<differences*100/c<<endl;/**/

	return differences*100.0/c;
}
float howBlack(Mat image) {

	unsigned blacks=0, c=0;
	MatIterator_<uchar> it, end;
	for( it = image.begin<uchar>(), end = image.end<uchar>(); it != end; ++it) {
		if(*it != 255) blacks++;
			c++;
	}
    
	static int z=0;	
	//cout<<(++z>14?z=1:z)<<" percentagem de pretos: "<<blacks*100/c<<endl;
	/*namedWindow("a"+ to_string(z));
	imshow("a"+ to_string(z),image);
	waitKey();/**/
	return blacks*100.0/c;
}

/**
*	get top view from image
*/
Mat getTopViewFromSpot(Mat src, vector<Point2f> vertices) {

	vector<Point2f> input(4);

	vector<float> variances(4);
	variances[0] = abs(vertices[0].x - vertices[1].x);
	variances[1] = abs(vertices[0].y - vertices[1].y);
	variances[2] = abs(vertices[1].x - vertices[2].x);
	variances[3] = abs(vertices[1].y - vertices[2].y);
	float width=0, height=0;
	for(unsigned i=0; i<4; i++) {
		if (variances[i] > height) {
			width = height;
			height = variances[i];
		}
		else if (variances[i] > width) {
			width = variances[i];
		}
	}
	
	input[0] = vertices[0];
	input[1] = Point2f(vertices[0].x,		vertices[0].y+height);
	input[2] = Point2f(vertices[0].x+width, vertices[0].y+height);
	input[3] = Point2f(vertices[0].x+width, vertices[0].y);

    Mat M( 2, 4, CV_32FC1 );
	M = Mat::zeros( src.rows, src.cols, src.type());
	M = getPerspectiveTransform(vertices,input);/**/
	//Mat M = findHomography(vertices,input,0 );
	cout<<"M: "<<M<<endl;
	Mat dst; warpPerspective(src, dst, M, Size(src.cols+172,src.rows+44));

	return dst;
}
Mat getTopViewFromROI(Mat src, vector<Point2f> vertices) {

	vector<Point2f> output(4);

	output[0] = Point2f(0,0);
	output[1] = Point2f(0,src.rows);
	output[2] = Point2f(src.cols,src.rows);
	output[3] = Point2f(src.cols,0);

    Mat M( 2, 4, CV_32FC1 );
	M = Mat::zeros( src.cols, src.rows, src.type());
	M = getPerspectiveTransform(vertices,output);/**/
	//Mat M = findHomography(vertices,input,0 );
	Mat dst; warpPerspective(src, dst, M, src.size());

	return dst;
}
Mat getTopView(Mat src) {
	namedWindow("empty park");
	setMouseCallback("empty park", CallBackFunc, NULL);
	imshow("empty park", src);
	waitKey();
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
	imshow("empty park", dst);
	return dst;
}

/**
*	check if file is a jpg
*/
bool isJPG(TCHAR filename[]) {
	string ws(filename);
	return ws.substr(ws.find(".")+1)=="jpg";
}

/**
*	get all files from set
*/
vector<string> getFiles(){
	
	vector<string> files;
	WIN32_FIND_DATA ffd;
	LARGE_INTEGER filesize;
	TCHAR szDir[MAX_PATH];
	size_t length_of_arg;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	DWORD dwError=0;
   
	StringCchLength("images\\PKlot_PUCPR_subset", MAX_PATH, &length_of_arg);

	if (length_of_arg > (MAX_PATH - 3))
	{
		_tprintf(TEXT("\nDirectory path is too long.\n"));
		getchar();
		exit(-1);
	}

	// Prepare string for use with FindFile functions.  First, copy the
	// string to a buffer, then append '\*' to the directory name.
	StringCchCopy(szDir, MAX_PATH, "images\\PKlot_PUCPR_subset");
	StringCchCat(szDir, MAX_PATH, TEXT("\\*"));

	// Find the first file in the directory.
	hFind = FindFirstFile(szDir, &ffd);
	if (INVALID_HANDLE_VALUE == hFind) 
	{
		cout <<TEXT("FindFirstFile")<< endl;
		getchar();
		exit(dwError);
	} 
	int c=0;
	// List all the files in the directory with some info about them.
	do
	{
		if(isJPG(ffd.cFileName)) {
			files.push_back("images\\PKlot_PUCPR_subset\\"+string(ffd.cFileName));
		}
	}
	while (FindNextFile(hFind, &ffd) != 0);
 
	dwError = GetLastError();
	if (dwError != ERROR_NO_MORE_FILES) 
	{
		cout << TEXT("FindFirstFile") << endl;
	}

	FindClose(hFind);

	return files;
}

/**
*	adds the black area in both images
*/
Mat addThresholds(Mat image1, Mat image2, Mat image3) {

	Mat addition(image1.rows,image1.cols,image1.type());
	MatIterator_<uchar> it, it2, it3, itF;
	MatIterator_<uchar> end, end2, end3, endF;

	it = image1.begin<uchar>();
	end = image1.end<uchar>();
	it2 = image2.begin<uchar>();
	end2 = image2.end<uchar>();
	it3 = image3.begin<uchar>();
	end3 = image3.end<uchar>();
	itF = addition.begin<uchar>();
	endF = addition.end<uchar>();

	for( ; it != end && it2 != end2 && it3 != end3 && itF != endF; ++it, ++it2, ++it3, ++itF) {
		if((*it > 0 && *it != *it2) || *it3 < 255) *itF = 0;
		else *itF = 255;
	}

	namedWindow("a");
	imshow("a",addition);

	return addition;
}

int main(){

	/**
	*	Define vars
	*/
	Mat empty_park, test_image;
	Mat ioi, ioi2;
	Mat empty_park_g, test_image_g, ept, tit;
	vector<string> files;
	vector<vector<Point2f>> spots;
	vector<Point2f> middles;
	vector<Mat> empty_ioi;
	vector<Mat> test_ioi;
	vector<int> business;

	/**
	*	Load image
	*/
	empty_park = loadImage("empty park");
	//getTopView(empty_park);
	test_image = loadImage("test image");
	files = getFiles();
	
	/**
	*	Pre-process images
	*/
	// Smooth image
	GaussianBlur( empty_park, empty_park, Size(3,3),0,0);// números impares
	GaussianBlur( test_image, test_image, Size(3,3),0,0);
	cvtColor( empty_park, empty_park_g, COLOR_BGR2GRAY);
	cvtColor( test_image, test_image_g, COLOR_BGR2GRAY);
	threshold(empty_park_g, ept, 60, 255,THRESH_BINARY_INV+THRESH_OTSU);
	//adaptiveThreshold(empty_park_g,ept,255,ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY,11,5);
	threshold(test_image_g, tit, 60, 255,THRESH_BINARY_INV+THRESH_OTSU);

	/**
	*	Get ROI of parking lots from empty_park image
	*/
	//RotatedRect roi;
	//roi = RotatedRect(Point2f(190,190),Size2f(30,30),25);
	//ioi = getRoiFromRotatedRect(empty_park, roi);
	
	//vector<Point2f> vectRoi(4);
	//vectRoi[0] = Point2f(216,229);	vectRoi[1] = Point2f(247,229);
	//vectRoi[2] = Point2f(263,206);	vectRoi[3] = Point2f(233,206);
	//ioi = getRoiFromPointsSimple(empty_park, vectRoi);
	//ioi2 = getRoiFromPointsSimple(test_image, vectRoi);

	float x1,x2,y1,y2;
	float incX1, incX2, incY;
	for(x1=29, x2=8, y1=208, y2=232, incX1=27, incX2=27, incY=(4.0/14); x1<445 && x2<430; x1+=incX1, x2+=incX2, y1-=incY, y2-=incY) {
		vector<Point2f> a(4);
		a[0] = Point2f(x1,y1);
		a[1] = Point2f(x2,y2);
		a[2] = Point2f(x2+incX2,y2-incY);
		a[3] = Point2f(x1+incX1,y1-incY);
		spots.push_back(a);
		middles.push_back(Point2f(x2+10.0,y2-10.0));
		incX1 += incX1*0.015;
		incX2 += incX2*0.019;
		incY += incY*0.0;
	}
	for(int i=0; i<spots.size(); i++) {
		empty_ioi.push_back(getRoiFromPointsSimple(empty_park, spots[i]));
		test_ioi.push_back(getRoiFromPointsSimple(test_image, spots[i]));
	}

	vector<Point2f> p(4);
	p[0] = Point2f(182,45);
	p[1] = Point2f(40,196);
	p[2] = Point2f(1053,180);
	p[3] = Point2f(998,17);

	/**
	*	Process test_image or a set of images and detect cars and empty spots
	*/
	//int imageMatch = getMatching(ioi,ioi2);
	//cout << (imageMatch>25 ? "BUSY" : "FREE") << endl;
	for(int i=0; i<spots.size(); i++) {
		//business.push_back(getMatching(empty_ioi[i],test_ioi[i])>25);
		business.push_back(getMatchingGray(getRoiFromPointsSimple(ept, spots[i]),getRoiFromPointsSimple(tit, spots[i]))>25);
	}

	/**
	*	Draw Solution
	*/
	/*//RotatedRect
	Point2f vertices[4];
	roi.points(vertices);
	for (int i = 0; i < 4; i++)
		line(empty_park, vertices[i], vertices[(i+1)%4], Scalar(0,255,0));
	for (int i = 0; i < 4; i++)
		line(test_image, vertices[i], vertices[(i+1)%4], Scalar(0,255,0));
	*/
	// Vector<Point2f>
	//for (int i = 0; i < 4; i++) line(empty_park, vectRoi[i], vectRoi[(i+1)%4], (imageMatch>25 ? Scalar(0,0,255) : Scalar(0,255,0)));
	//for (int i = 0; i < 4; i++) line(test_image, vectRoi[i], vectRoi[(i+1)%4], (imageMatch>25 ? Scalar(0,0,255) : Scalar(0,255,0)));
	//putText(test_image,(imageMatch>25?"BUSY":"FREE"),vectRoi[0],FONT_HERSHEY_PLAIN,1,(imageMatch>25 ? Scalar(0,0,127) : Scalar(0,127,0)),3);
	for(int i=0; i<spots.size(); i++) {
		for (int j = 0; j < 4; j++) line(empty_park, spots[i][j], spots[i][(j+1)%4], (business[i] ? Scalar(0,0,255) : Scalar(0,255,0)));
		for (int j = 0; j < 4; j++) line(test_image, spots[i][j], spots[i][(j+1)%4], (business[i] ? Scalar(0,0,255) : Scalar(0,255,0)));
		putText(test_image,(business[i]?"BUSY":"FREE"),middles[i],FONT_HERSHEY_PLAIN,0.5,(business[i] ? Scalar(0,0,127) : Scalar(0,127,0)));
	}
	
	/*namedWindow("test park");
	imshow("test park", test_image);
	/*namedWindow("empty park");
	setMouseCallback("empty park", CallBackFunc, NULL);
	imshow("empty park", empty_park);
	/*namedWindow("ept");
	imshow("ept",ept);
	namedWindow("tit");
	imshow("tit",tit);
	/*namedWindow("tt",WINDOW_AUTOSIZE);
	vector<Point2f> a(4);
		a[0] = Point2f(29,208);
		a[1] = Point2f(8,232);
		a[2] = Point2f(8+27,232-2);
		a[3] = Point2f(29+27,208-2);
	imshow("tt",getTopView(empty_park,a));/**/
	/*namedWindow("region of interest 1", WINDOW_AUTOSIZE );
	imshow("region of interest 1", ioi);
	namedWindow("region of interest 2", WINDOW_AUTOSIZE );
	imshow("region of interest 2", ioi2);
	/**/

	// PARA CADA IMAGE;!!!
	Scalar min_s(0, 255/3, 255/10);
	Scalar max_s(179, 255, 255);
	Mat s, set_image_hsv;

	for(int si=0; si<files.size();) {
		Mat set_image,set_image_g,set_image_t,set_image_t2,set_image_t3;
		set_image = imread(files[si]);
		business.clear();

		GaussianBlur( set_image, set_image, Size(3,3),0,0);
		cvtColor( set_image, set_image_g, COLOR_BGR2GRAY);
		//threshold(set_image_g, set_image_t2, 60, 255,THRESH_BINARY_INV+THRESH_OTSU);
		adaptiveThreshold(set_image_g,set_image_t3,255,ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY,11,5);
		Mat open = getStructuringElement(MORPH_CROSS, Size(3, 3), Point(1, 1));
		Mat erosion = getStructuringElement(MORPH_CROSS, Size(3,2), Point(1, 0));
		morphologyEx(set_image_t3, set_image_t3, MORPH_OPEN,open);
		morphologyEx(set_image_t3, set_image_t3, MORPH_CLOSE,erosion);
		
		set_image_t = set_image_t3;
		//set_image_t = addThresholds(set_image_t2, ept, set_image_t3);
		
		for(int i=0; i<spots.size(); i++) {

			cvtColor( getRoiFromPointsSimple(set_image, spots[i]), set_image_hsv, CV_BGR2HSV);
			inRange( set_image_hsv, min_s, max_s, s);

			// > than 25% of image has color
			if(100.0-howBlack(s) > 25) {
				business.push_back(true);
				cout<<i+1<<" color detected"<<endl;
			}
			else {
				//business.push_back(getMatchingGray(getRoiFromPointsSimple(ept, spots[i]),test_roi)>25);
				int val = howBlack(getRoiFromPointsSimple(set_image_t, spots[i]));
				business.push_back( (val>=30? true : (val<=20? false : 2)) );
				cout<<i+1<<" "<< business[i] <<endl;
			}
		}

		for(int i=0; i<spots.size(); i++) {
			Scalar color = business[i] == false ? Scalar(0,255,0) : ( business [i] == 1 ? Scalar(0,0,255) : Scalar(0,255,255));
			for (int j = 0; j < 4; j++) line(set_image, spots[i][j], spots[i][(j+1)%4], color);
			putText(set_image,(business[i]==2? "VAGUE" : (business[i]?"BUSY":"FREE")),middles[i],FONT_HERSHEY_PLAIN,0.5, color);
			/*for (int j = 0; j < 4; j++) line(set_image, spots[i][j], spots[i][(j+1)%4], (business[i] ? Scalar(0,0,255) : Scalar(0,255,0)));
			putText(set_image,(business[i]?"BUSY":"FREE"),middles[i],FONT_HERSHEY_PLAIN,0.5,(business[i] ? Scalar(0,0,127) : Scalar(0,127,0)));/**/
		}
	
		/*namedWindow(("image set"+to_string(si)).c_str(),CV_WINDOW_NORMAL);
		cvSetWindowProperty(("image set"+to_string(si)).c_str(), CV_WND_PROP_FULLSCREEN, CV_WINDOW_FULLSCREEN);
		imshow(("image set"+to_string(si)).c_str(), set_image);*/
		namedWindow(((string)"image set").c_str(),CV_WINDOW_NORMAL);
		cvSetWindowProperty(((string)"image set").c_str(), CV_WND_PROP_FULLSCREEN, CV_WINDOW_FULLSCREEN);
		imshow(((string)"image set").c_str(), set_image);
		int key = waitKey();
		if(key == 2424832) si--;
		else si++;
	}

	waitKey(0);
	return 0;
}