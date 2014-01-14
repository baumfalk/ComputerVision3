/*
 * Glut.cpp
 *
 *  Created on: Nov 15, 2013
 *      Author: Coert and a guy named Frank
 */

#include "Glut.h"

using namespace std;
using namespace cv;

namespace nl_uu_science_gmt
{

Glut* Glut::_glut;

Glut::Glut(Scene3DRenderer &s3d) :
		_scene3d(s3d)
{
	// static pointer to this class so we can get to it from the static GL events
	_glut = this;
}

Glut::~Glut()
{
}

#ifndef _WIN32
/**
 * Main OpenGL initialisation for Linux-like system (with Glut)
 */
void Glut::initializeLinux(const char* win_name, int argc, char** argv)
{
	arcball_reset();	//initialize the ArcBall for scene rotation

	glutInit(&argc, argv);
	glutInitWindowSize(_glut->getScene3d().getWidth(), _glut->getScene3d().getHeight());
	glutInitWindowPosition(700, 10);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);

	glutCreateWindow(win_name);

	glutReshapeFunc(reshape);
	glutDisplayFunc(display);
	glutKeyboardFunc(keyboard);
	glutIdleFunc(idle);
	glutMouseFunc(mouse);
	glutMotionFunc(motion);

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	glutTimerFunc(10, update, 0);

	// from now on it's just events
	glutMainLoop();
}
#elif defined _WIN32
/**
 * Main OpenGL initialisation for Windows-like system (without Glut)
 */
int Glut::initializeWindows(const char* win_name)
{
	Scene3DRenderer &scene3d = _glut->getScene3d();
	arcball_reset();	//initialize the ArcBall for scene rotation

	WNDCLASSEX windowClass;//window class
	HWND hwnd;//window handle
	DWORD dwExStyle;//window extended style
	DWORD dwStyle;//window style
	RECT windowRect;

	/*      Screen/display attributes*/
	int width = scene3d.getWidth();
	int height = scene3d.getHeight();
	int bits = 32;

	windowRect.left =(long)0;               //set left value to 0
	windowRect.right =(long)width;//set right value to requested width
	windowRect.top =(long)0;//set top value to 0
	windowRect.bottom =(long)height;//set bottom value to requested height

	/*      Fill out the window class structure*/
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = Glut::WndProc;
	windowClass.cbClsExtra = 0;
	windowClass.cbWndExtra = 0;
	windowClass.hInstance = 0;                //hInstance;
	windowClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	windowClass.hbrBackground = NULL;
	windowClass.lpszMenuName = NULL;
	windowClass.lpszClassName = LPCSTR("Glut");
	windowClass.hIconSm = LoadIcon(NULL, IDI_WINLOGO);

	/*      Register window class*/
	if (!RegisterClassEx(&windowClass))
	{
		return 0;
	}

	/*      Check if fullscreen is on*/
	if (scene3d.isShowFullscreen())
	{
		DEVMODE dmScreenSettings;
		memset(&dmScreenSettings, 0, sizeof(dmScreenSettings));
		dmScreenSettings.dmSize = sizeof(dmScreenSettings);
		dmScreenSettings.dmPelsWidth = width;   //screen width
		dmScreenSettings.dmPelsHeight = height;//screen height
		dmScreenSettings.dmBitsPerPel = bits;//bits per pixel
		dmScreenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

		if (ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN !=
						DISP_CHANGE_SUCCESSFUL))
		{
			/*      Setting display mode failed, switch to windowed*/
			MessageBox(NULL, LPCSTR("Display mode failed"), NULL, MB_OK);
			scene3d.setShowFullscreen(false);
		}
	}

	/*      Check if fullscreen is still on*/
	if (scene3d.isShowFullscreen())
	{
		dwExStyle = WS_EX_APPWINDOW;    //window extended style
		dwStyle = WS_POPUP;//windows style
		ShowCursor(FALSE);//hide mouse pointer
	}
	else
	{
		dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;  //window extended style
		dwStyle = WS_OVERLAPPEDWINDOW;//windows style
	}

	AdjustWindowRectEx(&windowRect, dwStyle, FALSE, dwExStyle);

	/*      Class registerd, so now create our window*/
	hwnd = CreateWindowEx(NULL, LPCSTR("Glut"),  //class name
			LPCSTR(win_name),//app name
			dwStyle |
			WS_CLIPCHILDREN |
			WS_CLIPSIBLINGS,
			0, 0,//x and y coords
			windowRect.right - windowRect.left,
			windowRect.bottom - windowRect.top,//width, height
			NULL,//handle to parent
			NULL,//handle to menu
			0,//application instance
			NULL);//no xtra params

	/*      Check if window creation failed (hwnd = null ?)*/
	if (!hwnd)
	{
		return 0;
	}

	ShowWindow(hwnd, SW_SHOW);             //display window
	UpdateWindow(hwnd);//update window

	if (scene3d.isShowFullscreen())
	{
		ChangeDisplaySettings(NULL, 0);
		ShowCursor(TRUE);
	}

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	// best frame for our movie
	scene3d.setCurrentFrame(101);

	//	return int(msg.wParam);
	return 1;
}

void Glut::cluster() {
	vector<Reconstructor::Voxel*> voxels = _glut->getScene3d().getReconstructor().getVisibleVoxels();

	vector<Point2f> voxelsVector;
	for (int i = 0; i < voxels.size(); i++) {
		voxelsVector.push_back(Point2f(voxels[i]->x, voxels[i]->y));
	}

	Mat voxels2d(voxelsVector, CV_32F);


	// keep doing kmeans until there is enough distance between clusters (i.e. we are fairly sure that we are not in a local maximum)
	float minDist = 51;
	do
	{
		kmeans(voxels2d, 4, _glut->_labels, TermCriteria(CV_TERMCRIT_ITER | CV_TERMCRIT_EPS, 10000, 0.0001), 5, KMEANS_RANDOM_CENTERS, _glut->_clusterCenters);
		// for all but the last cluster center
		for (int i = 0; i < 3; i++)
		{
			// check if all the cluster centers with a higher number
			// than the cluster we are looking at are too close
			const float* Mi = _glut->_clusterCenters.ptr<float>(i);
			for (int j = i + 1; j < 4; j++) {
				const float* Mj = _glut->_clusterCenters.ptr<float>(j);
				float a = Mi[0] - Mj[0];
				float b = Mi[1] - Mj[1];
				float dist = sqrt(a*a + b*b);
				if (dist < minDist)
					minDist = dist;
			}
		}
	} while (minDist <= 50);
	

	//cout << _glut->_clusterCenters << endl;
}

struct RGBcolor {
	float r;
	float g;
	float b;
};


void Glut::computeColorModels() {
	cluster();
	vector<Reconstructor::Voxel*> voxels = _glut->getScene3d().getReconstructor().getVisibleVoxels();
	map<Point2f, int> pt2vxl_vector[4]; // point and index of the voxel in the vector voxels
	map<int, Point2f> vxl2pt_vector[4];
	map<int, RGBcolor> voxel_colors;
	map<int,int> voxel_count; 
	// init
	for (int i = 0; i < 4; i++) {
		map<Point2f, int> pt2vxl;
		pt2vxl_vector[i] = pt2vxl;
		map<int, Point2f> vxl2pt;
		vxl2pt_vector[i] = vxl2pt;
	}
	// determine for all views which voxels are visible.
	// for each voxel
	
	for (int v = 0; v < voxels.size(); v++)
	{
		int teller = 0;
		// for each view	
		for (int i = 0; i < 4; i++) {
			// project voxel to 2d point
			Camera * cam = _glut->getScene3d().getCameras()[i];
			Point3f pt3d(voxels.at(v)->x, voxels.at(v)->y, voxels.at(v)->z);
			Point2f pt2d = cam->projectOnView(pt3d);
			// rounding so that small differences
			// inherent to working with floats are
			// neglected.
			pt2d.x = roundf(pt2d.x);
			pt2d.y = roundf(pt2d.y);

			// if another voxel with same 2d point exists
			if (pt2vxl_vector[i].find(pt2d) != pt2vxl_vector[i].end()) {
				// calculate distance from voxel to camera point
				int other_voxel_id = pt2vxl_vector[i][pt2d];
				Reconstructor::Voxel * voxel_other = voxels[other_voxel_id];
				Point3f pt3d_other(voxel_other->x, voxel_other->y, voxel_other->z);
				double x_diff, y_diff, z_diff, x_diff_other,
					y_diff_other, z_diff_other, distance, distance_other;
				x_diff = abs(pt3d.x - cam->getCameraLocation().x);
				y_diff = abs(pt3d.y - cam->getCameraLocation().y);
				z_diff = abs(pt3d.z - cam->getCameraLocation().z);
				distance = sqrtf(x_diff*x_diff + y_diff*y_diff + z_diff*z_diff);

				x_diff_other = abs(cam->getCameraLocation().x - pt3d_other.x);
				y_diff_other = abs(cam->getCameraLocation().y - pt3d_other.y);
				z_diff_other = abs(cam->getCameraLocation().z - pt3d_other.z);
				distance_other = sqrtf(x_diff_other*x_diff_other + y_diff_other*y_diff_other + z_diff_other*z_diff_other);
				// if this voxel is closer to camera point than the other voxel
				if (distance < distance_other) {
					teller++;
					// replace other voxel/point pair with this voxel/point pair
					pt2vxl_vector[i][pt2d] = v;
					// remove the other voxel
					vxl2pt_vector[i].erase(other_voxel_id);
				}

			}
			// else add voxel/point pair
			else {
				pt2vxl_vector[i][pt2d] = v;
				vxl2pt_vector[i][v] = pt2d;
			}
		}
	}
	cout << vxl2pt_vector[0].size() << endl;
	cout << pt2vxl_vector[0].size() << endl;
	Mat frame;

	RGBcolor pointColor;
	RGBcolor pointColor_old;
	for (int v = 0; v < voxels.size(); v++)
	{
		for (int i = 0; i < 4; i++) {
			// is voxel with index v visible in viewpoint i
			if (vxl2pt_vector[i].find(v) != vxl2pt_vector[i].end()) {
				
				Point3f pt3d(voxels.at(v)->x, voxels.at(v)->y, voxels.at(v)->z);
				Camera * cam = _glut->getScene3d().getCameras()[i];
				Point2f point = cam->projectOnView(pt3d);
				cam = _glut->getScene3d().getCameras()[i];
				frame = cam->getFrame();
				

				if (frame.dims == 0)
					continue;
			
				voxel_count[v]++;
				// first time we've seen this voxel in any view
				if (voxel_count[v] == 1) {
					if (point.x < frame.rows && point.y < frame.cols)
					{
						Vec3f bgrPixel = frame.at<Vec3b>(point.y, point.x);
						//Vec3b bgrPixel2 = frame.at<Vec3b>(point.y, point.x);
	
						pointColor.b = bgrPixel.val[0];
						pointColor.g = bgrPixel.val[1];
						pointColor.r = bgrPixel.val[2];
						voxel_colors[v] = pointColor;
						
					}
				}
				else {
					
					if (point.x < frame.rows && point.y < frame.cols)
					{		
						pointColor_old = voxel_colors[v];
					
						pointColor.r = ((pointColor_old.r * (voxel_count[v] - 1) + frame.at<Vec3b>(point.x, point.y)[2]) / voxel_count[v]);
						pointColor.g = ((pointColor_old.g * (voxel_count[v] - 1) + frame.at<Vec3b>(point.x, point.y)[1]) / voxel_count[v]);
						pointColor.b = ((pointColor_old.b * (voxel_count[v] - 1) + frame.at<Vec3b>(point.x, point.y)[0]) / voxel_count[v]);
						voxel_colors[v] = pointColor;
		
					}
				}
			}
		}
	}
	int label_count[4];
	double r[4], g[4], b[4];
	// average all voxel colors with the same label
	
	for (int v = 0; v < voxels.size(); v++)
	{
		bool visible = false;
		for (int i = 0; i < 4; i++) {
			if (vxl2pt_vector[i].find(v) != vxl2pt_vector[i].end()) {
				visible = true;
				break;
			}
		}
		if (visible) {
			cout << "HOO" << endl;
			//labels
			int label = _glut->_labels.at<int>(v);

			label_count[label]++;
	
			if (label_count[label] == 1) {
				
				r[label] = voxel_colors[v].r;
				g[label] = voxel_colors[v].g;
				b[label] = voxel_colors[v].b;
			}
			else {
				r[label] = (r[label] * (label_count[label] - 1) + voxel_colors[v].r) / label_count[label];
				g[label] = (g[label] * (label_count[label] - 1) + voxel_colors[v].g) / label_count[label];
				b[label] = (b[label] * (label_count[label] - 1) + voxel_colors[v].b) / label_count[label];
			}
			
			
		}
	}

	for (int i = 0; i < 4; i++) {
		cout << r[i] << " " << g[i] << " " << b[i] << endl;
	}

	


}

void Glut::updateColorModels() {
	cluster();
}

void Glut::drawClusterCenters()
{
	const Scene3DRenderer& scene3d = _glut->getScene3d();
	glLineWidth(1.0f);
	glPushMatrix();
	glBegin(GL_LINES);

	const int len = scene3d.getSquareSideLen();

	for (int i = 0; i < _glut->_clusterCenters.rows; i++)
	{
		const float* Mi = _glut->_clusterCenters.ptr<float>(i);

		glColor4f(1.0f, 0.0f, 0.0f, 0.5f);
		glVertex3f(Mi[0], Mi[1], 0.0f);
		glVertex3f(Mi[0], Mi[1], len * 30);
	}

	glEnd();
	glPopMatrix();
}

/**
* Draw all visible voxels
*/
void Glut::drawVoxels()
{
	glPushMatrix();

	// apply default translation
	glTranslatef(0, 0, 0);
	glPointSize(2.0f);
	glBegin(GL_POINTS);

	vector<Reconstructor::Voxel*> voxels = _glut->getScene3d().getReconstructor().getVisibleVoxels();

	for (size_t v = 0; v < voxels.size(); v++)
	{
		int label = 0;

		if (_glut->_labels.cols != 0)
			label = _glut->_labels.at<int>(v);

		// different color per label
		if (label == 0)
		{
			glColor4f(1.0f, 0.5f, 0.5f, 0.5f);
		}
		else if (label == 1)
		{
			glColor4f(0.5f, 1.0f, 0.5f, 0.5f);
		}
		else if (label == 2)
		{
			glColor4f(0.5f, 0.5f, 1.0f, 0.5f);
		}
		else
		{
			glColor4f(0.75f, 0.25f, 1.0f, 0.5f);
		}
		glVertex3f((GLfloat)voxels[v]->x, (GLfloat)voxels[v]->y, (GLfloat)voxels[v]->z);
	}

	glEnd();
	glPopMatrix();
}

/**
 * This loop updates and displays the scene every iteration
 */
void Glut::mainLoopWindows()
{
	if (!_glut->getScene3d().isQuit()) {
		
		update(0);
		//cluster();
		//computeColorModels();
		display();
		
	}
	update(0);
	//cluster();
	computeColorModels();
	display();
	while(!_glut->getScene3d().isQuit())
	{

		update(0);
		//updateColorModels();
		cluster();
		display();
		
	}
}
#endif

/**
 * http://nehe.gamedev.net/article/replacement_for_gluperspective/21002/
 * replacement for gluPerspective();
 */
void Glut::perspectiveGL(GLdouble fovY, GLdouble aspect, GLdouble zNear, GLdouble zFar)
{
	GLdouble fW, fH;

	fH = tan(fovY / 360 * CV_PI) * zNear;
	fW = fH * aspect;

	glFrustum(-fW, fW, -fH, fH, zNear, zFar);
}

void Glut::reset()
{
	Scene3DRenderer& scene3d = _glut->getScene3d();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	perspectiveGL(50, scene3d.getAspectRatio(), 1, 40000);
	gluLookAt(scene3d.getArcballEye().x, scene3d.getArcballEye().y, scene3d.getArcballEye().z,
			scene3d.getArcballCentre().x, scene3d.getArcballCentre().y, scene3d.getArcballCentre().z,
			scene3d.getArcballUp().x, scene3d.getArcballUp().y, scene3d.getArcballUp().z);

	// set up the ArcBall using the current projection matrix
	arcball_setzoom(scene3d.getSphereRadius(), scene3d.getArcballEye(), scene3d.getArcballUp());

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void Glut::quit()
{
	_glut->getScene3d().setQuit(true);
	exit(EXIT_SUCCESS);
}

/**
 * Handle all keyboard input
 */
void Glut::keyboard(unsigned char key, int x, int y)
{
	char *p_end;
	int key_i = strtol(string(key, key).substr(0, 1).c_str(), &p_end, 10);

	Scene3DRenderer& scene3d = _glut->getScene3d();
	if (key_i == 0)
	{
		if (key == 'q' || key == 'Q')
		{
			scene3d.setQuit(true);
		}
		else if (key == 'p' || key == 'P')
		{
			bool paused = scene3d.isPaused();
			scene3d.setPaused(!paused);
		}
		else if (key == 'b' || key == 'B')
		{
			scene3d.setCurrentFrame(scene3d.getCurrentFrame() - 1);
		}
		else if (key == 'n' || key == 'N')
		{
			scene3d.setCurrentFrame(scene3d.getCurrentFrame() + 1);
		}
		else if (key == 'r' || key == 'R')
		{
			bool rotate = scene3d.isRotate();
			scene3d.setRotate(!rotate);
		}
		else if (key == 's' || key == 'S')
		{
#ifdef _WIN32
			cerr << "ShowArcball() not supported on Windows!" << endl;
#endif
			bool arcball = scene3d.isShowArcball();
			scene3d.setShowArcball(!arcball);
		}
		else if (key == 'v' || key == 'V')
		{
			bool volume = scene3d.isShowVolume();
			scene3d.setShowVolume(!volume);
		}
		else if (key == 'g' || key == 'G')
		{
			bool floor = scene3d.isShowGrdFlr();
			scene3d.setShowGrdFlr(!floor);
		}
		else if (key == 'c' || key == 'C')
		{
			bool cam = scene3d.isShowCam();
			scene3d.setShowCam(!cam);
		}
		else if (key == 'i' || key == 'I')
		{
#ifdef _WIN32
			cerr << "ShowInfo() not supported on Windows!" << endl;
#endif
			bool info = scene3d.isShowInfo();
			scene3d.setShowInfo(!info);
		}
		else if (key == 'o' || key == 'O')
		{
			bool origin = scene3d.isShowOrg();
			scene3d.setShowOrg(!origin);
		}
		else if (key == 't' || key == 'T')
		{
			scene3d.setTopView();
			reset();
			arcball_reset();
		}
	}
	else if (key_i > 0 && key_i <= (int) scene3d.getCameras().size())
	{
		scene3d.setCamera(key_i - 1);
		reset();
		arcball_reset();
	}
}

#ifndef _WIN32
/**
 * Handle linux mouse input (clicks and scrolls)
 */
void Glut::mouse(int button, int state, int x, int y)
{
	if (state == GLUT_DOWN)
	{
		int invert_y = (_glut->getScene3d().getHeight() - y) - 1;  // OpenGL viewport coordinates are Cartesian
		arcball_start(x, invert_y);
	}

	// scrollwheel support, handcrafted!
	if (state == GLUT_UP)
	{
		if (button == MOUSE_WHEEL_UP && !_glut->getScene3d().isCameraView())
		{
			arcball_add_distance(+250);
		}
		else if (button == MOUSE_WHEEL_DOWN && !_glut->getScene3d().isCameraView())
		{
			arcball_add_distance(-250);
		}
	}
}
#elif defined _WIN32
/**
 * Function to set the pixel format for the device context
 */
void Glut::SetupPixelFormat(HDC hDC)
{
	/*      Pixel format index
	 */
	int nPixelFormat;

	static PIXELFORMATDESCRIPTOR pfd =
	{
		sizeof(PIXELFORMATDESCRIPTOR),          //size of structure
		1,//default version
		PFD_DRAW_TO_WINDOW |//window drawing support
		PFD_SUPPORT_OPENGL |//opengl support
		PFD_DOUBLEBUFFER,//double buffering support
		PFD_TYPE_RGBA,//RGBA color mode
		32,//32 bit color mode
		0, 0, 0, 0, 0, 0,//ignore color bits
		0,//no alpha buffer
		0,//ignore shift bit
		0,//no accumulation buffer
		0, 0, 0, 0,//ignore accumulation bits
		16,//16 bit z-buffer size
		0, //no stencil buffer
		0, //no aux buffer
		PFD_MAIN_PLANE,//main drawing plane
		0,//reserved
		0, 0, 0};                              //layer masks ignored

	/*      Choose best matching format*/
	nPixelFormat = ChoosePixelFormat(hDC, &pfd);

	/*      Set the pixel format to the device context*/
	SetPixelFormat(hDC, nPixelFormat, &pfd);
}

/**
 * Handle all windows keyboard and mouse inputs with WM_ events
 */
LRESULT CALLBACK Glut::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	Scene3DRenderer &scene3d = _glut->getScene3d();

	// Rendering and Device Context variables are declared here.
	static HGLRC hRC;
	static HDC hDC;
	LONG lRet = 1;

	switch(message)
	{
		case WM_CREATE:                              // Window being created
		{
			hDC = GetDC(hwnd);                              // Get current windows device context
			scene3d.setHDC(hDC);
			SetupPixelFormat(hDC);													// Call our pixel format setup function

																											// Create rendering context and make it current
			hRC = wglCreateContext(hDC);
			wglMakeCurrent(hDC, hRC);
		}
		break;
		case WM_CLOSE:                              // Window is closing
		{
			hDC = GetDC(hwnd);                              // Get current windows device context
			// Deselect rendering context and delete it
			wglMakeCurrent(hDC, NULL);
			wglDeleteContext(hRC);

			// Send quit message to queue
			PostQuitMessage(0);
		}
		break;
		case WM_SIZE:			//Resize window
		{
			reshape(LOWORD(lParam), HIWORD(lParam));
		}
		break;
		case WM_CHAR:
		{
			keyboard((unsigned char) LOWORD(wParam), 0, 0);
		}
		break;
		case WM_LBUTTONDOWN:			// Left mouse button down
		{
			int x = (int) LOWORD(lParam);
			int y = (int) HIWORD(lParam);
			const int invert_y = (_glut->getScene3d().getHeight() - y) - 1;  // OpenGL viewport coordinates are Cartesian
			arcball_start(x, invert_y);
		}
		break;
		case WM_MOUSEMOVE:  // Moving the mouse around
		{
			if(wParam & MK_LBUTTON)  // While left mouse button down
			{
				motion((int) LOWORD(lParam), (int) HIWORD(lParam));
			}
		}
		break;
		case WM_MOUSEWHEEL:  //Scroll wheel
		{
			short zDelta = (short) HIWORD(wParam);
			if (zDelta < 0 && !_glut->getScene3d().isCameraView())
			{
				arcball_add_distance(+250);
			}
			else if (zDelta > 0 && !_glut->getScene3d().isCameraView())
			{
				arcball_add_distance(-250);
			}
		}
		break;
		default:
		lRet = long(DefWindowProc(hwnd, message, wParam, lParam));
	}

	return lRet;
}
#endif

/**
 * Rotate the scene
 */
void Glut::motion(int x, int y)
{
	// motion is only called when a mouse button is held down
	int invert_y = (_glut->getScene3d().getHeight() - y) - 1;
	arcball_move(x, invert_y);
}

/**
 * Reshape the GL-window
 */
void Glut::reshape(int width, int height)
{
	float ar = (float) width / (float) height;
	_glut->getScene3d().setSize(width, height, ar);
	glViewport(0, 0, width, height);
	reset();
}

/**
 * When idle...
 */
void Glut::idle()
{
#ifndef _WIN32
	glutPostRedisplay();
#endif
}

/**
 * Render the 3D scene
 */
void Glut::display()
{
	// Enable depth testing
	glEnable(GL_DEPTH_TEST);

	// Here's our rendering. Clears the screen
	// to black, clear the color and depth
	// buffers, and reset our modelview matrix.
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);  //set modelview matrix
	glLoadIdentity();  //reset modelview matrix

	arcball_rotate();

	Scene3DRenderer& scene3d = _glut->getScene3d();
	if (scene3d.isShowGrdFlr()) drawGrdGrid();
	if (scene3d.isShowCam()) drawCamCoord();
	if (scene3d.isShowVolume()) drawVolume();
	if (scene3d.isShowArcball()) drawArcball();

	drawClusterCenters();
	drawVoxels();

	if (scene3d.isShowOrg()) drawWCoord();
	if (scene3d.isShowInfo()) drawInfo();

	glFlush();

#ifndef _WIN32
	glutSwapBuffers();
#elif defined _WIN32
	SwapBuffers(scene3d.getHDC());
#endif
}

/**
 * - Update the scene with a new frame from the video
 * - Handle the keyboard input from the OpenCV window
 * - Update the OpenCV video window and frames slider position
 */
void Glut::update(int v)
{
	
	char key = waitKey(10);
	keyboard(key, 0, 0);  // call glut key handler :)

	Scene3DRenderer& scene3d = _glut->getScene3d();
	if (scene3d.isQuit())
	{
		// Quit signaled
		quit();
	}
	if (scene3d.getCurrentFrame() > scene3d.getNumberOfFrames() - 2)
	{
		// Go to the start of the video if we've moved beyond the end
		scene3d.setCurrentFrame(0);
		for (size_t c = 0; c < scene3d.getCameras().size(); ++c)
			scene3d.getCameras()[c]->setVideoFrame(scene3d.getCurrentFrame());
	}
	if (scene3d.getCurrentFrame() < 0)
	{
		// Go to the end of the video if we've moved before the start
		scene3d.setCurrentFrame(scene3d.getNumberOfFrames() - 2);
		for (size_t c = 0; c < scene3d.getCameras().size(); ++c)
			scene3d.getCameras()[c]->setVideoFrame(scene3d.getCurrentFrame());
	}
	if (!scene3d.isPaused())
	{
		// If not paused move to the next frame
		scene3d.setCurrentFrame(scene3d.getCurrentFrame() + 1);
	}
	if (scene3d.getCurrentFrame() != scene3d.getPreviousFrame())
	{
		// If the current frame is different from the last iteration update stuff
		scene3d.processFrame();
		scene3d.getReconstructor().update();
		scene3d.setPreviousFrame(scene3d.getCurrentFrame());
	}
	else if (scene3d.getHThreshold() != scene3d.getPHThreshold() || scene3d.getSThreshold() != scene3d.getPSThreshold()
						|| scene3d.getVThreshold() != scene3d.getPVThreshold() || scene3d.getESize() != scene3d.getPESize() ||
						scene3d.getDSize() != scene3d.getPDSize())
	{
		// Update the scene if one of the HSV sliders was moved (when the video is paused)
		scene3d.processFrame();
		scene3d.getReconstructor().update();

		scene3d.setPHThreshold(scene3d.getHThreshold());
		scene3d.setPSThreshold(scene3d.getSThreshold());
		scene3d.setPVThreshold(scene3d.getVThreshold());
		scene3d.setPDSize(scene3d.getDSize());
		scene3d.setPESize(scene3d.getESize());
	}

	// Auto rotate the scene
	if (scene3d.isRotate())
	{
		arcball_add_angle(1);
	}

	// Get the image and the foreground image (of set camera)
	Mat canvas, foreground;
	if (scene3d.getCurrentCamera() != -1)
	{
		canvas = scene3d.getCameras()[scene3d.getCurrentCamera()]->getFrame();
		foreground = scene3d.getCameras()[scene3d.getCurrentCamera()]->getForegroundImage();
	}
	else
	{
		canvas = scene3d.getCameras()[scene3d.getPreviousCamera()]->getFrame();
		foreground = scene3d.getCameras()[scene3d.getPreviousCamera()]->getForegroundImage();
	}

	// Concatenate the video frame with the foreground image (of set camera)
	if (!canvas.empty() && !foreground.empty())
	{
		Mat fg_im_3c;
		cvtColor(foreground, fg_im_3c, CV_GRAY2BGR);
		hconcat(canvas, fg_im_3c, canvas);
		imshow(VIDEO_WINDOW, canvas);
	}
	else if (!canvas.empty())
	{
		imshow(VIDEO_WINDOW, canvas);
	}

	// Update the frame slider position
	setTrackbarPos("Frame", SLIDER_WINDOW, scene3d.getCurrentFrame());
#ifndef _WIN32
	glutSwapBuffers();
	glutTimerFunc(10, update, 0);
#endif
}




/**
 * Draw the floor
 */
void Glut::drawGrdGrid()
{
	vector<vector<Point3i*> > floor_grid = _glut->getScene3d().getFloorGrid();

	glLineWidth(1.0f);
	glPushMatrix();
	glBegin(GL_LINES);

	int gSize = _glut->getScene3d().getNum() * 2 + 1;
	for (int g = 0; g < gSize; g++)
	{
		// y lines
		glColor4f(0.9f, 0.9f, 0.9f, 0.5f);
		glVertex3f((GLfloat) floor_grid[0][g]->x, (GLfloat) floor_grid[0][g]->y, (GLfloat) floor_grid[0][g]->z);
		glVertex3f((GLfloat) floor_grid[2][g]->x, (GLfloat) floor_grid[2][g]->y, (GLfloat) floor_grid[2][g]->z);

		// x lines
		glColor4f(0.9f, 0.9f, 0.9f, 0.5f);
		glVertex3f((GLfloat) floor_grid[1][g]->x, (GLfloat) floor_grid[1][g]->y, (GLfloat) floor_grid[1][g]->z);
		glVertex3f((GLfloat) floor_grid[3][g]->x, (GLfloat) floor_grid[3][g]->y, (GLfloat) floor_grid[3][g]->z);
	}

	glEnd();
	glPopMatrix();
}

/**
 * Draw the cameras
 */
void Glut::drawCamCoord()
{
	vector<Camera*> cameras = _glut->getScene3d().getCameras();

	glLineWidth(1.0f);
	glPushMatrix();
	glBegin(GL_LINES);

	for (size_t i = 0; i < cameras.size(); i++)
	{
		vector<Point3f> plane = cameras[i]->getCameraPlane();

		// 0 - 1
		glColor4f(0.8f, 0.8f, 0.8f, 0.5f);
		glVertex3f(plane[0].x, plane[0].y, plane[0].z);
		glVertex3f(plane[1].x, plane[1].y, plane[1].z);

		// 0 - 2
		glColor4f(0.8f, 0.8f, 0.8f, 0.5f);
		glVertex3f(plane[0].x, plane[0].y, plane[0].z);
		glVertex3f(plane[2].x, plane[2].y, plane[2].z);

		// 0 - 3
		glColor4f(0.8f, 0.8f, 0.8f, 0.5f);
		glVertex3f(plane[0].x, plane[0].y, plane[0].z);
		glVertex3f(plane[3].x, plane[3].y, plane[3].z);

		// 0 - 4
		glColor4f(0.8f, 0.8f, 0.8f, 0.5f);
		glVertex3f(plane[0].x, plane[0].y, plane[0].z);
		glVertex3f(plane[4].x, plane[4].y, plane[4].z);

		// 1 - 2
		glColor4f(0.5f, 0.5f, 0.5f, 0.5f);
		glVertex3f(plane[1].x, plane[1].y, plane[1].z);
		glVertex3f(plane[2].x, plane[2].y, plane[2].z);

		// 2 - 3
		glColor4f(0.5f, 0.5f, 0.5f, 0.5f);
		glVertex3f(plane[2].x, plane[2].y, plane[2].z);
		glVertex3f(plane[3].x, plane[3].y, plane[3].z);

		// 3 - 4
		glColor4f(0.5f, 0.5f, 0.5f, 0.5f);
		glVertex3f(plane[3].x, plane[3].y, plane[3].z);
		glVertex3f(plane[4].x, plane[4].y, plane[4].z);

		// 4 - 1
		glColor4f(0.5f, 0.5f, 0.5f, 0.5f);
		glVertex3f(plane[4].x, plane[4].y, plane[4].z);
		glVertex3f(plane[1].x, plane[1].y, plane[1].z);
	}

	glEnd();
	glPopMatrix();
}

/**
 * Draw the voxel bounding box
 */
void Glut::drawVolume()
{
	vector<Point3f*> corners = _glut->getScene3d().getReconstructor().getCorners();

	glLineWidth(1.0f);
	glPushMatrix();
	glBegin(GL_LINES);

	// VR->volumeCorners[0]; // what's this frank?
	// bottom
	glColor4f(0.9f, 0.9f, 0.9f, 0.5f);
	glVertex3f(corners[0]->x, corners[0]->y, corners[0]->z);
	glVertex3f(corners[1]->x, corners[1]->y, corners[1]->z);

	glColor4f(0.9f, 0.9f, 0.9f, 0.5f);
	glVertex3f(corners[1]->x, corners[1]->y, corners[1]->z);
	glVertex3f(corners[2]->x, corners[2]->y, corners[2]->z);

	glColor4f(0.9f, 0.9f, 0.9f, 0.5f);
	glVertex3f(corners[2]->x, corners[2]->y, corners[2]->z);
	glVertex3f(corners[3]->x, corners[3]->y, corners[3]->z);

	glColor4f(0.9f, 0.9f, 0.9f, 0.5f);
	glVertex3f(corners[3]->x, corners[3]->y, corners[3]->z);
	glVertex3f(corners[0]->x, corners[0]->y, corners[0]->z);

	// top
	glColor4f(0.9f, 0.9f, 0.9f, 0.5f);
	glVertex3f(corners[4]->x, corners[4]->y, corners[4]->z);
	glVertex3f(corners[5]->x, corners[5]->y, corners[5]->z);

	glColor4f(0.9f, 0.9f, 0.9f, 0.5f);
	glVertex3f(corners[5]->x, corners[5]->y, corners[5]->z);
	glVertex3f(corners[6]->x, corners[6]->y, corners[6]->z);

	glColor4f(0.9f, 0.9f, 0.9f, 0.5f);
	glVertex3f(corners[6]->x, corners[6]->y, corners[6]->z);
	glVertex3f(corners[7]->x, corners[7]->y, corners[7]->z);

	glColor4f(0.9f, 0.9f, 0.9f, 0.5f);
	glVertex3f(corners[7]->x, corners[7]->y, corners[7]->z);
	glVertex3f(corners[4]->x, corners[4]->y, corners[4]->z);

	// connection
	glColor4f(0.9f, 0.9f, 0.9f, 0.5f);
	glVertex3f(corners[0]->x, corners[0]->y, corners[0]->z);
	glVertex3f(corners[4]->x, corners[4]->y, corners[4]->z);

	glColor4f(0.9f, 0.9f, 0.9f, 0.5f);
	glVertex3f(corners[1]->x, corners[1]->y, corners[1]->z);
	glVertex3f(corners[5]->x, corners[5]->y, corners[5]->z);

	glColor4f(0.9f, 0.9f, 0.9f, 0.5f);
	glVertex3f(corners[2]->x, corners[2]->y, corners[2]->z);
	glVertex3f(corners[6]->x, corners[6]->y, corners[6]->z);

	glColor4f(0.9f, 0.9f, 0.9f, 0.5f);
	glVertex3f(corners[3]->x, corners[3]->y, corners[3]->z);
	glVertex3f(corners[7]->x, corners[7]->y, corners[7]->z);

	glEnd();
	glPopMatrix();
}

/**
 * Draw the arcball wiresphere that guides scene rotation
 */
void Glut::drawArcball()
{
	//Arcball wiresphere (glutWireSphere) not supported on Windows! :(
#ifndef _WIN32
	glLineWidth(1.0f);
	glPushMatrix();
	glBegin(GL_LINES);

	glColor3f(1.0f, 0.9f, 0.9f);
	glutWireSphere(_glut->getScene3d().getSphereRadius(), 48, 24);

	glEnd();
	glPopMatrix();
#endif
}



/**
 * Draw origin into scene
 */
void Glut::drawWCoord()
{
	glLineWidth(1.5f);
	glPushMatrix();
	glBegin(GL_LINES);

	const Scene3DRenderer& scene3d = _glut->getScene3d();
	const int len = scene3d.getSquareSideLen();
	const float x_len = len * (scene3d.getBoardSize().height - 1);
	const float y_len = len * (scene3d.getBoardSize().width - 1);
	const float z_len = len * 3;

	// draw x-axis
	glColor4f(0.0f, 0.0f, 1.0f, 0.5f);
	glVertex3f(0.0f, 0.0f, 0.0f);
	glVertex3f(x_len, 0.0f, 0.0f);

	// draw y-axis
	glColor4f(0.0f, 1.0f, 0.0f, 0.5f);
	glVertex3f(0.0f, 0.0f, 0.0f);
	glVertex3f(0.0f, y_len, 0.0f);

	// draw z-axis
	glColor4f(1.0f, 0.0f, 0.0f, 0.5f);
	glVertex3f(0.0f, 0.0f, 0.0f);
	glVertex3f(0.0f, 0.0f, z_len);

	glEnd();
	glPopMatrix();
}

/**
 * Draw camera numbers into scene
 */
void Glut::drawInfo()
{
	// glutBitmapCharacter() is not supported on Windows
#ifndef _WIN32
	glPushMatrix();
	glBegin(GL_BITMAP);

	if (_glut->getScene3d().isShowInfo())
	{
		vector<Camera*> cameras = _glut->getScene3d().getCameras();
		for (size_t c = 0; c < cameras.size(); ++c)
		{
			glRasterPos3d(cameras[c]->getCameraLocation().x, cameras[c]->getCameraLocation().y,
					cameras[c]->getCameraLocation().z);
			stringstream sstext;
			sstext << (c + 1) << "\0";
			for (const char* c = sstext.str().c_str(); *c != '\0'; c++)
			{
				glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
			}
		}
	}

	glEnd();
	glPopMatrix();
#endif
}

} /* namespace nl_uu_science_gmt */
