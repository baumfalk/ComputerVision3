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
	struct RGBColor {
		float r;
		float g;
		float b;
	};

class Glut
{
	Scene3DRenderer &_scene3d;
	cv::Mat	 _clusterCenters;
	cv::Mat  _labels;
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
	static float calcDistanceBetween(cv::Point3f pt1, cv::Point3f pt2);
	static void viewColorsToVoxelColors(vector<int>(&closestVectorId)[4], vector<RGBColor>(&closestVectorColor)[4],
		cv::Mat(&cameraFrames)[4], vector<RGBColor>& vectorColors);
	static void quit();
	static void drawClusterCenters();
	Scene3DRenderer& getScene3d() const
	{
		return _scene3d;
	}

};

} /* namespace nl_uu_science_gmt */

#endif /* GLUT_H_ */
