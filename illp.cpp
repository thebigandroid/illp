#include "illp.h"
#include <iostream>
#include <fstream>
#include <string>
using namespace std;

// copied from ofxDirList.cpp:
// a handy string function
static std::string::size_type idx;
static string getExt(string filename){
	idx = filename.rfind('.');

	if(idx != std::string::npos){
		return filename.substr(idx+1);
	}
	else{
		return "";
	}
}

//--------------------------------------------------------------
void illp::setup()
{
	imageFolderName[0] = 0;
	// try to read folder names from config file
	readConfigFile();
	// if no luck, default to "data" folder
	if (imageFolderName[0] == 0)
	{
		strcpy(imageFolderName, "data");
		strcpy(saveFolder, "data");
	}
	// this will include subfolders and non-image files, 
	// if any are present...
	int numDirEntries = DIR.listDir(imageFolderName);
	if (numDirEntries == 0)
	{
		// terminate with error message
		cout << "no files found in image folder [" << imageFolderName << "], exiting (sorry)\n";
		exit();
	}
	// ...so we may allocate a few unused strings here:
	imgFilenames = new string[numDirEntries];
	numBitPlanes = new int[numDirEntries];
	nImages = 0;
	for(int i = 0; i < numDirEntries; i++)
	{
		// we're only interested in image files
		if (getExt(DIR.getPath(i)).compare("bmp") == 0)
		{
			numBitPlanes[nImages] = 3;
			imgFilenames[nImages++] = DIR.getPath(i);
		}
		else if (getExt(DIR.getPath(i)).compare("png") == 0)
		{
			numBitPlanes[nImages] = 4;
			imgFilenames[nImages++] = DIR.getPath(i);
		}
		else if (getExt(DIR.getPath(i)).compare("jpg") == 0)
		{
			numBitPlanes[nImages] = 3;
			imgFilenames[nImages++] = DIR.getPath(i);
		}
	}
	if (nImages == 0)
	{
		// TO DO: terminate with error message
	}
 	images = new ofImage[nImages];
    // we can now iterate through the files
    for(int i = 0; i < nImages; i++)
	{
        images[i].loadImage(imgFilenames[i]);
    }

	currentImage = 0;
	newColor = false;
	deletedColor = false;
	numColors = 0;
	createGreyScaleImage();
}


//--------------------------------------------------------------
void illp::update()
{
	if (nImages > 0)
	{
		pathInfo = imgFilenames[currentImage]
		+ "\n" + "press n for next image, p for previous";

		if (newColor)
		{
			newColor = false;
			addNewColor();
		}
		else if (deletedColor)
		{
			deletedColor = false;
			createNewImage();
		}
	}
	ofBackground(0xF5, 0xF5, 0xFF);
}

//--------------------------------------------------------------
void illp::draw()
{	
	if (nImages > 0)
	{
		// display the original image
		ofSetColor(0xFF, 0xFF, 0xFF);
		images[currentImage].draw(leftBorder, topBorder, displayedImageWidth, displayedImageHeight);
		// caption
        ofSetColor(0x06, 0x05, 0x65);
        ofDrawBitmapString(pathInfo, leftBorder, displayedImageHeight + captionOffset);

		if (0 == numColors)
		{
			ofSetColor(0xFF, 0xFF, 0xFF);
			greyScale.draw(displayedImageWidth + (2*leftBorder), topBorder, displayedImageWidth, displayedImageHeight);
		}
		else
		{
			// show the current selected colors...
			int yCenter = displayedImageHeight + colorCircleOffset;
			for (int c = 0; c < numColors; ++c)
			{
				int xCenter = (1+c) * 2*(colorCircleRadius + 5);
				// circular black outline
				ofSetColor(0, 0, 0);
				ofCircle(xCenter, yCenter, colorCircleRadius + 2);
				// around a colored circle
				ofSetColor(color[c][0], color[c][1], color[c][2]);
				ofCircle(xCenter, yCenter, colorCircleRadius);		
			}
			// ...and show the "poster" they generate
			ofSetColor(0xFF, 0xFF, 0xFF);
			posterized.draw(displayedImageWidth + (2*leftBorder), topBorder, displayedImageWidth, displayedImageHeight);
		}
	}
}

//--------------------------------------------------------------
void illp::keyPressed(int key)
{
	if (nImages > 0)
	{
		if (key == 'n')
		{
			currentImage++;
			currentImage %= nImages;
			createNewImage();
		}
		else if (key == 'p')
		{
			currentImage = (0 == currentImage) ? nImages - 1 : currentImage - 1;
			createNewImage();
		}
		else if (key == 's')
		{
			if (numColors == 0)
			{
				string savename = imgFilenames[currentImage] + "-grey.jpg";
				greyScale.saveImage(savename);
			}
			else
			{
				saveLayers();
			}
		}
    }
}

//--------------------------------------------------------------
void illp::keyReleased(int key)
{
}

//--------------------------------------------------------------
void illp::mouseMoved(int x, int y )
{
}

//--------------------------------------------------------------
void illp::mouseDragged(int x, int y, int button)
{
	// cout << "[" << x << "," << y << "]";
}

//--------------------------------------------------------------
void illp::mousePressed(int x, int y, int button)
{
	// cout << "(" << x << "," << y << ")";
}

//--------------------------------------------------------------
void illp::mouseReleased(int x, int y, int button)
{
	if (button == 0)	// left click to select a color from the image
	{
		// click within image?
		if (x >= leftBorder && x < (displayedImageWidth + leftBorder)
			&& y >= topBorder && y < (displayedImageHeight + topBorder))
		{
			if (numColors < MAX_LAYERS)
			{
				selectedX = (int)((x - leftBorder) * (1.0/rescaleRatio));
				selectedY = (int)((y - topBorder) * (1.0/rescaleRatio));
				newColor = true;
			}
		}
	}
	else	// right-click to delete the last selected color from the list
	{
		if (numColors > 0)
		{
			// remove last color added
			--numColors;
			deletedColor = true;
		}
	}
}

//--------------------------------------------------------------
void illp::windowResized(int w, int h)
{
	windowWidth = w;
	windowHeight = h;
	updateImageScaling();
}

// standard OF app methods above here, additions below

void illp::updateImageScaling()
{
	displayedImageWidth = images[currentImage].width;
	displayedImageHeight = images[currentImage].height;
	rescaleKeepingAspectRatio();
}

void illp::createGreyScaleImage()
{
	// get the pixels out of an image, and then 
	// average the color_plane values to create new grey-scale pixels
	unsigned char * pixels = images[currentImage].getPixels();
	greyScale.allocate(images[currentImage].width, images[currentImage].height, OF_IMAGE_COLOR);
	unsigned char * greyPixels = greyScale.getPixels();
	int bytes_per_pixel = numBitPlanes[currentImage];	// 3 ==> RGB or 4 ==> RGBA
	int w = images[currentImage].width * bytes_per_pixel;
	int h = images[currentImage].height;
	for (int i = 0; i < w; i += bytes_per_pixel){
		for (int j = 0; j < h; j++){
			int red = pixels[(j * w) + i];
			int green = pixels[(j * w) + i + 1];
			int blue = pixels[(j * w) + i + 2];
			//int alpha = pixels[(j * w) + i + 3];
			float avg = (red + green + blue)/3.0;
			unsigned char grey = (unsigned char)(int)avg;
			greyPixels[(j * w) + i] = grey;
			greyPixels[(j * w) + i + 1] = grey;
			greyPixels[(j * w) + i + 2] = grey;
		}
	}
	greyScale.setFromPixels(greyPixels, images[currentImage].width, images[currentImage].height,
		OF_IMAGE_COLOR);
}

void illp::drawColorPlaneCircles()
{
	// get the pixels out of an image, 
	// and then use the values to draw circles
	unsigned char * pixels = images[currentImage].getPixels();
	int bytes_per_pixel = numBitPlanes[currentImage];
	int w = images[currentImage].width * bytes_per_pixel;
	int h = images[currentImage].height;
	for (int i = 0; i < w; i += bytes_per_pixel){
		for (int j = 0; j < h; j++){
			int red = pixels[(j * w) + i];
			int green = pixels[(j * w) + i + 1];
			int blue = pixels[(j * w) + i + 2];
			int alpha = pixels[(j * w) + i + 3];
			float pct = 1.0;
			//switch (bit_plane)
			//{
			//case 0:
			//	pct -= (red / 255.0f);
			//	break;
			//case 1:
			//	pct -= (green / 255.0f);
			//	break;
			//case 2:
			//	pct -= (blue / 255.0f);
			//	break;
			//case 3: 
			//	pct -= (alpha / 255.0f);
			//}
			ofCircle(images[currentImage].width + 20 + i*2, 10 + j*8, 1 + 3*pct);		
		}
	}
}

void illp::addNewColor()
{
	// get the color of the pixel @ (selectedX, selectedY)
	unsigned char * pixels = images[currentImage].getPixels();
	int bytes_per_pixel = numBitPlanes[currentImage];
	int w = images[currentImage].width * bytes_per_pixel;
	int h = images[currentImage].height;
	int offset = (selectedY * w) + (selectedX * bytes_per_pixel);
	color[numColors][0] = pixels[offset];
	color[numColors][1] = pixels[offset + 1];
	color[numColors][2] = pixels[offset + 2];
	++numColors;
	// update the posterized image to use the newly added color
	createPosterizedImage();
}

void illp::createPosterizedImage()
{
	// get the pixels out of an image, and compare each pixel's color
	// to the list of selected color values, replacing it with the closest match
	unsigned char * pixels = images[currentImage].getPixels();
	// TO DO: handle alpha channel?
	posterized.allocate(images[currentImage].width, images[currentImage].height, OF_IMAGE_COLOR);
	unsigned char * posterPixels = posterized.getPixels();
	int bytes_per_pixel = numBitPlanes[currentImage];	// 3 ==> RGB or 4 ==> RGBA
	int w = images[currentImage].width * bytes_per_pixel;
	int h = images[currentImage].height;
	for (int i = 0; i < w; i += bytes_per_pixel){
		for (int j = 0; j < h; j++){
			int red = pixels[(j * w) + i];
			int green = pixels[(j * w) + i + 1];
			int blue = pixels[(j * w) + i + 2];
			mapToClosestColor(&red, &green, &blue);
			posterPixels[(j * w) + i] = (unsigned char)red;
			posterPixels[(j * w) + i + 1] = (unsigned char)green;
			posterPixels[(j * w) + i + 2] = (unsigned char)blue;
		}
	}
	posterized.setFromPixels(posterPixels, images[currentImage].width, images[currentImage].height,
		OF_IMAGE_COLOR);
}

// replace red, green, blue with the "closest" of the selected colors,
// where "closest" is defined by the Euclidean distance formula, applied
// as though red, green, blue were x,y,z coordinates in a Euclidean 3-space
void illp::mapToClosestColor( int* red, int* green, int* blue )
{
	float dist = 3 * 255*255 + 1;
	int index = 0;
	for (int c = 0; c < numColors; ++c)
	{
		int rdiff = (*red - color[c][0]);
		int gdiff = (*green - color[c][1]);
		int bdiff = (*blue - color[c][2]);
		float checkdist = (rdiff*rdiff + gdiff*gdiff + bdiff*bdiff);
		if (dist > checkdist)
		{
			dist = checkdist;
			index = c;
		}
	}
	*red = color[index][0];
	*green = color[index][1];
	*blue = color[index][2];
}

void illp::createNewImage()
{
	if (0 == numColors)
	{
		createGreyScaleImage();
	}
	else
	{
		createPosterizedImage();
	}
	updateImageScaling();
}

// create a set of image files: one per selected color, 
// plus one for the posterized image
void illp::saveLayers()
{
	// we want this function to be callable multiple times
	// during a program run, without overwriting any files;
	// to that end, we encode this counter in the file names
	static int numSaves = 0;
	// save a pic of what all the layers look like together
	char filename[512];
	sprintf(filename, "%s/posterized%d.jpg", saveFolder, numSaves);
	posterized.saveImage(filename);
	// then save each individual layer as black on white,
	// with a bar across the bottom showing the layer's color
	int w = posterized.width;
	int h = posterized.height;
	ofImage* layer = new ofImage[numColors];
	unsigned char** layerpixels = new unsigned char*[numColors];
	unsigned char* pixels = posterized.getPixels();
	for (int lyr = 0; lyr < numColors; ++lyr)
	{
		layer[lyr].allocate(w, h+10, OF_IMAGE_COLOR);
		layerpixels[lyr] = layer[lyr].getPixels();
		for (int x = 0; x < w; ++x)
		{
			for (int y = 0; y < h; ++y)
			{
				int offset = y*3*w + x*3;
				if (color[lyr][0] == pixels[offset]
				&& color[lyr][1] == pixels[offset+1]
				&& color[lyr][2] == pixels[offset+2])
				{
					layerpixels[lyr][offset] = 0;
					layerpixels[lyr][offset+1] = 0;
					layerpixels[lyr][offset+2] = 0;
				}
				else
				{
					layerpixels[lyr][offset] = 255;
					layerpixels[lyr][offset+1] = 255;
					layerpixels[lyr][offset+2] = 255;
				}
			}
			for (int y = h; y < (10+h); ++y)
			{
				int offset = y*3*w + x*3;
				layerpixels[lyr][offset] = color[lyr][0];
				layerpixels[lyr][offset+1] = color[lyr][1];
				layerpixels[lyr][offset+2] = color[lyr][2];
			}
		}
	}
	for (int c = 0; c < numColors; ++c)
	{
		char layerfilename[512];
		sprintf(layerfilename, "%s\\save%dlayer%d.jpg", saveFolder, c, numSaves);
		layer[c].saveImage(layerfilename);
	}
	++numSaves;
}

void illp::readConfigFile()
{
	char* configFileName = "imageloc.txt";
	ifstream configFile;
	configFile.open(configFileName);
	configFile.getline(imageFolderName, 511);
	configFile.getline(saveFolder, 511);
}

// TO DO: stack images vertically if w > h
void illp::rescaleKeepingAspectRatio()
{
	if (displayedImageWidth > displayedImageHeight)
	{
		float maxX = (windowWidth - 2*leftBorder) / 2.0;
		rescaleRatio = (maxX/(displayedImageWidth));
		displayedImageHeight = (int)(displayedImageHeight * rescaleRatio);
		displayedImageWidth = (int)maxX;
	}
	else
	{
		float maxY = windowHeight - (topBorder + 100);
		rescaleRatio = (maxY/(displayedImageHeight));
		displayedImageWidth = (int)(displayedImageWidth * rescaleRatio);
		displayedImageHeight = (int)maxY;
	}
}





