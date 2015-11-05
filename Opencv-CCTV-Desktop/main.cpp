// Avoid Visual C++ warnings
#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif /*  _WIN32 */

#include <iostream>
#include <sstream>
#include <vector>

#include <cstdio>
#include <cstdlib>
#include <ctime>

#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>


#ifdef _WIN32
#include <windows.h>
#endif /*  _WIN32 */



#define PRINT(format, ...) fprintf(stdout, "[%s::%d] " format "\n", __FUNCTION__, __LINE__, __VA_ARGS__)
#define delay(ms)	Sleep((ms));

#undef SAVE_TO_FILE
//#define SAVE_TO_FILE

#define BUFFER_SIZE 255
#define WAIT_TIME 5.0f
#define DIR_VIDEOS "Videos"
#define MAX_THRESHOLD 255

// Typedefs
typedef cv::Vec4i VEC4_i;
typedef std::vector<VEC4_i> VECTOR_OF_VEC4i;

typedef std::vector<cv::Point> VECTOR_OF_POINTS;
typedef std::vector<VECTOR_OF_POINTS> VECTOR2_OF_POINTS;



const static int SENSITIVITY_VALUE = 20;
const static int BLUR_SIZE = 10;
const static int DELAY_KEYPRESS_IN_MS = 33;

static cv::RNG rng(12345);
static int track_position = 0;
static std::string main_window_title = "Detector";


/// Function header
static BOOL CtrlHandler(DWORD);

static bool createDirectory(const std::string & name);
static cv::Mat calculateThresholdImage(cv::Mat, cv::Mat);
static cv::Mat drawObjectContours(cv::Mat, bool &);
static void handleObjectDetected(cv::Mat, bool);
static void showDetectedObjectMessage(cv::Mat, bool);
static void recordVideo(cv::Mat, int, int, bool);
static void readTextFromImage(cv::Mat);
static std::string createFilenameString();
static void thresh_callback(int position, void * user_data);
static std::vector<cv::Rect> detectText(cv::Mat image);
static cv::Mat getScreenShot(HWND hWnd);

static void startRecording(cv::Mat, int, int);
static void finishRecording();


static clock_t begin_time;
static bool is_running;
static double frame_width;
static double frame_height;

static bool useGUI;
static bool can_record;


static cv::VideoWriter video;
static cv::BackgroundSubtractorMOG2 * bg;


static cv::VideoCapture * vcap;


static void printInfo(cv::VideoCapture *);
static void runLoop(cv::VideoCapture *);
static void onKeypress();



/**
 *
 *
 */
int main(int argc, char ** argv)
{
	//useGUI = false;
	useGUI = true;

#ifdef _WIN32
	if (SetConsoleCtrlHandler((PHANDLER_ROUTINE) CtrlHandler, TRUE))
	{
	} // End of if
	else
	{
	} // End of else
#endif /*  _WIN32 */


#define __SCREEN_SHOT__		0
#if __SCREEN_SHOT__
#ifdef _WIN32
	HWND hDesktop;
	hDesktop = GetDesktopWindow();
	cv::Mat screenshot = getScreenShot(hDesktop);
	cv::cvtColor(screenshot, screenshot, CV_BGR2GRAY);
	//cv::imshow("ScreenShot", screenshot);
	cv::imwrite("screenshot.jpg", screenshot);
#endif /*  _WIN32 */
#else
	vcap = new cv::VideoCapture(0);

	if (!vcap->isOpened()) {
		std::cout << "Error opening video stream or file" << std::endl;
		return -1;
	}

	frame_width = vcap->get(CV_CAP_PROP_FRAME_WIDTH);
	frame_height = vcap->get(CV_CAP_PROP_FRAME_HEIGHT);

	printInfo(vcap);

	createDirectory(DIR_VIDEOS);

	//bg = new cv::BackgroundSubtractorMOG2(50, 5.0f);
	clock_t begin = 0;


#if __DETECT_ID_CARD__
	cv::Mat idCard = cv::imread("id_card.jpg");
	std::vector<cv::Rect> letterBoxes = detectText(idCard);
	for (size_t i = 0; letterBoxes.size(); i++) {
		cv::rectangle(idCard, letterBoxes[i], cv::Scalar(0, 255, 0), 3, 8, 0);
	}
	cv::imwrite("detected_id_card.jpg", idCard);
#endif /* __DETECT_ID_CARD__ */




	runLoop(vcap);



	//delete bg;
	//bg = NULL;

	delete vcap;
	vcap = NULL;


#endif /* __SCREEN_SHOT__ */

	PRINT("Bye!");
	return EXIT_SUCCESS;
} // End of Entry Point

/////////////////////////////////////////////////////////////////////
//                                                                 //
//                                                                 //
//                                                                 //
//                                                                 //
//                                                                 //
//                                                                 //
//                                                                 //
/////////////////////////////////////////////////////////////////////

/**
*
*
*/
static void printInfo(cv::VideoCapture * videoCapure)
{
	double w = 0.0f;
	double h = 0.0f;

	w = videoCapure->get(CV_CAP_PROP_FRAME_WIDTH);
	h = videoCapure->get(CV_CAP_PROP_FRAME_HEIGHT);

	PRINT("Frame Size: [%.2f x %.2f]", w, h);
}

/**
 *
 *
 */
static void runLoop(cv::VideoCapture * videoCapure)
{
	cv::Mat current_frame;
	cv::Mat next_frame;
	cv::Mat threshold_frame;
	cv::Mat show_this_frame;
	cv::Mat countour_this_frame;

	cv::Mat id_card;

	bool objectDetected;


	if (useGUI)
	{
		cv::imshow(main_window_title, NULL);
		cv::createTrackbar(" Threshold:", main_window_title, &track_position, MAX_THRESHOLD, thresh_callback);
		thresh_callback(0, 0);
	} // End of if


	is_running = true;
	while (is_running) {
		videoCapure->read(current_frame);
		readTextFromImage(current_frame);

		delay(100);

		videoCapure->read(next_frame);

		show_this_frame = next_frame;

		threshold_frame = calculateThresholdImage(current_frame, next_frame);
		countour_this_frame = drawObjectContours(threshold_frame, objectDetected);

		handleObjectDetected(show_this_frame, objectDetected);



		//show_this_frame = countour_this_frame;

		if (useGUI)
		{
			cv::imshow(main_window_title, show_this_frame);
		} // End of if


		onKeypress();
	} // End of while


	if (can_record && video.isOpened()) {
		can_record = false;
		video.release();
	} // End of if

} // End of function

/**
 *
 *
 */
static void handleObjectDetected(cv::Mat frame, bool objectDetected)
{
	showDetectedObjectMessage(frame, objectDetected);
	recordVideo(frame, static_cast<int>(frame_width), static_cast<int>(frame_height), objectDetected);
} // End of function

/**
*
*
*/
static void showDetectedObjectMessage(cv::Mat frame, bool objectDetected)
{
	std::string message;
	cv::Point textPos;
	cv::Scalar textColor;

	textPos = cv::Point(5, 50);
	textColor = cv::Scalar(255, 0, 0);

	message = (objectDetected ? "Someone detected" : "No one detected");

	cv::putText(frame, message, textPos, 1, 1, cv::Scalar(255, 0, 0), 2);

	PRINT("%s", message.c_str());
} // End of function

 /**
  *
  *
  */
static void recordVideo(cv::Mat frame, int width, int height, bool objectDetected)
{
	if (objectDetected)
	{
		startRecording(frame, width, height);
	} // End of if
	else
	{
		finishRecording();
	} // End of else

} // End of function

/**
 *
 *
 */
static void startRecording(cv::Mat frame, int width, int height)
{
	if (can_record)
	{
		video.write(frame);
	} // End of if
	else
	{
		std::string filename;
		cv::Size videoSize;
		int fourcc = CV_FOURCC('M', 'J', 'P', 'G');

		filename = createFilenameString();
		videoSize = cv::Size(width, height);
		video = cv::VideoWriter(filename, fourcc, 10, videoSize, true);
		can_record = true;
	} // End of else

	if (begin_time == 0) {
		begin_time = clock();
	} // End of if

} // End of function

/**
 *
 *
 */
static void finishRecording()
{

	if (can_record && video.isOpened()) {
		clock_t end_time = 0;
		double elapsed_secs = 0.0f;

		end_time = clock();
		elapsed_secs = double(end_time - begin_time) / CLOCKS_PER_SEC;

		// If no one was detected, wait for 5 seconds before closing the file.
		if (elapsed_secs > WAIT_TIME)
		{
			PRINT("After %f seconds, no one was detected. Closing the video.", WAIT_TIME);
			can_record = false;
			begin_time = 0;
			video.release();
		} // End of if
	} // End of if

} // End of function

/**
 *
 *
 */
#ifdef _WIN32
static BOOL CtrlHandler(DWORD fdwCtrlType)
{
	switch (fdwCtrlType)
	{
		// Handle the CTRL-C signal. 
		case CTRL_C_EVENT:
			PRINT("Ctrl-C event");
			Beep(750, 300);
			return TRUE;

		// CTRL-CLOSE: confirm that the user wants to exit. 
		case CTRL_CLOSE_EVENT:
			Beep(600, 200);
			PRINT("Ctrl-Close event");
			return TRUE;

		// Pass other signals to the next handler. 
		case CTRL_BREAK_EVENT:
			Beep(900, 200);
			PRINT("Ctrl-Break event");
			return FALSE;

		//
		case CTRL_LOGOFF_EVENT:
			Beep(1000, 200);
			PRINT("Ctrl-Logoff event");
			return FALSE;

		//
		case CTRL_SHUTDOWN_EVENT:
			Beep(750, 500);
			PRINT("Ctrl-Shutdown event");
			return FALSE;

		//
		default:
			return FALSE;
	}
}
#endif /* _WIN32 */

/**
 *
 *
 */
static std::string createFilenameString()
{
	std::string filename;
	std::time_t current_time;
	char current_time_text[100];


	current_time = std::time(NULL);
	std::strftime(current_time_text, sizeof(current_time_text), "%Y%m%d_%H%M%S", std::localtime(&current_time));

	filename.append(DIR_VIDEOS);
	filename.append("\\");
	filename.append(current_time_text);
	filename.append(".avi");

	return filename;
}

/**
 *
 *
 */
static cv::Mat calculateThresholdImage(cv::Mat frame0, cv::Mat frame1)
{
	cv::Mat thisFrame;
	cv::Mat nextFrame;
	cv::Mat differenceImage;
	cv::Mat thresholdImage;
	cv::Mat result;

	thisFrame = frame0;
	nextFrame = frame1;

	cv::cvtColor(thisFrame, thisFrame, CV_BGR2GRAY);
	cv::cvtColor(nextFrame, nextFrame, CV_BGR2GRAY);


	cv::absdiff(thisFrame, nextFrame, differenceImage);
	cv::threshold(differenceImage, thresholdImage, SENSITIVITY_VALUE, 255, cv::THRESH_BINARY);

	cv::blur(thresholdImage, thresholdImage, cv::Size(BLUR_SIZE, BLUR_SIZE));

	thresholdImage.copyTo(result);

	return result;
} // End of function

/**
 *
 *
 */
static cv::Mat drawObjectContours(cv::Mat frame, bool &objectDetected)
{
	VECTOR2_OF_POINTS contours;
	VECTOR2_OF_POINTS contours_poly;
	VECTOR_OF_VEC4i hierarchy;
	cv::Mat drawing;


	cv::findContours(frame, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

	objectDetected = (contours.size() > 0);

	/// Approximate contours to polygons + get bounding rects and circles
	contours_poly = VECTOR2_OF_POINTS(contours.size());
	for (size_t i = 0; i < contours.size(); i++) {
		cv::Mat curve(contours[i]);

		cv::approxPolyDP(curve, contours_poly[i], 3, true);
	} // End of for


	drawing = cv::Mat::zeros(frame.size(), CV_8UC3);
	for (size_t i = 0; i < contours.size(); i++)
	{
		cv::Scalar color;

		color = cv::Scalar(rng.uniform(0, 255), rng.uniform(0, 255), rng.uniform(0, 255));
		drawContours(drawing, contours_poly, static_cast<int>(i), color, 1, 8, std::vector<VEC4_i>(), 0, cv::Point());
	} // End of for

	return drawing;
} // End of function

/**
 *
 *
 */
static void onKeypress()
{
	char key = (char)cv::waitKey(DELAY_KEYPRESS_IN_MS);
	switch (key) {
	case 27: { // ESC key
		PRINT("<ESC> was pressed!");
		is_running = false;
	}
	break;
	} // End of switch
} // End of function

/**
 *
 *
 */
static bool createDirectory(const std::string & name) {

	wchar_t * directory = NULL;

	directory = new wchar_t[BUFFER_SIZE];
	int res = GetCurrentDirectory(BUFFER_SIZE, directory);

	std::wstring wstrName = std::wstring(name.begin(), name.end());

	if (!CreateDirectory(wstrName.c_str(), NULL)) {
		printf("Directory creation has failed (error code: %d)\n", GetLastError());
		return false;
	}

	delete directory;
	directory = NULL;
	return true;
} // End of function


/**
 *
 *
 */
/** @function thresh_callback */
static void thresh_callback(int position, void * user_data) {
	printf("Position: [%d]\n", position);
} // End of function

/**
 *
 *
 */
static cv::Mat getScreenShot(HWND hWnd) {
	cv::Mat frame;

	HDC hWinDC;
	HDC hWinCompatibleDC;
	RECT winSize; // Get the Width and Height of the screen
	int w, h;
	int screen_width;
	int screen_height;
	HBITMAP hBitmap;
	BITMAPINFOHEADER bmpInfoHeader;

	hWinDC = GetDC(hWnd);
	hWinCompatibleDC = CreateCompatibleDC(hWinDC);
	SetStretchBltMode(hWinDC, COLORONCOLOR);

	GetClientRect(hWnd, &winSize);

	screen_width = winSize.right;
	screen_height = winSize.bottom;

	// w = winSize.right / 2; // Change this to whatever size you want to resize to
	// h = winSize.bottom / 2; // Change this to whatever size you want to resize to

	w = winSize.right;
	h = winSize.bottom;

	frame.create(h, w, CV_8UC4);


	// Create a bitmap
	hBitmap = CreateCompatibleBitmap(hWinDC, w, h);
	bmpInfoHeader.biSize = sizeof(BITMAPINFOHEADER); // http://msdn.microsoft.com/en-us/library/windows/window/dd183402%28v=vs.85%29.aspx
	bmpInfoHeader.biWidth = w;
	bmpInfoHeader.biHeight = -1 * h; // This is the line that makes it draw upside down or not
	bmpInfoHeader.biPlanes = 1;
	bmpInfoHeader.biBitCount = 32;
	bmpInfoHeader.biCompression = BI_RGB;
	bmpInfoHeader.biSizeImage = 0;
	bmpInfoHeader.biXPelsPerMeter = 0;
	bmpInfoHeader.biYPelsPerMeter = 0;
	bmpInfoHeader.biClrUsed = 0;
	bmpInfoHeader.biClrImportant = 0;


	// use the previously created device context with the bitmap
	SelectObject(hWinCompatibleDC, hBitmap);


	// copy from the window device context to the bitmap device context
	StretchBlt(hWinCompatibleDC, 0, 0, w, h, hWinDC, 0, 0, screen_width, screen_height, SRCCOPY);		    // Change SRCCOPY to NOTSRCCOPY for wacky colors!
	GetDIBits(hWinCompatibleDC, hBitmap, 0, h, frame.data, (BITMAPINFO *)& bmpInfoHeader, DIB_RGB_COLORS);  // Copy from hwindowCompatibleDC to hbwindow
																										    // Avoid memory leak
	DeleteObject(hBitmap);

	DeleteDC(hWinCompatibleDC);
	ReleaseDC(hWnd, hWinDC);

	return frame;
} // End of function


/**
 *
 *
 */
static std::vector<cv::Rect> detectText(cv::Mat image) {
	std::vector<cv::Rect> boundRect;
	cv::Mat img_gray;
	cv::Mat img_sobel;
	cv::Mat img_theshold;
	cv::Mat element;

	//printf("Image Size: [%d x %d]\n", image.cols, image.rows);

	cv::cvtColor(image, img_gray, CV_BGR2GRAY);

	cv::Sobel(img_gray, img_sobel, CV_8U, 1, 0, 3, 1, 0, cv::BORDER_DEFAULT);

	cv::threshold(img_sobel, img_theshold, 0, 255, CV_THRESH_OTSU + CV_THRESH_BINARY);

	element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(17, 3));

	cv::morphologyEx(img_theshold, img_theshold, CV_MOP_CLOSE, element); // Does the trick

	std::vector<std::vector<cv::Point>> contours;

	cv::findContours(img_theshold, contours, 0, 1);

	std::vector<std::vector<cv::Point>> contours_poly(contours.size());

	for (size_t i = 0; i < contours.size(); i++) {
		if (contours[i].size() > 100) {
			cv::approxPolyDP(cv::Mat(contours[i]), contours_poly[i], 3, true);
			cv::Rect appRect(cv::boundingRect(cv::Mat(contours_poly[i])));
			if (appRect.width > appRect.height) {
				boundRect.push_back(appRect);
			}
		}
	}

	return boundRect;
} // End of function


/**
 *
 */
static void readTextFromImage(cv::Mat frame)
{
  //current_frame.copyTo(frame);
  //id_card = current_frame;
  //std::vector<cv::Rect> letterBoxes = detectText(id_card);
  //for (size_t i = 0; i < letterBoxes.size(); i++) {
  //	cv::rectangle(id_card, letterBoxes[i], cv::Scalar(0, 255, 0), 3, 8, 0);
  //}
  //cv::imshow("Card ID", id_card);


  //cv::Mat foreground;
  //cv::Mat background;
  //cv::Mat bg_detection;
  //bg.operator ()(current_frame, foreground);
  //bg.getBackgroundImage(background);
  //cv::erode(foreground, foreground, cv::Mat());
  //cv::dilate(foreground, foreground, cv::Mat());
  //cv::findContours(foreground, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);
  //cv::drawContours(bg_detection, contours, -1, cv::Scalar(0, 0, 255), 2);
  //cv::imshow("Frame2", bg_detection);
} // End of function
