#include <iostream>
#include <lodepng.h>

std::string outputFilename = "output.png";

const int width = 800, height = 756;
const int nChannels = 4;

void SetFlatBackground(std::vector<uint8_t>& imageBuffer, int red, int green, int blue, int alpha)
{

    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x) 
        {
            int pixelIdx = x + y * width;
            imageBuffer[pixelIdx * nChannels + 0] = 0;
            imageBuffer[pixelIdx * nChannels + 1] = 255;
            imageBuffer[pixelIdx * nChannels + 2] = 255;
            imageBuffer[pixelIdx * nChannels + 3] = 255;
        }
}

void SetSinglePixel(std::vector<uint8_t>& imageBuffer, int x, int y, int red, int green, int blue, int alpha)
{
    int pixelIdx = x + y * width;
    imageBuffer[pixelIdx * nChannels + 0] = red; // Set red pixel values to 0
    imageBuffer[pixelIdx * nChannels + 1] = green; // Set green pixel values to 255 (full brightness)
    imageBuffer[pixelIdx * nChannels + 2] = blue; // Set blue pixel values to 255 (full brightness)
    imageBuffer[pixelIdx * nChannels + 3] = alpha; // Set alpha (transparency) pixel values to 255 (fully opaque)
}

void SetImageBackground(std::vector<uint8_t>& imageBuffer, int mode, std::string inputFilename)
{
	//CURRENTLY BROKEN
	unsigned int _width = width;
	unsigned int _height = height;
	lodepng::decode(imageBuffer, _width, _height, inputFilename);

	switch (mode)
	{
	case 1:

		// *** Tasks ***
		// This code loads an image from a png file. This is an image of the famous 
		// Stanford Bunny https://engineering.stanford.edu/magazine/tale-ubiquitous-stanford-bunny
		// You'll need to load and manipulate images to add texturing to your rasteriser and raytracer.
		// Let's try changing this image.
		// If you'd like, you can use the setPixel function you wrote in the previous task.
		// The code below reduces the brightness of the image by 0.5x, as an example.

		for (int y = 0; y < height; ++y)
			for (int x = 0; x < width; ++x)
				for (int c = 0; c < 3; ++c)
				{
					int pixelIdx = x + y * width;
				}

		break;

	case 2:

		// Once you have tested this code, comment out the for loops above and try the following tasks:
		// * Task 1: Try making a *negative* of the input image. Pixels that are bright in the input should be dark in your output. 
		//           Hint: if the pixels ranged in value from 0 to 1, you could replace each pixel value (v) with (1 - v). 
		//           but remember, the pixels have 8-bit unsigned values, so range from 0 to 255!


		for (int y = 0; y < height; ++y)
			for (int x = 0; x < width; ++x)
				for (int c = 0; c < 3; ++c)
				{
					int pixelIdx = x + y * width;
					imageBuffer[pixelIdx * 4 + c] = (255 - imageBuffer[pixelIdx * 4 + c]); //makes a negative of the input image
				}

		break;

	case 3:

		// * Optional Task 2: Try downsampling this image to 1/2 resolution.
		//           You can either just keep one in every 4 pixels, or better yet, average the pixels in each 2x2 block.
		//           Hint: Be careful when averaging! You probably want to convert the pixel values to floating-point to
		//           do the averaging maths.s

		//width /= ResolutionMode;
		//height /= ResolutionMode;

		for (int y = 0; y < height; ++y)
			for (int x = 0; x < width; ++x)
				for (int c = 0; c < 3; ++c)
				{
					int pixelIdx = x + y * width;
				}

		break;

	}
}

int main()
{

	// Set up an image buffer
	std::vector<uint8_t> imageBuffer(height * width * nChannels);





    // **** Begin your lovely rasteriser code ****





    // IMAGE BUFFER
    SetFlatBackground(imageBuffer, 0, 255, 255, 255);

	//SetImageBackground(imageBuffer, 1, "../images/stanford_bunny.png"); //Currently broken

    SetSinglePixel(imageBuffer, 12, 12, 255, 0, 255, 255);





    // **** End lovely rasteriser code ****

    // Save the image
    int errorCode;
        errorCode = lodepng::encode(outputFilename, imageBuffer, width, height);
        if (errorCode) { // check the error code, in case an error occurred.
            std::cout << "lodepng error encoding image: " << lodepng_error_text(errorCode) << std::endl;
            return errorCode;
        }

    return 0;
}


