#include <stdio.h>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <deque>
#include <cmath>

using namespace std;
using namespace cv;

const Mat orangeLower(1,1,CV_8UC3, Scalar(5,113,0));
const Mat orangeUpper(1,1,CV_8UC3, Scalar(23,195,255));
const Mat blueLower(1,1,CV_8UC3, Scalar(97,54,89));
const Mat blueUpper(1,1,CV_8UC3, Scalar(127,180,189));

const int veclen = 32;
int main(int argc, char** argv)
{
		/*
		if (argc != 2)
		{
				printf("usage: DisplayImage <Image_Path>\n");
				return -1;
		}
		*/
		VideoCapture cap(0);

		Mat src, dst, hsv;
		deque<Point> pts; 
		//src = imread( argv[1], CV_LOAD_IMAGE_COLOR);
		while(true)
		{
		cap >> src;

		if ( !src.data )
		{
				printf("No image data \n");
				return -1;
		}

		cvtColor(src,hsv, COLOR_BGR2HSV);
		inRange(hsv, orangeLower, orangeUpper, dst);
		erode(dst, dst,Mat(),Point(-1,-1), 2, BORDER_CONSTANT, morphologyDefaultBorderValue());
		dilate(dst, dst,Mat(), Point(-1,-1), 2, BORDER_CONSTANT, morphologyDefaultBorderValue());

		vector<Mat> cnts;
		findContours(dst.clone(), cnts, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);
		if (cnts.size() > 0)
		{
				for(int i = 0; i < cnts.size(); i++)
				{
						Point2f xy;
						float radius;
						Mat c = cnts[i];
						minEnclosingCircle(c, xy, radius);
						Moments M = moments(c);
						Point center = cvPoint(int(M.m10/M.m00), int(M.m01/M.m00));
						if (radius > 10)
						{
								circle(src, xy, int(radius), Scalar(0,255,255), 2);
								circle(src, center, 5, Scalar(0,0,255), -1);
						}
						if(pts.size() == veclen)
							pts.pop_front();
						pts.push_back(center);
				}
		}
		for (int i = 0; i < pts.size(); i++)
		{
				int thickness = int(sqrt(veclen/ float(i+1)) * 2.5);
				if(pts.size()>1)
				{
				line(src, pts.back(), pts[pts.size()-2], Scalar(0,0,255), thickness);
				}
		}
		

		imshow("Display Image", src);

		int key = waitKey(1);
		if (key == 27) break;
		}

		return 0;
}
