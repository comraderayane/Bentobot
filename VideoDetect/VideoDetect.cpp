#include <stdio.h>
#include <iostream>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/videoio.hpp>
#include <deque>
#include <cmath>
#include <string>
#include <raspicam/raspicam_cv.h>

using namespace std;
using namespace cv;


#define VECLEN 100

const int orange_min [3] = {11,26,233};
const int orange_max [3] = {30,217,255};
const int blue_min [3] = {97,54,89};
const int blue_max [3] = {127,180,189};

const int threshold_slider_max = 255;
const int threshold_slider_min = 0;
const char * THRESH_FILE_DEFAULT = "threshold.yml";
const int BUFLEN = 50;

struct Threshold
{
    int min[3];
    int max[3];
};

const Threshold orange = {.min = {0,0,0},
    .max = {255,255,255}};

//function primitives


void findInRange(Mat & src,Mat & dst,Mat & hsv, deque<Point> pts, Threshold main_thresh);
void findCircle(Mat & src, Mat& dst);

int main(int argc, char** argv)
{
    int cameraID;
    //load threshold file if it exists
    FileStorage threshfile;
    char thresh_file_name[BUFLEN];
    //create instance of threshold
    Threshold main_thresh = orange;

   if (argc == 2)
    {
        cameraID = atoi(argv[1]);
    }
    else
    {
        strcpy(thresh_file_name,THRESH_FILE_DEFAULT);
        cameraID = 0;
    }

        sprintf(thresh_file_name,"threshold_%d.yml",cameraID);

    if (threshfile.open(thresh_file_name, FileStorage::READ))
    {
        threshfile["min1"] >> main_thresh.min[0];
        threshfile["min2"] >> main_thresh.min[1];
        threshfile["min3"] >> main_thresh.min[2];
        threshfile["max1"] >> main_thresh.max[0];
        threshfile["max2"] >> main_thresh.max[1];
        threshfile["max3"] >> main_thresh.max[2];
    }
    threshfile.release();

    // Initialize Videocapture
    raspicam::RaspiCam_Cv Camera;
    VideoCapture cap1;
    if(cameraID == 0)
    {
    cap1.open(0);
    }
    else if (cameraID == 1)
    {
        //set camera params
//        Camera.set( CV_CAP_PROP_FORMAT, CV_8UC1 );
    Camera.set ( CV_CAP_PROP_FRAME_WIDTH, 640);
    Camera.set ( CV_CAP_PROP_FRAME_HEIGHT, 480);
        //Open camera
        cout<<"Opening Camera..."<<endl;
        if (!Camera.open()) {cerr<<"Error opening the camera"<<endl;return -1;}
    }

    //Mat src0, dst0, hsv0;
    //deque<Point> pts0; 

    Mat src1, dst1, hsv1; 
    deque<Point> pts1;



    //create window for trackbar
    namedWindow("Threshold", 1);

    createTrackbar("H min", "Threshold",&main_thresh.min[0],threshold_slider_max);
    createTrackbar("S min", "Threshold",&main_thresh.min[1],threshold_slider_max);
    createTrackbar("V min", "Threshold",&main_thresh.min[2],threshold_slider_max);
    createTrackbar("H max", "Threshold",&main_thresh.max[0],threshold_slider_max);  
    createTrackbar("S max", "Threshold",&main_thresh.max[1],threshold_slider_max);  
    createTrackbar("V max", "Threshold",&main_thresh.max[2],threshold_slider_max);  

    //begin loop
    while(true)
    {
    //    cap0 >> src0;
        if (cameraID == 0)
        {
            cap1 >> src1;
        }
        else if (cameraID == 1)
        {
            Camera.grab();
            Camera.retrieve (src1);
        }
 
        //	findInRange(src0,dst0,hsv0,pts0, main_thresh);
        	findInRange(src1,dst1,hsv1,pts1, main_thresh);
        //findCircle(src1,dst1);

        imshow("Display Image 0", src1);
        imshow("Filtered Image 0", dst1);
        //	imshow("Display Image 1", src1);
        //	imshow("Filtered Image 1", dst1);

        //close program when escape key is pressed
        int key = waitKey(1);
        if (key == 27 && threshfile.open(thresh_file_name, FileStorage::WRITE))
        {
            threshfile << "min1" << main_thresh.min[0];
            threshfile << "min2" << main_thresh.min[1];
            threshfile << "min3" << main_thresh.min[2];
            threshfile << "max1" << main_thresh.max[0];
            threshfile << "max2" << main_thresh.max[1];
            threshfile << "max3" << main_thresh.max[2];
            cout << "Saved to: " << thresh_file_name << endl;
            threshfile.release();
            break;
        }
    }

    return 0;
}

void findInRange(Mat & src,Mat & dst,Mat & hsv, deque<Point> pts, Threshold main_thresh)
{
    if ( !src.data )
    {
        printf("No image data \n");
        return;
    }

    //convert source image to HSV colorspace
    cvtColor(src,hsv, COLOR_BGR2HSV);

    //apply color range
    inRange(hsv, Scalar(main_thresh.min[0],
                main_thresh.min[1],
                main_thresh.min[2]),
            Scalar(main_thresh.max[0],
                main_thresh.max[1],
                main_thresh.max[2]),
            dst);

    //erode and dilate for faster processing
    erode(dst, dst,Mat(),Point(-1,-1), 2, BORDER_CONSTANT, morphologyDefaultBorderValue());
    dilate(dst, dst,Mat(), Point(-1,-1), 2, BORDER_CONSTANT, morphologyDefaultBorderValue());

    //find contours
    vector<Mat> cnts;
    findContours(dst.clone(), cnts, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);
    if (cnts.size() > 0)
    {
        //find the contour with max area
        Mat c = cnts[0];
        for(int i = 0; i < cnts.size(); i++)
        {
            if (contourArea(c) < contourArea(cnts[i]))
            {
                c = cnts[i];
            }
        }

        Point2f xy;
        float radius;

        //find the minimum enclosing circle around that contour
        minEnclosingCircle(c, xy, radius);
        //place the center at the centroid of the contour
        Moments M = moments(c);
        Point center = cvPoint(int(M.m10/M.m00), int(M.m01/M.m00));
        //draw the enclosing circle and centroid
        if (radius > 10)
        {
            circle(src, xy, int(radius), Scalar(0,255,255), 2);
            circle(src, center, 5, Scalar(0,0,255), -1);
            pts.push_back(center);
        }
        if(pts.size() == VECLEN)
            pts.pop_front();
    }
    //draw history trail
    for (int i = 1; i < pts.size(); i++)
    {
        int thickness = int(sqrt(VECLEN/ float(i+1)) * 2.5);
        if(pts.size()>1)
        {
            line(src, pts[i], pts[i-1], Scalar(0,0,255), thickness);
        }
    }

}

void findCircle(Mat & src, Mat & gray)
{
    cvtColor(src, gray, COLOR_BGR2GRAY);
    medianBlur(gray, gray, 5);
    vector<Vec3f> circles;
    HoughCircles(gray, circles, HOUGH_GRADIENT, 1,
            gray.rows/16, // change this value to detect circles with different distances to each other
            100, 30, 1, 1000 // change the last two parameters
            // (min_radius & max_radius) to detect larger circles
            );
    for( size_t i = 0; i < circles.size(); i++ )
    {
        Vec3i c = circles[i];
        circle( src, Point(c[0], c[1]), c[2], Scalar(0,0,255), 3, LINE_AA);
        circle( src, Point(c[0], c[1]), 2, Scalar(0,255,0), 3, LINE_AA);
    }
}
