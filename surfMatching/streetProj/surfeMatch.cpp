#include <stdio.h>
#include <iostream>
#include "opencv2/core/core.hpp"
#include "opencv2/features2d/features2d.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/calib3d/calib3d.hpp"
#include "opencv2/nonfree/nonfree.hpp"
#include <opencv2\opencv.hpp>

using namespace cv;
using namespace std;

/** @function main */
int main(int argc, char** argv)
{
	Mat frame;
	Mat object = imread("bbox.jpg");
	if (object.empty()) return -1;

	VideoCapture capture(1);
	if (!capture.isOpened())  return -1;
	
	for (;;) {
		capture.read(frame);
		capture >> frame;

		//Size half(frame.size().width / 2, frame.size().height / 2);
		//resize(frame, frame, half);

		//Detect the keypoints using SURF Detector
		int minHessian = 200;
		SurfFeatureDetector detector(minHessian);
		vector<KeyPoint> keypoints_object, keypoints_scene;
		detector.detect(object, keypoints_object);
		detector.detect(frame, keypoints_scene);

		//Calculate descriptors (feature vectors)
		SurfDescriptorExtractor extractor;
		Mat descriptors_object, descriptors_scene;
		extractor.compute(object, keypoints_object, descriptors_object);
		extractor.compute(frame, keypoints_scene, descriptors_scene);

		//Matching descriptor vectors using FLANN matcher
		FlannBasedMatcher matcher;
		vector< DMatch > matches;
		matcher.match(descriptors_object, descriptors_scene, matches);

		double max_dist = 0; double min_dist = 100;
		//Quick calculation of max and min distances between keypoints
		for (int i = 0; i < descriptors_object.rows; i++) {
			double dist = matches[i].distance;
			if (dist < min_dist) min_dist = dist;
			if (dist > max_dist) max_dist = dist;
		}

		//printf("-- Max dist : %f \n", max_dist);
		//printf("-- Min dist : %f \n", min_dist);

		//Draw only "good" matches (i.e. whose distance is less than 3*min_dist )
		vector< DMatch > good_matches;
		for (int i = 0; i < descriptors_object.rows; i++) {
			if (matches[i].distance < 3 * min_dist)	{
				good_matches.push_back(matches[i]);
			}
		}

		Mat img_matches;
		drawMatches(object, keypoints_object, frame, keypoints_scene,
			good_matches, img_matches, Scalar::all(-1), Scalar::all(-1),
			vector<char>(), DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS);

		//Localize the object
		vector<Point2f> obj;
		vector<Point2f> scene;

		for (int i = 0; i < good_matches.size(); i++) {
			//Get the keypoints from the good matches
			obj.push_back(keypoints_object[good_matches[i].queryIdx].pt);
			scene.push_back(keypoints_scene[good_matches[i].trainIdx].pt);
		}

		Mat H = findHomography(obj, scene, CV_RANSAC);
		//Finds a perspective transformation between two planes

		//Get the corners from the image_1 ( the object to be "detected" )
		vector<Point2f> obj_corners(4);
		obj_corners[0] = cvPoint(0, 0); obj_corners[1] = cvPoint(object.cols, 0);
		obj_corners[2] = cvPoint(object.cols, object.rows); obj_corners[3] = cvPoint(0, object.rows);
		vector<Point2f> scene_corners(4);

		perspectiveTransform(obj_corners, scene_corners, H);
		//Performs the perspective matrix transformation of vectors.

		//Draw lines between the corners (the mapped object in the scene - image_2 )
		line(img_matches, scene_corners[0] + Point2f(object.cols, 0), scene_corners[1] + Point2f(object.cols, 0), Scalar(0, 255, 255), 2);
		line(img_matches, scene_corners[1] + Point2f(object.cols, 0), scene_corners[2] + Point2f(object.cols, 0), Scalar(0, 255, 255), 2);
		line(img_matches, scene_corners[2] + Point2f(object.cols, 0), scene_corners[3] + Point2f(object.cols, 0), Scalar(0, 255, 255), 2);
		line(img_matches, scene_corners[3] + Point2f(object.cols, 0), scene_corners[0] + Point2f(object.cols, 0), Scalar(0, 255, 255), 2);

		imshow("MatchingResult", img_matches);
		int key = cvWaitKey(10);
		if (key == 27) {
			break;
		}
	}

	return 0;
}
