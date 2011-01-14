#include "ofMain.h"
#include "illp.h"
#include "ofAppGlutWindow.h"

//========================================================================
int main()
{
    ofAppGlutWindow window;
	// setup the GL context
	//ofSetupOpenGL(&window, 0, 0, OF_FULLSCREEN);
	ofSetupOpenGL(&window, 1280, 860, OF_WINDOW);
	// this kicks off the running of my app
	// can be OF_WINDOW or OF_FULLSCREEN
	// pass in width and height too:
	ofRunApp( new illp());
}
