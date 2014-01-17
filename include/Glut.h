/*
 * Glut.h
 *
 *  Created on: Nov 15, 2013
 *      Author: coert
 */

#ifndef GLUT_H_
#define GLUT_H_

#ifdef _WIN32
#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>
#endif
#ifdef __linux__
#include <GL/glut.h>
#include <GL/glu.h>
#endif

#ifdef __APPLE__
#include <GLUT/glut.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>

#endif

#include "opencv2/opencv.hpp"

#include "arcball.h"

#include "General.h"
#include "Scene3DRenderer.h"
#include "Reconstructor.h"

using namespace std;
// i am not sure about the compatibility with this...
#define MOUSE_WHEEL_UP   3
#define MOUSE_WHEEL_DOWN 4

namespace nl_uu_science_gmt
{
	
#define MIN3(x,y,z)  ((y) <= (z) ? \
	((x) <= (y) ? (x) : (y)) \
	: \
	((x) <= (z) ? (x) : (z)))

#define MAX3(x,y,z)  ((y) >= (z) ? \
	((x) >= (y) ? (x) : (y)) \
	: \
	((x) >= (z) ? (x) : (z)))

	struct RGBColor {
		float r;
		float g;
		float b;
	};
	struct HSVColor {
		float hue;        /* Hue degree between 0 and 255 */
		float sat;        /* Saturation between 0 (gray) and 255 */
		float val;        /* Value between 0 (black) and 255 */
	};

	
class Glut
{
	Scene3DRenderer &_scene3d;
	cv::Mat	 _clusterCenters;
	cv::Mat  _labels;
	vector<int> _vectorLabels;
	vector<RGBColor> _clusterColors;
	

	static Glut* _glut;

	static void drawGrdGrid();
	static void drawCamCoord();
	static void drawVolume();
	static void drawArcball();
	static void drawVoxels();
	static void drawWCoord();
	static void drawInfo();

	static inline void perspectiveGL(GLdouble, GLdouble, GLdouble, GLdouble);

#ifdef _WIN32
	static void SetupPixelFormat(HDC hDC);
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
#endif

public:
	Glut(Scene3DRenderer &);
	virtual ~Glut();

#ifndef _WIN32
	void initializeLinux(const char*, int, char**);
	static void mouse(int, int, int, int);
#endif
#ifdef _WIN32
	int initializeWindows(const char*);
	void mainLoopWindows();
#endif

	static void keyboard(unsigned char, int, int);
	static void motion(int, int);
	static void reshape(int, int);
	static void reset();
	static void idle();
	static void display();
	static void update(int);
	static void cluster();
	static void findColors();
	static void computeColorModels();
	static void updateColorModels();
	static void getClosestVoxelsAndColorPerView(vector<int>(&closestVectorId)[4], vector<RGBColor>(&closestVectorColor)[4], cv::Mat(&cameraFrames)[4], bool initClusterColors);
	static void init(vector<int>(&closestVectorId)[4], vector<RGBColor>(&closestVectorColor)[4],
					cv::Mat(&cameraFrames)[4], vector<float>(&closestVectorDistance)[4],
					vector<RGBColor>& vectorColors, bool initClusterColors);
	static void calculateClosestVoxelAndColor(vector<int>(&closestVectorId)[4], vector<RGBColor>(&closestVectorColor)[4],
										cv::Mat(&cameraFrames)[4], vector<float>(&closestVectorDistance)[4],
										vector<RGBColor>& vectorColors, int currentView, int currentVoxelIndex);
	static RGBColor calculateColor(cv::Vec3f & bgrPixel);
	static float calcDistanceBetween3d(cv::Point3f pt1, cv::Point3f pt2);
	static float calcDistanceBetween2d(cv::Point2f pt1, cv::Point2f pt2);
	static void viewColorsToVoxelColors(vector<int>(&closestVectorId)[4], vector<RGBColor>(&closestVectorColor)[4],
		cv::Mat(&cameraFrames)[4], vector<RGBColor>& vectorColors);
	static void quit();
	static void drawClusterCenters();
	Scene3DRenderer& getScene3d() const
	{
		return _scene3d;
	}

	static struct HSVColor rgb_to_hsv(struct RGBColor rgb);
	static struct RGBColor hsv_to_rgb(struct HSVColor hsv);

};

} /* namespace nl_uu_science_gmt */

#endif /* GLUT_H_ */
