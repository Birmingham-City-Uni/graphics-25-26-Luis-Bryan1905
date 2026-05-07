// This define is necessary to get the M_PI constant.
#define _USE_MATH_DEFINES
#include <math.h>

#include <iostream>
#include <lodepng.h>
#include "Image.hpp"
#include "LinAlg.hpp"
#include "Light.hpp"
#include "Mesh.hpp"
#include "Shading.hpp"
#include <string.h>


// ***** WEEK 6 LAB *****
// Subtask 1: Implement the projectionMatrix function, to make a projection matrix to view your scene!
// Subtask 2: Complete the transformation chain, moving vertices from model space all the way to screen space.
// Subtask 3: Set up the camera and projection matrices for the transformation chain
// Subtask 4: Implement Z buffering.
// Subtask 5: Implement texture mapping.
// If you finish early - note that we now have all the tools to properly set up your own scene!
// This is a great time to start on your own code in the coursework/rasteriser folder, using this as a base if
// you wish. We will in future labs work on more advanced shading, but you can port this feature over later.

enum ShadingMode {
	PHONG, // phong specular gives a highlight that is largest when the reflected ray points directly at the camera.
	BLINN_PHONG, // blinn-phong specular gives a highlight that is largest when the half vector between the light and view ray points directly at the normal.
	EMISSIVE // emissive just gives the albedo color, without any lighting. This is useful for debugging, and also for drawing things like the sun, which are light sources but also visible objects.
};

struct Triangle {
	std::array<Eigen::Vector3f, 3> screen; // Coordinates of the triangle in screen space.
	std::array<Eigen::Vector3f, 3> verts; // Vertices of the triangle in world space.
	std::array<Eigen::Vector3f, 3> cam; // Vertices of the triangle in camera space.
	std::array<Eigen::Vector3f, 3> norms; // Normals of the triangle corners in world space.
	std::array<Eigen::Vector2f, 3> texs; // Texture coordinates of the triangle corners.
};


Eigen::Matrix4f projectionMatrix(int height, int width, float horzFov = 55.1f*M_PI/180.f, float zFar = 10000.f, float zNear = 0.1f)
{

	// Make a projection matrix following the formulation in the lecture slides, and using the provided parameters.
	// First, work out vertical FoV based on the horizontal FoV:

	float vertFov = horzFov * float(height) / width;

	// Now construct the matrix.
	Eigen::Matrix4f projection = Eigen::Matrix4f::Zero();

	projection(0, 0) = 1.f / tanf(0.5f * horzFov);
	projection(1, 1) = 1.f / tanf(0.5f * vertFov);
	projection(2, 2) = zFar / (zFar - zNear);
	projection(2, 3) = -zFar * zNear / (zFar - zNear);
	projection(3, 2) = 1.f;
	 
	return projection;
	// *** END YOUR CODE ***
}

void findScreenBoundingBox(const Triangle& t, int width, int height, int& minX, int& minY, int& maxX, int& maxY)
{
	// Find a bounding box around the triangle
	minX = std::min(std::min(t.screen[0].x(), t.screen[1].x()), t.screen[2].x());
	minY = std::min(std::min(t.screen[0].y(), t.screen[1].y()), t.screen[2].y());
	maxX = std::max(std::max(t.screen[0].x(), t.screen[1].x()), t.screen[2].x());
	maxY = std::max(std::max(t.screen[0].y(), t.screen[1].y()), t.screen[2].y());

	// Constrain it to lie within the image.
	minX = std::min(std::max(minX, 0), width-1);
	maxX = std::min(std::max(maxX, 0), width-1);
	minY = std::min(std::max(minY, 0), height-1);
	maxY = std::min(std::max(maxY, 0), height-1);
}


void drawTriangle(std::vector<uint8_t>& image, int width, int height,
	std::vector<float>& zBuffer,
	const Triangle& t,
	const std::vector<std::unique_ptr<Light>>& lights,
	const std::vector<uint8_t>& albedo, const Eigen::Vector3f& specularColor,
	float specularExponent,
	ShadingMode shadingMode,
	const Eigen::Vector3f& camWorldPos, int texWidth, int texHeight)
{
	int minX, minY, maxX, maxY;
	findScreenBoundingBox(t, width, height, minX, minY, maxX, maxY);

	Eigen::Vector2f edge1 = v2(t.screen[2] - t.screen[0]);
	Eigen::Vector2f edge2 = v2(t.screen[1] - t.screen[0]);
	float triangleArea = 0.5f * vec2Cross(edge2, edge1);
	if (triangleArea < 0) {
		// Triangle is backfacing
		// Exit and quit drawing!
		return;
	}

	for(int x = minX; x <= maxX; ++x) 
		for (int y = minY; y <= maxY; ++y) {
			Eigen::Vector2f p(x, y);

			// Find sub-triangle areas
			float a0 = 0.5f * fabsf(vec2Cross(v2(t.screen[1]) - v2(t.screen[2]), p - v2(t.screen[2])));
			float a1 = 0.5f * fabsf(vec2Cross(v2(t.screen[0]) - v2(t.screen[2]), p - v2(t.screen[2])));
			float a2 = 0.5f * fabsf(vec2Cross(v2(t.screen[0]) - v2(t.screen[1]), p - v2(t.screen[1])));

			// find barycentrics
			float b0 = a0 / triangleArea;
			float b1 = a1 / triangleArea;
			float b2 = a2 / triangleArea;

			// If outside triangle, exit early
			float sum = b0 + b1 + b2;
			if (sum > 1.0001) {
				continue;
			}
			
			// This interpolation is currently just basic barycentric interpolation, which will cause issues!
			// Replace with perspective-correct, following the steps below

			// Get the depths from the camera-space position of the 3 corners.
			float depth0 = t.cam[0].z(), depth1 = t.cam[1].z(), depth2 = t.cam[2].z();

			// Work out the depth at the point P
			float depthP = 1 / (b0 / depth0 + b1 / depth1 + b2 / depth2);

			// Interpolate to find the world-space position of this pixel (correct this version to be 
			// perspective-correct).
			// Don't forget to multiply by depthP!
			Eigen::Vector3f worldP = (t.verts[0] * b0 / depth0 + t.verts[1] * b1 / depth1 + t.verts[2] * b2 / depth2) * depthP;

			// Interpolate to find the normal of this pixel (correct this version to be 
			// perspective-correct).
			// Tip: you don't need to worry about multiplying by depthP - you'll normalise this anyway!
			Eigen::Vector3f normP = (t.norms[0] * b0 / depth0 + t.norms[1] * b1 / depth1 + t.norms[2] * b2 / depth2);
			normP.normalize();

			// Interpolate to find the correct clip-space depth (correct this version to be perspective-correct)
			// This won't make too much of a difference in this case, but technically this version does use slightly
			// incorrect depths.
			float depth = b0 * t.screen[0].z() + b1 * t.screen[1].z() + b2 * t.screen[2].z();
			// *** END YOUR CODE ***

			int depthIdx = static_cast<int>(p.x()) + static_cast<int>(p.y()) * width;
			if (depth > zBuffer[depthIdx]) continue;
			zBuffer[depthIdx] = depth;

			// Work out colour at this position.
			Eigen::Vector2f texP = Eigen::Vector2f::Zero();
			texP = (t.texs[0] * b0 / depth0 + t.texs[1] * b1 / depth1 + t.texs[2] * b2 / depth2) * depthP;

			texP.x() = texP.x() - floor(texP.x()); //wrap UVs (this is the important bit)
			texP.y() = texP.y() - floor(texP.y());
			// Convert this coordinate to a point in texture space
			// To do so, multiply by the texWidth and texHeight to get to the correct range.
			// Don't forget to flip the y coordinates!
			// 
			int texR = (1 - texP.y()) * texHeight;
			int texC = texP.x() * texWidth;

			// Handle the case where texR or texC end up outside the image!
			// There are different ways you could do this - for example using 
			// the modulo (%) operator to wrap around, or clamping to the edges.
			// Write your own code below to do this - once you're done you should be sure 
			// that 0 <= texC < texWidth and 0 <= texR < texHeight.

			texR = std::min(std::max(texR, 0), texHeight - 1);
			texC = std::min(std::max(texC, 0), texWidth - 1);

			// Get the value from the texture (hint: use the getPixel function on the albedoTexture).
			Color texColor = getPixel(albedo, texC, texR, texWidth, texHeight);

			// Convert it into an Eigen::Vector3f as an albedo
			// (Optional bonus task, if you checked out the slides on gamma correction:
			// gamma correct this colour, so the texture doesn't appear overly bright.
			// should you raise to the power 1/2.2, or 2.2?)
			Eigen::Vector3f albedo;
			albedo.x() = powf(texColor.r / 255.f, 2.2f);
			albedo.y() = powf(texColor.g / 255.f, 2.2f);
			albedo.z() = powf(texColor.b / 255.f, 2.2f);

			// *** END YOUR CODE ***

			Eigen::Vector3f viewDir = (camWorldPos - worldP).normalized();

			Eigen::Vector3f color = Eigen::Vector3f::Zero();

			if (shadingMode != ShadingMode::EMISSIVE)
			{ 
			
				// Iterate over lights, and sum to find colour.
				for (auto& light : lights)
				{

					// Work out the contribution from this light source, and add it to the color variable.

					// Work out the intensity of this light source, at the point worldP.
					Eigen::Vector3f lightIntensity = light->getIntensityAt(worldP);

					// We only need to do the following if the light isn't an ambient light.
					if (light->getType() != Light::Type::AMBIENT) {
						Eigen::Vector3f incomingLightDir = light->getDirection(worldP).normalized();;

						// Take the dot product of the normal with the light direction.
						float dotProd = normP.dot(-incomingLightDir);

						// We don't want negative light - if dot product less than 0, set it to 0.
						dotProd = std::max(dotProd, 0.0f);

						if (dotProd > 0.0f)
						{
							float specularTerm;
							if (shadingMode == ShadingMode::PHONG) {
								specularTerm = phongSpecularTerm(incomingLightDir, normP, viewDir, specularExponent);
							}
							else {
								specularTerm = blinnPhongSpecularTerm(incomingLightDir, normP, viewDir, specularExponent);
							}

							Eigen::Vector3f specularOut = specularColor * specularTerm;
							specularOut = coeffWiseMultiply(specularOut, lightIntensity);

							// Multiply the light intensity by the dot product.
							Eigen::Vector3f diffuseOut = lightIntensity * dotProd;
							diffuseOut = coeffWiseMultiply(diffuseOut, albedo);

							color += specularOut;
							color += diffuseOut;
							//color = (incomingLightDir + Eigen::Vector3f::Ones()) / 2;
						}


					}
					else
					{
						// Light is ambient - just multiply light intensity with albedo.
						color += coeffWiseMultiply(lightIntensity, albedo);
					}
				}

			}
			else
			{
				// Emissive - just use the albedo colour as the output colour.
				color = albedo;
			}


			//color = (worldP + Eigen::Vector3f::Ones()) / 2;
			//color = (viewDir + Eigen::Vector3f::Ones()) / 2;
			//color = (normP + Eigen::Vector3f::Ones()) / 2;

			//color = (normP + Eigen::Vector3f::Ones()) * 0.5f; // Temporary - set colour to be based on normal, so we can check that interpolation is working correctly.

			Color c;
			// Gamma-correcting colours.
			c.r = std::min(powf(color.x(), 1 / 2.2f), 1.0f) * 255;
			c.g = std::min(powf(color.y(), 1 / 2.2f), 1.0f) * 255;
			c.b = std::min(powf(color.z(), 1 / 2.2f), 1.0f) * 255;

			c.a = 255;

			

			Color oldColor = getPixel(image, x, y, width, height);

			float alpha = texColor.a / 255.f;

			c.r = c.r * alpha + (1 - alpha) * oldColor.r;
			c.g = c.g * alpha + (1 - alpha) * oldColor.g;
			c.b = c.b * alpha + (1 - alpha) * oldColor.b;
			setPixel(image, x, y, width, height, c);
		}
}



void drawMesh(std::vector<unsigned char>& image,
	std::vector<float>& zBuffer,
	const Mesh& mesh,
	const std::vector<uint8_t>& albedo, 
	int texWidth, int texHeight,
	const Eigen::Vector3f& specularColor,
	float specularExponent,
	ShadingMode shadingMode,
	const Eigen::Vector3f& camWorldPos,
	const Eigen::Matrix4f& modelToWorld,
	const Eigen::Matrix4f& worldToCam,
	const Eigen::Matrix4f& camToClip,
	const std::vector<std::unique_ptr<Light>>& lights,
	int width, int height)
{
	std::cout << "Drawing with " << mesh.vFaces.size() << " triangles." << std::endl;

	for (int i = 0; i < mesh.vFaces.size(); ++i) {
		Eigen::Vector3f
			v0 = mesh.verts[mesh.vFaces[i][0]],
			v1 = mesh.verts[mesh.vFaces[i][1]],
			v2 = mesh.verts[mesh.vFaces[i][2]];
		Eigen::Vector3f
			n0 = mesh.norms[mesh.nFaces[i][0]],
			n1 = mesh.norms[mesh.nFaces[i][1]],
			n2 = mesh.norms[mesh.nFaces[i][2]];

		Triangle t;
		t.verts[0] = (modelToWorld * vec3ToVec4(v0)).block<3, 1>(0, 0);
		t.verts[1] = (modelToWorld * vec3ToVec4(v1)).block<3, 1>(0, 0);
		t.verts[2] = (modelToWorld * vec3ToVec4(v2)).block<3, 1>(0, 0);

		t.cam[0] = (worldToCam * modelToWorld * vec3ToVec4(v0)).block<3, 1>(0, 0);
		t.cam[1] = (worldToCam * modelToWorld * vec3ToVec4(v1)).block<3, 1>(0, 0);
		t.cam[2] = (worldToCam * modelToWorld * vec3ToVec4(v2)).block<3, 1>(0, 0);

		// Work out the clip space coordinates, by multiplying by worldToClip and doing the 
		// perspective divide.
		Eigen::Vector4f vClip0 = camToClip * worldToCam * modelToWorld * vec3ToVec4(v0);
		vClip0 /= vClip0.w();
		Eigen::Vector4f vClip1 = camToClip * worldToCam * modelToWorld * vec3ToVec4(v1);
		vClip1 /= vClip1.w();
		Eigen::Vector4f vClip2 = camToClip * worldToCam * modelToWorld * vec3ToVec4(v2);
		vClip2 /= vClip2.w();

		// Check that all 3 vertices are in the clip box (-1 to 1 in x, y and z) and if not,
		// skip drawing this triangle.
		if (outsideClipBox(vClip0) && outsideClipBox(vClip1) && outsideClipBox(vClip2)) continue;

		// Work out the screen space coordinates based on the image height and width.
		float x0 = -vClip0.x();
		float x1 = -vClip1.x();
		float x2 = -vClip2.x();

		t.screen[0] = Eigen::Vector3f((x0 + 1.0f) * width / 2, (-vClip0.y() + 1.0f) * height / 2, vClip0.z());
		t.screen[1] = Eigen::Vector3f((x1 + 1.0f) * width / 2, (-vClip1.y() + 1.0f) * height / 2, vClip1.z());
		t.screen[2] = Eigen::Vector3f((x2 + 1.0f) * width / 2, (-vClip2.y() + 1.0f) * height / 2, vClip2.z());

		// transform the normals (using the inverse transpose of the upper 3x3 block)
		t.norms[0] = (modelToWorld.block<3, 3>(0, 0).inverse().transpose() * n0).normalized();
		t.norms[1] = (modelToWorld.block<3, 3>(0, 0).inverse().transpose() * n1).normalized();
		t.norms[2] = (modelToWorld.block<3, 3>(0, 0).inverse().transpose() * n2).normalized();

		t.texs[0] = mesh.texs[mesh.tFaces[i][0]];
		t.texs[1] = mesh.texs[mesh.tFaces[i][1]];
		t.texs[2] = mesh.texs[mesh.tFaces[i][2]];

		std::cout << "Drawing triangle " << i << " / " << mesh.vFaces.size() << "\r" << std::flush;

		drawTriangle(image, width, height, zBuffer, t, lights, albedo, specularColor, specularExponent, shadingMode, camWorldPos, texWidth, texHeight);

	}
}

int drawScene(const std::string& outputFilename, ShadingMode mode, float specularExponent)
{
	//std::string outputFilename = "output.png";

	const int width = 1920, height = 1080;
	const int nChannels = 4;

	// Setting up an image buffer
	// This std::vector has one 8-bit value for each pixel in each row and column of the image, and
	// for each of the 4 channels (red, green, blue and alpha).
	// Remember 8-bit unsigned values can range from 0 to 255.
	std::vector<uint8_t> imageBuffer(height*width*nChannels);
	std::vector<float> zBuffer(height * width);

	// This line sets the image to black initially.
	Color black{ 0,0,0,255 };
	for (int r = 0; r < height; ++r) {
		for (int c = 0; c < width; ++c) {
			setPixel(imageBuffer, c, r, width, height, black);
			zBuffer[r * width + c] = 1.0f;
		}
	}

	Eigen::Matrix4f projection = projectionMatrix(height, width);

	// This matrix rotates the camera, tilting it down, then translates it up to make it look down on the scene.
	Eigen::Matrix4f cameraToWorld = translationMatrix(Eigen::Vector3f(-2.13726, 2.38136, -3.87966)) * rotateYMatrix(0.58f) * rotateXMatrix(0.36f);

	Eigen::Vector3f camWorldPos = (cameraToWorld * Eigen::Vector4f(0, 0, 0, 1)).block<3, 1>(0, 0);

	// The main important task = set up the worldToCamera and worldToClip matrices here!
	// Set up worldToCamera, based on cameraToWorld above
	Eigen::Matrix4f worldToCamera = cameraToWorld.inverse();
	// Set up worldToClip, using the projection and worldToCamera matrices
	Eigen::Matrix4f worldToClip = projection * worldToCamera;

	////////////////////////////////////////////////	LIGHTS		////////////////////////////////////////////////////////////

	std::vector<std::unique_ptr<Light>> lights;
	// I've already added an ambient light for you!
	lights.emplace_back(new AmbientLight(Eigen::Vector3f(0.1f, 0.1f, 0.1f)));

	//RADIUS = 0, BLENDER X Y Z = -x, -y, z
	lights.emplace_back(new PointLight(Eigen::Vector3f(0.491f, 0.423f, 0.141) * 30, Eigen::Vector3f(-3.407f, 6.018f, -2.217f))); // intensity, postion || rt_w9a02_s01_ceilingup__0013
	lights.emplace_back(new PointLight(Eigen::Vector3f(0.491f, 0.423f, 0.141) * 30, Eigen::Vector3f(3.0f, -6.018f, -2.217f))); // intensity, postion || rt_w9a02_s01_ceilingup__0014
	lights.emplace_back(new PointLight(Eigen::Vector3f(0.491f, 0.423f, 0.141) * 30, Eigen::Vector3f(3.0f, 6.018f, 2.217f))); // intensity, postion || rt_w9a02_s01_ceilingup__0015
	lights.emplace_back(new PointLight(Eigen::Vector3f(0.491f, 0.423f, 0.141) * 30, Eigen::Vector3f(-3.407f, -6.018f, 2.217f))); // intensity, postion || rt_w9a02_s01_ceilingup__0016

	lights.emplace_back(new PointLight(Eigen::Vector3f(0.0f, 0.491f, 0.491) * 18, Eigen::Vector3f(3.918f, -2.936f, 0.256f))); // intensity, postion || rt_w9a02_s01_monitor__0023
	
	lights.emplace_back(new PointLight(Eigen::Vector3f(0.491f, 0.328f, 0.141f) * 12, Eigen::Vector3f(0.979f,- 3.43f, 4.378f))); // intensity, postion || rt_w9a02_s01_wall__0019
	lights.emplace_back(new PointLight(Eigen::Vector3f(0.491f, 0.328f, 0.141f) * 12, Eigen::Vector3f(-5.703f, -3.43f, 4.378f))); // intensity, postion || rt_w9a02_s01_wall__0020

	
	Eigen::Matrix4f MeshTransform; 
	MeshTransform = translationMatrix(Eigen::Vector3f(-0.0f, 0.0f, 0.f)) * rotateYMatrix(M_PI);

	////////////////////////////////////////////////	w9a02		////////////////////////////////////////////////////////////

	Mesh w9a02_brick02 = loadMeshFile("../models/w9a02_brick02.obj"); //BROKEN RENDERERING???
	std::vector<uint8_t> w3_brick001_abd;
	unsigned int w3_brick001_abd_TexWidth, w3_brick001_abd_TexHeight;
	lodepng::decode(w3_brick001_abd, w3_brick001_abd_TexWidth, w3_brick001_abd_TexHeight, "../textures/w3_brick001_abd.png");
	//drawMesh(imageBuffer, zBuffer, w9a02_concrete03, w3_concrete003_abd, concrete003_TexWidth, concrete003_TexHeight, MeshTransform, worldToClip, lights, width, height);
	drawMesh(imageBuffer, zBuffer, w9a02_brick02, w3_brick001_abd, w3_brick001_abd_TexWidth, w3_brick001_abd_TexHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, MeshTransform, worldToCamera, projection, lights, width, height);
	std::cout << "Mesh w9a02_brick02 Drawn" << std::endl;


	Mesh w9a02_concrete03 = loadMeshFile("../models/w9a02_concrete03.obj"); //BROKEN RENDERERING???
	std::vector<uint8_t> w3_concrete003_abd;
	unsigned int concrete003_TexWidth, concrete003_TexHeight;
	lodepng::decode(w3_concrete003_abd, concrete003_TexWidth, concrete003_TexHeight, "../textures/w3_concrete003_abd.png");
	//drawMesh(imageBuffer, zBuffer, w9a02_concrete03, w3_concrete003_abd, concrete003_TexWidth, concrete003_TexHeight, MeshTransform, worldToClip, lights, width, height);
	drawMesh(imageBuffer, zBuffer, w9a02_concrete03, w3_concrete003_abd, concrete003_TexWidth, concrete003_TexHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, MeshTransform, worldToCamera, projection, lights, width, height);
	std::cout << "Mesh w9a02_concrete03 Drawn" << std::endl;

	Mesh w9a02_debris02 = loadMeshFile("../models/w9a02_debris02.obj");
	std::vector<uint8_t> w3_debris002_abd_a;
	unsigned int w3_debris002_abd_a_TexWidth, w3_debris002_abd_a_TexHeight;
	lodepng::decode(w3_debris002_abd_a, w3_debris002_abd_a_TexWidth, w3_debris002_abd_a_TexHeight, "../textures/w3_debris002_abd_a.png");
	//drawMesh(imageBuffer, zBuffer, w9a02_debris02, w3_debris002_abd_a, w3_debris002_abd_a_TexWidth, w3_debris002_abd_a_TexHeight, MeshTransform, worldToClip, lights, width, height);
	drawMesh(imageBuffer, zBuffer, w9a02_debris02, w3_debris002_abd_a, w3_debris002_abd_a_TexWidth, w3_debris002_abd_a_TexHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, MeshTransform, worldToCamera, projection, lights, width, height);
	std::cout << "Mesh w9a02_debris02 Drawn" << std::endl;

	Mesh w9a02_debris03 = loadMeshFile("../models/w9a02_debris03.obj");
	std::vector<uint8_t> w3_wall001_abd;
	unsigned int w3_wall001_abd_TexWidth, w3_wall001_abd_TexHeight;
	lodepng::decode(w3_wall001_abd, w3_wall001_abd_TexWidth, w3_wall001_abd_TexHeight, "../textures/w3_wall001_abd.png");
	//drawMesh(imageBuffer, zBuffer, w9a02_debris02, w3_debris002_abd_a, w3_debris002_abd_a_TexWidth, w3_debris002_abd_a_TexHeight, MeshTransform, worldToClip, lights, width, height);
	drawMesh(imageBuffer, zBuffer, w9a02_debris03, w3_wall001_abd, w3_wall001_abd_TexWidth, w3_wall001_abd_TexHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, MeshTransform, worldToCamera, projection, lights, width, height);
	std::cout << "Mesh w9a02_debris03 Drawn" << std::endl;

	Mesh w9a02_fy1_wood04 = loadMeshFile("../models/w9a02_fy1_wood04.obj");
	std::vector<uint8_t> w3_wood304_abd;
	unsigned int w3_wood304_abd_TexWidth, w3_wood304_abd_TexHeight;
	lodepng::decode(w3_wood304_abd, w3_wood304_abd_TexWidth, w3_wood304_abd_TexHeight, "../textures/w3_wood304_abd.png");
	//drawMesh(imageBuffer, zBuffer, w9a02_debris02, w3_debris002_abd_a, w3_debris002_abd_a_TexWidth, w3_debris002_abd_a_TexHeight, MeshTransform, worldToClip, lights, width, height);
	drawMesh(imageBuffer, zBuffer, w9a02_fy1_wood04, w3_wood304_abd, w3_wood304_abd_TexWidth, w3_wood304_abd_TexHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, MeshTransform, worldToCamera, projection, lights, width, height);
	std::cout << "Mesh w9a02_fy1_wood04 Drawn" << std::endl;

	Mesh w9a02_drum01 = loadMeshFile("../models/w9a02_drum01.obj"); //CAUSES CRASH??
	std::vector<uint8_t> w3_mat_ym2_drum01;
	unsigned int w3_metal405_abd_TexWidth, w3_metal405_abd_TexHeight;
	lodepng::decode(w3_mat_ym2_drum01, w3_metal405_abd_TexWidth, w3_metal405_abd_TexWidth, "../textures/w3_metal405_abd.png");
	//drawMesh(imageBuffer, zBuffer, w9a02_drum01, w3_mat_ym2_drum01, w3_metal405_abd_TexWidth, w3_metal405_abd_TexWidth, MeshTransform, worldToClip, lights, width, height);
	drawMesh(imageBuffer, zBuffer, w9a02_drum01, w3_mat_ym2_drum01, w3_metal405_abd_TexWidth, w3_metal405_abd_TexWidth, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, MeshTransform, worldToCamera, projection, lights, width, height);
	std::cout << "Mesh w9a02_drum01 Drawn" << std::endl;

	Mesh w9a02_iron02 = loadMeshFile("../models/w9a02_iron02.obj");
	std::vector<uint8_t> w3_iron402_abd;
	unsigned int w3_iron402_abd_TexWidth, w3_iron402_abd_TexHeight;
	lodepng::decode(w3_iron402_abd, w3_iron402_abd_TexWidth, w3_iron402_abd_TexHeight, "../textures/w3_iron402_abd.png");
	//drawMesh(imageBuffer, zBuffer, w9a02_iron02, w3_iron402_abd, w3_iron402_abd_TexWidth, w3_iron402_abd_TexHeight, MeshTransform, worldToClip, lights, width, height);
	drawMesh(imageBuffer, zBuffer, w9a02_iron02, w3_iron402_abd, w3_iron402_abd_TexWidth, w3_iron402_abd_TexHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, MeshTransform, worldToCamera, projection, lights, width, height);
	std::cout << "Mesh w9a02_iron02 Drawn" << std::endl;

	Mesh w9a02_km1_brick01 = loadMeshFile("../models/w9a02_km1_brick01.obj");
	std::vector<uint8_t> w3_brick201_abd;
	unsigned int w3_brick201_TexWidth, w3_brick201_TexHeight;
	lodepng::decode(w3_brick201_abd, w3_brick201_TexWidth, w3_brick201_TexHeight, "../textures/w3_brick201_abd.png");
	//drawMesh(imageBuffer, zBuffer, w9a02_km1_brick01, w3_brick201_abd, w3_brick201_TexWidth, w3_brick201_TexHeight, MeshTransform, worldToClip, lights, width, height);
	drawMesh(imageBuffer, zBuffer, w9a02_km1_brick01, w3_brick201_abd, w3_brick201_TexWidth, w3_brick201_TexHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, MeshTransform, worldToCamera, projection, lights, width, height);
	std::cout << "Mesh w9a02_km1_brick01 Drawn" << std::endl;

	Mesh w9a02_km1_concrete01 = loadMeshFile("../models/w9a02_km1_concrete01.obj");
	std::vector<uint8_t> w3_concrete201_abd;
	unsigned int concrete201_TexWidth, concrete201_TexHeight;
	lodepng::decode(w3_concrete201_abd, concrete201_TexWidth, concrete201_TexHeight, "../textures/w3_concrete201_abd.png");
	//drawMesh(imageBuffer, zBuffer, w9a02_km1_concrete01, w3_concrete201_abd, concrete201_TexWidth, concrete201_TexHeight, MeshTransform, worldToClip, lights, width, height);
	drawMesh(imageBuffer, zBuffer, w9a02_km1_concrete01, w3_concrete201_abd, concrete201_TexWidth, concrete201_TexHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, MeshTransform, worldToCamera, projection, lights, width, height);
	std::cout << "Mesh w9a02_km1_concrete01 Drawn" << std::endl;

	Mesh w9a02_km1_wall01 = loadMeshFile("../models/w9a02_km1_wall01.obj");
	std::vector<uint8_t> w3_wall201_abd;
	unsigned int w3_wall201_abd_TexWidth, w3_wall201_abd_TexHeight;
	lodepng::decode(w3_wall201_abd, w3_wall201_abd_TexWidth, w3_wall201_abd_TexHeight, "../textures/w3_wall201_abd.png");
	//drawMesh(imageBuffer, zBuffer, w9a02_metal02, w3_metal102_abd, w3_metal102_abd_TexWidth, w3_metal102_abd_TexHeight, MeshTransform, worldToClip, lights, width, height);
	//drawMesh(imageBuffer, zBuffer, w9a02_km1_wall01, w3_wall201_abd, w3_wall201_abd_TexWidth, w3_wall201_abd_TexHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, MeshTransform, worldToCamera, projection, lights, width, height);
	std::cout << "Mesh w9a02_km1_wall01 Drawn" << std::endl;

	Mesh w9a02_metal02 = loadMeshFile("../models/w9a02_metal02.obj");
	std::vector<uint8_t> w3_metal102_abd;
	unsigned int w3_metal102_abd_TexWidth, w3_metal102_abd_TexHeight;
	lodepng::decode(w3_metal102_abd, w3_metal102_abd_TexWidth, w3_metal102_abd_TexHeight, "../textures/w3_metal102_abd.png");
	//drawMesh(imageBuffer, zBuffer, w9a02_metal02, w3_metal102_abd, w3_metal102_abd_TexWidth, w3_metal102_abd_TexHeight, MeshTransform, worldToClip, lights, width, height);
	drawMesh(imageBuffer, zBuffer, w9a02_metal02, w3_metal102_abd, w3_metal102_abd_TexWidth, w3_metal102_abd_TexHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, MeshTransform, worldToCamera, projection, lights, width, height);
	std::cout << "Mesh w9a02_metal02 Drawn" << std::endl;

	Mesh w9a02_metal05 = loadMeshFile("../models/w9a02_metal05.obj");
	std::vector<uint8_t> w3_metal105_abd;
	unsigned int w3_metal104_abd_TexWidth, w3_metal104_abd_TexHeight;
	lodepng::decode(w3_metal105_abd, w3_metal104_abd_TexWidth, w3_metal104_abd_TexHeight, "../textures/w3_metal104_abd.png");
	//drawMesh(imageBuffer, zBuffer, w9a02_metal05, w3_metal105_abd, w3_metal104_abd_TexWidth, w3_metal104_abd_TexHeight, MeshTransform, worldToClip, lights, width, height);
	drawMesh(imageBuffer, zBuffer, w9a02_metal05, w3_metal105_abd, w3_metal104_abd_TexWidth, w3_metal104_abd_TexHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, MeshTransform, worldToCamera, projection, lights, width, height);
	std::cout << "Mesh w9a02_metal05 Drawn" << std::endl;

	Mesh w9a02_metal101_kd1 = loadMeshFile("../models/w9a02_metal101_kd1.obj");
	std::vector<uint8_t> m9_metal101_kd1;
	unsigned int w9_metal101_abd_TexWidth, w9_metal101_abd_abd_TexHeight;
	lodepng::decode(m9_metal101_kd1, w9_metal101_abd_TexWidth, w9_metal101_abd_abd_TexHeight, "../textures/w9_metal101_abd.png");
	//drawMesh(imageBuffer, zBuffer, w9a02_metal101_kd1, m9_metal101_kd1, w9_metal101_abd_TexWidth, w9_metal101_abd_abd_TexHeight, MeshTransform, worldToClip, lights, width, height);
	drawMesh(imageBuffer, zBuffer, w9a02_metal101_kd1, m9_metal101_kd1, w9_metal101_abd_TexWidth, w9_metal101_abd_abd_TexHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, MeshTransform, worldToCamera, projection, lights, width, height);
	std::cout << "Mesh w9a02_metal101_kd1 Drawn" << std::endl;

	Mesh w9a02_metal151 = loadMeshFile("../models/w9a02_metal151.obj"); //TEXTURE CAUSES CRASH??
	std::vector<uint8_t> w9_metal151_abd;
	unsigned int w9_metal151_abd_TexWidth, w9_metal151_abd_TexHeight;
	lodepng::decode(w9_metal151_abd, w9_metal151_abd_TexWidth, w9_metal151_abd_TexHeight, "../textures/w9_metal151_abd.png");
	//drawMesh(imageBuffer, zBuffer, w9a02_metal151, w9_metal151_abd, w9_metal151_abd_TexWidth, w9_metal151_abd_TexHeight, MeshTransform, worldToClip, lights, width, height);
	drawMesh(imageBuffer, zBuffer, w9a02_metal151, w9_metal151_abd, w9_metal151_abd_TexWidth, w9_metal151_abd_TexHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, MeshTransform, worldToCamera, projection, lights, width, height);
	std::cout << "Mesh w9a02_metal151 Drawn" << std::endl;

	Mesh w9a02_metal159 = loadMeshFile("../models/w9a02_metal159.obj");
	std::vector<uint8_t> w9_metal159_abd;
	unsigned int w9_metal159_abd_TexWidth, w9_metal159_abd_TexHeight;
	lodepng::decode(w9_metal159_abd, w9_metal159_abd_TexWidth, w9_metal159_abd_TexHeight, "../textures/w9_metal159_abd.png");
	//drawMesh(imageBuffer, zBuffer, w9a02_metal159, w9_metal159_abd, w9_metal159_abd_TexWidth, w9_metal159_abd_TexHeight, MeshTransform, worldToClip, lights, width, height);
	drawMesh(imageBuffer, zBuffer, w9a02_metal159, w9_metal159_abd, w9_metal159_abd_TexWidth, w9_metal159_abd_TexHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, MeshTransform, worldToCamera, projection, lights, width, height);
	std::cout << "Mesh w9a02_metal159 Drawn" << std::endl;

	Mesh w9a02_metal154 = loadMeshFile("../models/w9a02_metal154.obj"); //TEXTURE CAUSES CRASH??
	std::vector<uint8_t> w9_metal154_abd;
	unsigned int w9_metal154_abd_TexWidth, w9_metal154_abd_TexHeight;
	lodepng::decode(w9_metal154_abd, w9_metal154_abd_TexWidth, w9_metal154_abd_TexHeight, "../textures/w9_metal154_abd.png");
	//drawMesh(imageBuffer, zBuffer, w9a02_metal154, w9_metal154_abd, w9_metal154_abd_TexWidth, w9_metal154_abd_TexHeight, MeshTransform, worldToClip, lights, width, height);
	drawMesh(imageBuffer, zBuffer, w9a02_metal154, w9_metal154_abd, w9_metal154_abd_TexWidth, w9_metal154_abd_TexHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, MeshTransform, worldToCamera, projection, lights, width, height);
	std::cout << "Mesh w9a02_metal154 Drawn" << std::endl;

	Mesh w9a02_metal709 = loadMeshFile("../models/w9a02_metal709.obj");
	std::vector<uint8_t> w9_metal709_abd;
	unsigned int w9_metal709_abd_TexWidth, w9_metal709_abd_TexHeight;
	lodepng::decode(w9_metal709_abd, w9_metal709_abd_TexWidth, w9_metal709_abd_TexHeight, "../textures/w9_metal159_abd.png");
	//drawMesh(imageBuffer, zBuffer, w9a02_metal709, w9_metal709_abd, w9_metal709_abd_TexWidth, w9_metal709_abd_TexHeight, MeshTransform, worldToClip, lights, width, height);
	drawMesh(imageBuffer, zBuffer, w9a02_metal709, w9_metal709_abd, w9_metal709_abd_TexWidth, w9_metal709_abd_TexHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, MeshTransform, worldToCamera, projection, lights, width, height);
	std::cout << "Mesh w9a02_metal709 Drawn" << std::endl;

	Mesh w9a02_metal854 = loadMeshFile("../models/w9a02_metal854.obj");
	std::vector<uint8_t> w9_metal854_abd;
	unsigned int w9_metal854_abd_TexWidth, w9_metal854_abd_TexHeight;
	lodepng::decode(w9_metal854_abd, w9_metal854_abd_TexWidth, w9_metal854_abd_TexHeight, "../textures/w9_metal854_abd.png");
	//drawMesh(imageBuffer, zBuffer, w9a02_metal909, w9_metal909_abd, w9_metal909_abd_TexWidth, w9_metal909_abd_TexHeight, MeshTransform, worldToClip, lights, width, height);
	drawMesh(imageBuffer, zBuffer, w9a02_metal854, w9_metal854_abd, w9_metal854_abd_TexWidth, w9_metal854_abd_TexHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, MeshTransform, worldToCamera, projection, lights, width, height);
	std::cout << "Mesh w9a02_metal854 Drawn" << std::endl;

	Mesh w9a02_metal901 = loadMeshFile("../models/w9a02_metal901.obj");
	std::vector<uint8_t> w9_metal901_abd;
	unsigned int w9_metal901_abd_TexWidth, w9_metal901_abd_TexHeight;
	lodepng::decode(w9_metal901_abd, w9_metal901_abd_TexWidth, w9_metal901_abd_TexHeight, "../textures/w9_metal901_abd.png");
	//drawMesh(imageBuffer, zBuffer, w9a02_metal909, w9_metal909_abd, w9_metal909_abd_TexWidth, w9_metal909_abd_TexHeight, MeshTransform, worldToClip, lights, width, height);
	drawMesh(imageBuffer, zBuffer, w9a02_metal901, w9_metal901_abd, w9_metal901_abd_TexWidth, w9_metal901_abd_TexHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, MeshTransform, worldToCamera, projection, lights, width, height);
	std::cout << "Mesh w9a02_metal901 Drawn" << std::endl;

	Mesh w9a02_metal903 = loadMeshFile("../models/w9a02_metal903.obj");
	std::vector<uint8_t> w9_metal903_abd;
	unsigned int w9_metal903_abd_TexWidth, w9_metal903_abd_TexHeight;
	lodepng::decode(w9_metal903_abd, w9_metal903_abd_TexWidth, w9_metal903_abd_TexHeight, "../textures/w9_metal903_abd.png");
	//drawMesh(imageBuffer, zBuffer, w9a02_metal909, w9_metal909_abd, w9_metal909_abd_TexWidth, w9_metal909_abd_TexHeight, MeshTransform, worldToClip, lights, width, height);
	drawMesh(imageBuffer, zBuffer, w9a02_metal903, w9_metal903_abd, w9_metal903_abd_TexWidth, w9_metal903_abd_TexHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, MeshTransform, worldToCamera, projection, lights, width, height);
	std::cout << "Mesh w9a02_metal903 Drawn" << std::endl;

	Mesh w9a02_metal909 = loadMeshFile("../models/w9a02_metal909.obj");
	std::vector<uint8_t> w9_metal909_abd;
	unsigned int w9_metal909_abd_TexWidth, w9_metal909_abd_TexHeight;
	lodepng::decode(w9_metal909_abd, w9_metal909_abd_TexWidth, w9_metal909_abd_TexHeight, "../textures/w9_metal909_abd.png");
	//drawMesh(imageBuffer, zBuffer, w9a02_metal909, w9_metal909_abd, w9_metal909_abd_TexWidth, w9_metal909_abd_TexHeight, MeshTransform, worldToClip, lights, width, height);
	drawMesh(imageBuffer, zBuffer, w9a02_metal909, w9_metal909_abd, w9_metal909_abd_TexWidth, w9_metal909_abd_TexHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, MeshTransform, worldToCamera, projection, lights, width, height);
	std::cout << "Mesh w9a02_metal909 Drawn" << std::endl;

	Mesh w9a02_metal913mc = loadMeshFile("../models/w9a02_metal913mc.obj");
	std::vector<uint8_t> w9_metal913_abd;
	unsigned int w9_metal913_abd_TexWidth, w9_metal913_abd_TexHeight;
	lodepng::decode(w9_metal913_abd, w9_metal913_abd_TexWidth, w9_metal913_abd_TexHeight, "../textures/w9_metal913_abd.png");
	//drawMesh(imageBuffer, zBuffer, w9a02_metal913mc, w9_metal913_abd, w9_metal913_abd_TexWidth, w9_metal913_abd_TexHeight, MeshTransform, worldToClip, lights, width, height);
	drawMesh(imageBuffer, zBuffer, w9a02_metal913mc, w9_metal913_abd, w9_metal913_abd_TexWidth, w9_metal913_abd_TexHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, MeshTransform, worldToCamera, projection, lights, width, height);
	std::cout << "Mesh w9a02_metal913mc Drawn" << std::endl;

	Mesh w9a02_metalnail001 = loadMeshFile("../models/w9a02_metalnail001.obj");
	std::vector<uint8_t> w3_metal402_abd;
	unsigned int w3_metal402_abd_TexWidth, w3_metal402_abd_TexHeight;
	lodepng::decode(w3_metal402_abd, w3_metal402_abd_TexWidth, w3_metal402_abd_TexHeight, "../textures/w3_metal402_abd.png");
	//drawMesh(imageBuffer, zBuffer, w9a02_metalnail001, w3_metal402_abd, w3_metal402_abd_TexWidth, w3_metal402_abd_TexHeight, MeshTransform, worldToClip, lights, width, height);
	drawMesh(imageBuffer, zBuffer, w9a02_metalnail001, w3_metal402_abd, w3_metal402_abd_TexWidth, w3_metal402_abd_TexHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, MeshTransform, worldToCamera, projection, lights, width, height);
	std::cout << "Mesh w9a02_metalnail001 Drawn" << std::endl;

	Mesh w9a02_metalwall701 = loadMeshFile("../models/w9a02_metalwall701.obj");
	std::vector<uint8_t> w9_metal701_abd;
	unsigned int w9_metal701_abd_TexWidth, w9_metal701_abd_TexHeight;
	lodepng::decode(w9_metal701_abd, w9_metal701_abd_TexWidth, w9_metal701_abd_TexHeight, "../textures/w9_metal701_abd.png");
	//drawMesh(imageBuffer, zBuffer, w9a02_metalwall701, w9_metal701_abd, w9_metal701_abd_TexWidth, w9_metal701_abd_TexHeight, MeshTransform, worldToClip, lights, width, height);
	drawMesh(imageBuffer, zBuffer, w9a02_metalwall701, w9_metal701_abd, w9_metal701_abd_TexWidth, w9_metal701_abd_TexHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, MeshTransform, worldToCamera, projection, lights, width, height);
	std::cout << "Mesh w9a02_metalwall701 Drawn" << std::endl;

	Mesh w9a02_metalwall703 = loadMeshFile("../models/w9a02_metalwall703.obj");
	std::vector<uint8_t> w9_metal703_abd;
	unsigned int w9_metal703_abd_TexWidth, w9_metal703_abd_TexHeight;
	lodepng::decode(w9_metal703_abd, w9_metal703_abd_TexWidth, w9_metal703_abd_TexHeight, "../textures/w9_metal703_abd.png");
	//drawMesh(imageBuffer, zBuffer, w9a02_metalwall703, w9_metal703_abd, w9_metal703_abd_TexWidth, w9_metal703_abd_TexHeight, MeshTransform, worldToClip, lights, width, height);
	drawMesh(imageBuffer, zBuffer, w9a02_metalwall703, w9_metal703_abd, w9_metal703_abd_TexWidth, w9_metal703_abd_TexHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, MeshTransform, worldToCamera, projection, lights, width, height);
	std::cout << "Mesh w9a02_metalwall703 Drawn" << std::endl;

	Mesh w9a02_metalwall705 = loadMeshFile("../models/w9a02_metalwall705.obj");
	std::vector<uint8_t> w9_metal705_abd;
	unsigned int w9_metal705_abd_TexWidth, w9_metal705_abd_TexHeight;
	lodepng::decode(w9_metal705_abd, w9_metal705_abd_TexWidth, w9_metal705_abd_TexHeight, "../textures/w9_metal705_abd.png");
	//drawMesh(imageBuffer, zBuffer, w9a02_metalwall705, w9_metal705_abd, w9_metal705_abd_TexWidth, w9_metal705_abd_TexHeight, MeshTransform, worldToClip, lights, width, height);
	drawMesh(imageBuffer, zBuffer, w9a02_metalwall705, w9_metal705_abd, w9_metal705_abd_TexWidth, w9_metal705_abd_TexHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, MeshTransform, worldToCamera, projection, lights, width, height);
	std::cout << "Mesh w9a02_metalwall705 Drawn" << std::endl;

	Mesh w9a02_my1_brick01 = loadMeshFile("../models/w9a02_my1_brick01.obj");
	//drawMesh(imageBuffer, zBuffer, w9a02_my1_brick01, w3_brick001_abd, w3_brick001_abd_TexWidth, w3_brick001_abd_TexHeight, MeshTransform, worldToClip, lights, width, height);
	drawMesh(imageBuffer, zBuffer, w9a02_my1_brick01, w3_brick001_abd, w3_brick001_abd_TexWidth, w3_brick001_abd_TexHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, MeshTransform, worldToCamera, projection, lights, width, height);
	std::cout << "Mesh w9a02_my1_brick01 Drawn" << std::endl;

	Mesh w9a02_my1_concrete01 = loadMeshFile("../models/w9a02_my1_concrete01.obj");
	std::vector<uint8_t> w3_concrete001_abd;
	unsigned int w3_concrete001_abd_TexWidth, w3_concrete001_abd_TexHeight;
	lodepng::decode(w3_concrete001_abd, w3_concrete001_abd_TexWidth, w3_concrete001_abd_TexHeight, "../textures/w3_concrete001_abd.png");
	//drawMesh(imageBuffer, zBuffer, w9a02_my1_concrete01, w3_concrete001_abd, w3_concrete001_abd_TexWidth, w3_concrete001_abd_TexHeight, MeshTransform, worldToClip, lights, width, height);
	drawMesh(imageBuffer, zBuffer, w9a02_my1_concrete01, w3_concrete001_abd, w3_concrete001_abd_TexWidth, w3_concrete001_abd_TexHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, MeshTransform, worldToCamera, projection, lights, width, height);
	std::cout << "Mesh w9a02_my1_concrete01 Drawn" << std::endl;

	Mesh w9a02_my1_iron01 = loadMeshFile("../models/w9a02_my1_iron01.obj");
	std::vector<uint8_t> w3_iron001_abd;
	unsigned int w3_iron001_abd_TexWidth, w3_iron001_abd_TexHeight;
	lodepng::decode(w3_iron001_abd, w3_iron001_abd_TexWidth, w3_iron001_abd_TexHeight, "../textures/w3_iron001_abd.png");
	//drawMesh(imageBuffer, zBuffer, w9a02_my1_iron01, w3_iron001_abd, w3_iron001_abd_TexWidth, w3_iron001_abd_TexHeight, MeshTransform, worldToClip, lights, width, height);
	drawMesh(imageBuffer, zBuffer, w9a02_my1_iron01, w3_iron001_abd, w3_iron001_abd_TexWidth, w3_iron001_abd_TexHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, MeshTransform, worldToCamera, projection, lights, width, height);
	std::cout << "Mesh w9a02_my1_iron01 Drawn" << std::endl;

	Mesh w9a02_my1_wall01 = loadMeshFile("../models/w9a02_my1_wall01.obj");
	//drawMesh(imageBuffer, zBuffer, w9a02_my1_wall01, w3_wall001_abd, w3_wall001_abd_TexWidth, w3_wall001_abd_TexHeight, MeshTransform, worldToClip, lights, width, height);
	drawMesh(imageBuffer, zBuffer, w9a02_my1_wall01, w3_wall001_abd, w3_wall001_abd_TexWidth, w3_wall001_abd_TexHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, MeshTransform, worldToCamera, projection, lights, width, height);
	std::cout << "Mesh w9a02_my1_wall01 Drawn" << std::endl;

	Mesh w9a02_my1_wood04 = loadMeshFile("../models/w9a02_my1_wood04.obj");
	std::vector<uint8_t> w3_wood004_abd;
	unsigned int w3_wood004_abd_TexWidth, w3_wood004_abd_TexHeight;
	lodepng::decode(w3_wood004_abd, w3_wood004_abd_TexWidth, w3_wood004_abd_TexHeight, "../textures/w3_wood004_abd.png");
	//drawMesh(imageBuffer, zBuffer, w9a02_my1_wood04, w3_wood004_abd, w3_wood004_abd_TexWidth, w3_wood004_abd_TexHeight, MeshTransform, worldToClip, lights, width, height);
	drawMesh(imageBuffer, zBuffer, w9a02_my1_wood04, w3_wood004_abd, w3_wood004_abd_TexWidth, w3_wood004_abd_TexHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, MeshTransform, worldToCamera, projection, lights, width, height);
	std::cout << "Mesh w9a02_my1_wood04 Drawn" << std::endl;

	Mesh w9a02_neon745 = loadMeshFile("../models/w9a02_neon745.obj");
	std::vector<uint8_t> w9_neon723_ems;
	unsigned int w9_neon723_ems_TexWidth, w9_neon723_ems_TexHeight;
	lodepng::decode(w9_neon723_ems, w9_neon723_ems_TexWidth, w9_neon723_ems_TexHeight, "../textures/w9_neon723_ems.png");
	//drawMesh(imageBuffer, zBuffer, w9a02_neon745, w9_neon723_ems, w9_neon723_ems_TexWidth, w9_neon723_ems_TexHeight, MeshTransform, worldToClip, lights, width, height);
	drawMesh(imageBuffer, zBuffer, w9a02_neon745, w9_neon723_ems, w9_neon723_ems_TexWidth, w9_neon723_ems_TexHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, EMISSIVE, camWorldPos, MeshTransform, worldToCamera, projection, lights, width, height);
	std::cout << "Mesh w9a02_neon745 Drawn" << std::endl;

	Mesh w9a02_neon746 = loadMeshFile("../models/w9a02_neon746.obj");
	//drawMesh(imageBuffer, zBuffer, w9a02_neon746, w9_neon723_ems, w9_neon723_ems_TexWidth, w9_neon723_ems_TexHeight, MeshTransform, worldToClip, lights, width, height);
	drawMesh(imageBuffer, zBuffer, w9a02_neon746, w9_neon723_ems, w9_neon723_ems_TexWidth, w9_neon723_ems_TexHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, EMISSIVE, camWorldPos, MeshTransform, worldToCamera, projection, lights, width, height);
	std::cout << "Mesh w9a02_neon746 Drawn" << std::endl;

	Mesh w9a02_neon747 = loadMeshFile("../models/w9a02_neon747.obj");
	//drawMesh(imageBuffer, zBuffer, w9a02_neon747, w9_neon723_ems, w9_neon723_ems_TexWidth, w9_neon723_ems_TexHeight, MeshTransform, worldToClip, lights, width, height);
	drawMesh(imageBuffer, zBuffer, w9a02_neon747, w9_neon723_ems, w9_neon723_ems_TexWidth, w9_neon723_ems_TexHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, EMISSIVE, camWorldPos, MeshTransform, worldToCamera, projection, lights, width, height);
	std::cout << "Mesh w9a02_neon747 Drawn" << std::endl;

	Mesh w9a02_neon748 = loadMeshFile("../models/w9a02_neon748.obj");
	//drawMesh(imageBuffer, zBuffer, w9a02_neon748, w9_neon723_ems, w9_neon723_ems_TexWidth, w9_neon723_ems_TexHeight, MeshTransform, worldToClip, lights, width, height);
	drawMesh(imageBuffer, zBuffer, w9a02_neon748, w9_neon723_ems, w9_neon723_ems_TexWidth, w9_neon723_ems_TexHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, EMISSIVE, camWorldPos, MeshTransform, worldToCamera, projection, lights, width, height);
	std::cout << "Mesh w9a02_neon748 Drawn" << std::endl;

	Mesh w9a02_pipe901 = loadMeshFile("../models/w9a02_pipe901.obj");
	std::vector<uint8_t> w9_pipe901_abd;
	unsigned int w9_pipe901_abd_TexWidth, w9_pipe901_abd_TexHeight;
	lodepng::decode(w9_pipe901_abd, w9_pipe901_abd_TexWidth, w9_pipe901_abd_TexHeight, "../textures/w9_pipe901_abd.png");
	//drawMesh(imageBuffer, zBuffer, w9a02_pipe901, w9_pipe901_abd, w9_pipe901_abd_TexWidth, w9_pipe901_abd_TexHeight, MeshTransform, worldToClip, lights, width, height);
	drawMesh(imageBuffer, zBuffer, w9a02_pipe901, w9_pipe901_abd, w9_pipe901_abd_TexWidth, w9_pipe901_abd_TexHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, MeshTransform, worldToCamera, projection, lights, width, height);
	std::cout << "Mesh w9a02_pipe901 Drawn" << std::endl;

	Mesh w9a02_tire01 = loadMeshFile("../models/w9a02_tire01.obj");
	std::vector<uint8_t> w3_rubber401_abd;
	unsigned int w3_rubber401_abd_TexWidth, w3_rubber401_abd_TexHeight;
	lodepng::decode(w3_rubber401_abd, w3_rubber401_abd_TexWidth, w3_rubber401_abd_TexHeight, "../textures/w3_rubber401_abd.png");
	//drawMesh(imageBuffer, zBuffer, w9a02_tire01, w3_rubber401_abd, w3_rubber401_abd_TexWidth, w3_rubber401_abd_TexHeight, MeshTransform, worldToClip, lights, width, height);
	drawMesh(imageBuffer, zBuffer, w9a02_tire01, w3_rubber401_abd, w3_rubber401_abd_TexWidth, w3_rubber401_abd_TexHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, MeshTransform, worldToCamera, projection, lights, width, height);
	std::cout << "Mesh w9a02_tire01 Drawn" << std::endl;

	Mesh w9a02_wall03 = loadMeshFile("../models/w9a02_wall03.obj"); //BROKEN RENDERERING???
	std::vector<uint8_t> w3_mat_km1_wall03;
	unsigned int w3_wall203_abd_TexWidth, w3_wall203_abd_abd_TexHeight;
	lodepng::decode(w3_mat_km1_wall03, w3_wall203_abd_TexWidth, w3_wall203_abd_abd_TexHeight, "../textures/w3_wall203_abd.png");
	//drawMesh(imageBuffer, zBuffer, w9a02_wall03, w3_mat_km1_wall03, w3_wall203_abd_TexWidth, w3_wall203_abd_abd_TexHeight, MeshTransform, worldToClip, lights, width, height);
	drawMesh(imageBuffer, zBuffer, w9a02_wall03, w3_mat_km1_wall03, w3_wall203_abd_TexWidth, w3_wall203_abd_abd_TexHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, MeshTransform, worldToCamera, projection, lights, width, height);
	std::cout << "Mesh w9a02_wall03 Drawn" << std::endl;

	Mesh w9a02_wall05 = loadMeshFile("../models/w9a02_wall05.obj"); 
	std::vector<uint8_t> w3_wall205_abd;
	unsigned int w3_wall205_abd_TexWidth, w3_wall205_abd_TexHeight;
	lodepng::decode(w3_wall205_abd, w3_wall205_abd_TexWidth, w3_wall205_abd_TexHeight, "../textures/w3_wall205_abd.png");
	//drawMesh(imageBuffer, zBuffer, w9a02_wall05, w3_wall205_abd, w3_wall205_abd_TexWidth, w3_wall205_abd_TexHeight, MeshTransform, worldToClip, lights, width, height);
	drawMesh(imageBuffer, zBuffer, w9a02_wall05, w3_wall205_abd, w3_wall205_abd_TexWidth, w3_wall205_abd_TexHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, MeshTransform, worldToCamera, projection, lights, width, height);
	std::cout << "Mesh w9a02_wall05 Drawn" << std::endl;

	Mesh w9a02_wood01 = loadMeshFile("../models/w9a02_wood01.obj");
	std::vector<uint8_t> w3_woodburn401_abd;
	unsigned int w3_woodburn401_abd_TexWidth, w3_woodburn401_abd_TexHeight;
	lodepng::decode(w3_woodburn401_abd, w3_woodburn401_abd_TexWidth, w3_woodburn401_abd_TexHeight, "../textures/w3_woodburn401_abd.png");
	//drawMesh(imageBuffer, zBuffer, w9a02_wood01, w3_woodburn401_abd, w3_woodburn401_abd_TexWidth, w3_woodburn401_abd_TexHeight, MeshTransform, worldToClip, lights, width, height);
	drawMesh(imageBuffer, zBuffer, w9a02_wood01, w3_woodburn401_abd, w3_woodburn401_abd_TexWidth, w3_woodburn401_abd_TexHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, MeshTransform, worldToCamera, projection, lights, width, height);
	std::cout << "Mesh w9a02_wood01 Drawn" << std::endl;

	Mesh w9a02_wood03 = loadMeshFile("../models/w9a02_wood03.obj");
	std::vector<uint8_t> w3_wood303_abd;
	unsigned int w3_wood303_abd_TexWidth, w3_wood303_abd_TexHeight;
	lodepng::decode(w3_wood303_abd, w3_wood303_abd_TexWidth, w3_wood303_abd_TexHeight, "../textures/w3_wood303_abd.png");
	//drawMesh(imageBuffer, zBuffer, w9a02_wood03, w3_wood303_abd, w3_wood303_abd_TexWidth, w3_wood303_abd_TexHeight, MeshTransform, worldToClip, lights, width, height);
	drawMesh(imageBuffer, zBuffer, w9a02_wood03, w3_wood303_abd, w3_wood303_abd_TexWidth, w3_wood303_abd_TexHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, MeshTransform, worldToCamera, projection, lights, width, height);
	std::cout << "Mesh w9a02_wood03 Drawn" << std::endl;

	Mesh w9a02_wood05 = loadMeshFile("../models/w9a02_wood05.obj");
	std::vector<uint8_t> w3_wood005_abd;
	unsigned int w3_wood005_abd_TexWidth, w3_wood005_abd_TexHeight;
	lodepng::decode(w3_wood005_abd, w3_wood005_abd_TexWidth, w3_wood005_abd_TexHeight, "../textures/w3_wood005_abd.png");
	//drawMesh(imageBuffer, zBuffer, w9a02_wood05, w3_wood005_abd, w3_wood005_abd_TexWidth, w3_wood005_abd_TexHeight, MeshTransform, worldToClip, lights, width, height);
	drawMesh(imageBuffer, zBuffer, w9a02_wood05, w3_wood005_abd, w3_wood005_abd_TexWidth, w3_wood005_abd_TexHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, MeshTransform, worldToCamera, projection, lights, width, height);
	std::cout << "Mesh w9a02_wood05 Drawn" << std::endl;

	Mesh w9a02_ym2_iron01 = loadMeshFile("../models/w9a02_ym2_iron01.obj");
	std::vector<uint8_t> w3_iron401_abd;
	unsigned int w3_iron401_abd_TexWidth, w3_iron401_abd_TexHeight;
	lodepng::decode(w3_iron401_abd, w3_iron401_abd_TexWidth, w3_iron401_abd_TexHeight, "../textures/w3_iron401_abd.png");
	//drawMesh(imageBuffer, zBuffer, w9a02_ym2_iron01, w3_iron401_abd, w3_iron401_abd_TexWidth, w3_iron401_abd_TexHeight, MeshTransform, worldToClip, lights, width, height);
	drawMesh(imageBuffer, zBuffer, w9a02_ym2_iron01, w3_iron401_abd, w3_iron401_abd_TexWidth, w3_iron401_abd_TexHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, MeshTransform, worldToCamera, projection, lights, width, height);
	std::cout << "Mesh w9a02_ym2_iron01 Drawn" << std::endl;

	////////////////////////////////////////////////	CONSOLE		////////////////////////////////////////////////////////////

	Mesh ev_obj_w9_hideconsole_metal901 = loadMeshFile("../models/ev_obj_w9_hideconsole_metal901.obj");
	//drawMesh(imageBuffer, zBuffer, ConsoleMesh2, m9_metalwall702_kd1, metalwall702_TexWidth, metalwall702_TexHeight, MeshTransform, worldToClip, lights, width, height);
	drawMesh(imageBuffer, zBuffer, ev_obj_w9_hideconsole_metal901, w9_metal901_abd, w9_metal901_abd_TexWidth, w9_metal901_abd_TexHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, MeshTransform, worldToCamera, projection, lights, width, height);
	std::cout << "Mesh ev_obj_w9_hideconsole_metal901 Drawn" << std::endl;

	Mesh ev_obj_w9_hideconsole_metalwall702 = loadMeshFile("../models/ev_obj_w9_hideconsole_metalwall702.obj");
	std::vector<uint8_t> m9_metalwall702_kd1;
	unsigned int metalwall702_TexWidth, metalwall702_TexHeight;
	lodepng::decode(m9_metalwall702_kd1, metalwall702_TexWidth, metalwall702_TexHeight, "../textures/w9_metal702_abd.png");
	//drawMesh(imageBuffer, zBuffer, ConsoleMesh2, m9_metalwall702_kd1, metalwall702_TexWidth, metalwall702_TexHeight, MeshTransform, worldToClip, lights, width, height);
	drawMesh(imageBuffer, zBuffer, ev_obj_w9_hideconsole_metalwall702, m9_metalwall702_kd1, metalwall702_TexWidth, metalwall702_TexHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, MeshTransform, worldToCamera, projection, lights, width, height);
	std::cout << "Mesh ev_obj_w9_hideconsole_metalwall702 Drawn" << std::endl;

	////////////////////////////////////////////////	DOORS		////////////////////////////////////////////////////////////

	Mesh ev_obj_w9_hidedoors_metal702 = loadMeshFile("../models/ev_obj_w9_hidedoors_metal702.obj");
	std::vector<uint8_t> w9_metal744_abd;
	unsigned int metal744_TexWidth, metal744_TexHeight;
	lodepng::decode(w9_metal744_abd, metal744_TexWidth, metal744_TexHeight, "../textures/w9_metal744_abd.png");
	//drawMesh(imageBuffer, zBuffer, ConsoleMesh3, w9_metal744_abd, metal744_TexWidth, metal744_TexHeight, MeshTransform, worldToClip, lights, width, height);
	drawMesh(imageBuffer, zBuffer, ev_obj_w9_hidedoors_metal702, w9_metal744_abd, metal744_TexWidth, metal744_TexHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, MeshTransform, worldToCamera, projection, lights, width, height);
	std::cout << "Mesh ev_obj_w9_hidedoors_metal702 Drawn" << std::endl;

	Mesh ev_obj_w9_hidedoors_metal701 = loadMeshFile("../models/ev_obj_w9_hidedoors_metal701.obj");
	drawMesh(imageBuffer, zBuffer, ev_obj_w9_hidedoors_metal701, w9_metal701_abd, w9_metal701_abd_TexWidth, w9_metal701_abd_TexWidth, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, MeshTransform, worldToCamera, projection, lights, width, height);
	std::cout << "Mesh ev_obj_w9_hidedoors_metal701 Drawn" << std::endl;


	Mesh ev_obj_w9_hidedoors_metal154 = loadMeshFile("../models/ev_obj_w9_hidedoors_metal154.obj");
	drawMesh(imageBuffer, zBuffer, ev_obj_w9_hidedoors_metal154, w9_metal154_abd, w9_metal154_abd_TexWidth, w9_metal154_abd_TexHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, MeshTransform, worldToCamera, projection, lights, width, height);
	std::cout << "Mesh ev_obj_w9_hidedoors_metal154 Drawn" << std::endl;

	////////////////////////////////////////////////	MONITOR		////////////////////////////////////////////////////////////

	Mesh ev_obj_w9_hidemonitor_metal154 = loadMeshFile("../models/ev_obj_w9_hidemonitor_metal154.obj");
	drawMesh(imageBuffer, zBuffer, ev_obj_w9_hidemonitor_metal154, w9_metal154_abd, w9_metal154_abd_TexWidth, w9_metal154_abd_TexHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, MeshTransform, worldToCamera, projection, lights, width, height);
	std::cout << "Mesh ev_obj_w9_hidemonitor_metal154 Drawn" << std::endl;

	Mesh ev_obj_w9_hidemonitor_metal748 = loadMeshFile("../models/ev_obj_w9_hidemonitor_metal748.obj");
	std::vector<uint8_t> w9_metal748_abd;
	unsigned int metal748_TexWidth, metal748_TexHeight;
	lodepng::decode(w9_metal748_abd, metal748_TexWidth, metal748_TexHeight, "../textures/w9_metal748_abd.png");
	drawMesh(imageBuffer, zBuffer, ev_obj_w9_hidemonitor_metal748, w9_metal748_abd, metal748_TexWidth, metal748_TexHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, MeshTransform, worldToCamera, projection, lights, width, height);
	std::cout << "Mesh ev_obj_w9_hidemonitor_metal748 Drawn" << std::endl;

	Mesh ev_obj_w9_hidemonitor_movtex = loadMeshFile("../models/ev_obj_w9_hidemonitor_movtex.obj");
	std::vector<uint8_t> ev_movtex_dummy;
	unsigned int ev_movtex_dummy_TexWidth, ev_movtex_dummy_TexHeight;
	lodepng::decode(ev_movtex_dummy, ev_movtex_dummy_TexWidth, ev_movtex_dummy_TexHeight, "../textures/ev_movtex_dummy.png");
	drawMesh(imageBuffer, zBuffer, ev_obj_w9_hidemonitor_movtex, ev_movtex_dummy, ev_movtex_dummy_TexWidth, ev_movtex_dummy_TexHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, EMISSIVE, camWorldPos, MeshTransform, worldToCamera, projection, lights, width, height);
	std::cout << "Mesh ev_obj_w9_hidemonitor_movtex Drawn" << std::endl;

	////////////////////////////////////////////////	AMY		////////////////////////////////////////////////////////////

	Mesh Amy_Skin = loadMeshFile("../models/Amy_Skin.obj");
	std::vector<uint8_t> chr_amy_skin_abd;
	unsigned int chr_amy_skin_abd_TexWidth, chr_amy_skin_abd_TexHeight;
	lodepng::decode(chr_amy_skin_abd, chr_amy_skin_abd_TexWidth, chr_amy_skin_abd_TexHeight, "../textures/chr_amy_skin_abd.png");
	drawMesh(imageBuffer, zBuffer, Amy_Skin, chr_amy_skin_abd, chr_amy_skin_abd_TexWidth, chr_amy_skin_abd_TexHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, MeshTransform, worldToCamera, projection, lights, width, height);
	std::cout << "Mesh Amy_Skin Drawn" << std::endl;

	Mesh Amy_Dress = loadMeshFile("../models/Amy_Dress.obj");
	std::vector<uint8_t> chr_amy_dress_abd;
	unsigned int chr_amy_dress_abd_TexWidth, chr_amy_dress_abd_TexHeight;
	lodepng::decode(chr_amy_dress_abd, chr_amy_dress_abd_TexWidth, chr_amy_dress_abd_TexHeight, "../textures/chr_amy_dress_abd.png");
	drawMesh(imageBuffer, zBuffer, Amy_Dress, chr_amy_dress_abd, chr_amy_dress_abd_TexWidth, chr_amy_dress_abd_TexHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, MeshTransform, worldToCamera, projection, lights, width, height);
	std::cout << "Mesh Amy_Dress Drawn" << std::endl;

	Mesh Amy_Cloth = loadMeshFile("../models/Amy_Cloth.obj");
	std::vector<uint8_t> chr_amy_cloth_abd;
	unsigned int chr_amy_cloth_abd_TexWidth, chr_amy_cloth_abd_TexHeight;
	lodepng::decode(chr_amy_cloth_abd, chr_amy_cloth_abd_TexWidth, chr_amy_cloth_abd_TexHeight, "../textures/chr_amy_cloth_abd.png");
	drawMesh(imageBuffer, zBuffer, Amy_Cloth, chr_amy_cloth_abd, chr_amy_cloth_abd_TexWidth, chr_amy_cloth_abd_TexHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, MeshTransform, worldToCamera, projection, lights, width, height);
	std::cout << "Mesh Amy_Cloth Drawn" << std::endl;

	////////////////////////////////////////////////	KNUCKLES		////////////////////////////////////////////////////////////

	Mesh Knuckles_body = loadMeshFile("../models/Knuckles_body.obj");
	std::vector<uint8_t> chr_knuckles_body_abd;
	unsigned int chr_knuckles_body_abd_TexWidth, chr_knuckles_body_abd_TexHeight;
	lodepng::decode(chr_knuckles_body_abd, chr_knuckles_body_abd_TexWidth, chr_knuckles_body_abd_TexHeight, "../textures/chr_knuckles_body_abd.png");
	drawMesh(imageBuffer, zBuffer, Knuckles_body, chr_knuckles_body_abd, chr_knuckles_body_abd_TexWidth, chr_knuckles_body_abd_TexHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, MeshTransform, worldToCamera, projection, lights, width, height);
	std::cout << "Mesh Knuckles_body Drawn" << std::endl;

	Mesh Knuckles_cloth = loadMeshFile("../models/Knuckles_cloth.obj");
	std::vector<uint8_t> chr_knuckles_cloth_abd;
	unsigned int chr_knuckles_cloth_abd_TexWidth, chr_knuckles_cloth_abd_TexHeight;
	lodepng::decode(chr_knuckles_cloth_abd, chr_knuckles_cloth_abd_TexWidth, chr_knuckles_cloth_abd_TexHeight, "../textures/chr_knuckles_cloth_abd.png");
	drawMesh(imageBuffer, zBuffer, Knuckles_cloth, chr_knuckles_cloth_abd, chr_knuckles_cloth_abd_TexWidth, chr_knuckles_cloth_abd_TexHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, MeshTransform, worldToCamera, projection, lights, width, height);
	std::cout << "Mesh Knuckles_cloth Drawn" << std::endl;

	Mesh Knuckles_shoe = loadMeshFile("../models/Knuckles_shoe.obj");
	std::vector<uint8_t> chr_knuckles_shoes_abd;
	unsigned int chr_knuckles_shoes_abd_TexWidth, chr_knuckles_shoes_abd_TexHeight;
	lodepng::decode(chr_knuckles_shoes_abd, chr_knuckles_shoes_abd_TexWidth, chr_knuckles_shoes_abd_TexHeight, "../textures/chr_knuckles_shoes_abd.png");
	drawMesh(imageBuffer, zBuffer, Knuckles_shoe, chr_knuckles_shoes_abd, chr_knuckles_shoes_abd_TexWidth, chr_knuckles_shoes_abd_TexHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, MeshTransform, worldToCamera, projection, lights, width, height);
	std::cout << "Mesh Knuckles_shoe Drawn" << std::endl;

	Mesh Knuckles_eye = loadMeshFile("../models/Knuckles_eye.obj");
	std::vector<uint8_t> chr_knuckles_eye_abd;
	unsigned int chr_knuckles_eye_abd_TexWidth, chr_knuckles_eye_abd_TexHeight;
	lodepng::decode(chr_knuckles_eye_abd, chr_knuckles_eye_abd_TexWidth, chr_knuckles_eye_abd_TexHeight, "../textures/chr_knuckles_eye_abd.png");
	drawMesh(imageBuffer, zBuffer, Knuckles_eye, chr_knuckles_eye_abd, chr_knuckles_eye_abd_TexWidth, chr_knuckles_eye_abd_TexHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, MeshTransform, worldToCamera, projection, lights, width, height);
	std::cout << "Mesh Knuckles_eye Drawn" << std::endl;

	////////////////////////////////////////////////	SILVER	////////////////////////////////////////////////////////////

	Mesh Silver_body = loadMeshFile("../models/Silver_body.obj");
	std::vector<uint8_t> chr_silver_body_abd;
	unsigned int chr_silver_body_abd_TexWidth, chr_silver_body_abd_TexHeight;
	lodepng::decode(chr_silver_body_abd, chr_silver_body_abd_TexWidth, chr_silver_body_abd_TexHeight, "../textures/chr_silver_body_abd.png");
	drawMesh(imageBuffer, zBuffer, Silver_body, chr_silver_body_abd, chr_silver_body_abd_TexWidth, chr_silver_body_abd_TexHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, MeshTransform, worldToCamera, projection, lights, width, height);
	std::cout << "Mesh Silver_body Drawn" << std::endl;

	Mesh Silver_cell = loadMeshFile("../models/Silver_cell.obj");
	std::vector<uint8_t> chr_silver_cell_abd;
	unsigned int chr_silver_cell_abd_TexWidth, chr_silver_cell_abd_TexHeight;
	lodepng::decode(chr_silver_cell_abd, chr_silver_cell_abd_TexWidth, chr_silver_cell_abd_TexHeight, "../textures/chr_silver_cell_abd.png");
	drawMesh(imageBuffer, zBuffer, Silver_cell, chr_silver_cell_abd, chr_silver_cell_abd_TexWidth, chr_silver_cell_abd_TexHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, MeshTransform, worldToCamera, projection, lights, width, height);
	std::cout << "Mesh Silver_cell Drawn" << std::endl;

	Mesh Silver_cloth = loadMeshFile("../models/Silver_cloth.obj");
	std::vector<uint8_t> chr_silver_cloth_abd;
	unsigned int chr_silver_cloth_abd_TexWidth, chr_silver_cloth_abd_TexHeight;
	lodepng::decode(chr_silver_cloth_abd, chr_silver_cloth_abd_TexWidth, chr_silver_cloth_abd_TexHeight, "../textures/chr_silver_cloth_abd.png");
	drawMesh(imageBuffer, zBuffer, Silver_cloth, chr_silver_cloth_abd, chr_silver_cloth_abd_TexWidth, chr_silver_cloth_abd_TexHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, MeshTransform, worldToCamera, projection, lights, width, height);
	std::cout << "Mesh Silver_cloth Drawn" << std::endl;

	Mesh Silver_fur = loadMeshFile("../models/Silver_fur.obj");
	std::vector<uint8_t> chr_silver_fur_abd;
	unsigned int chr_silver_fur_abd_TexWidth, chr_silver_fur_abd_TexHeight;
	lodepng::decode(chr_silver_fur_abd, chr_silver_fur_abd_TexWidth, chr_silver_fur_abd_TexHeight, "../textures/chr_silver_fur_abd.png");
	drawMesh(imageBuffer, zBuffer, Silver_fur, chr_silver_fur_abd, chr_silver_fur_abd_TexWidth, chr_silver_fur_abd_TexHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, MeshTransform, worldToCamera, projection, lights, width, height);
	std::cout << "Mesh Silver_fur Drawn" << std::endl;

	Mesh Silver_light = loadMeshFile("../models/Silver_light.obj");
	std::vector<uint8_t> chr_silver_light_abd;
	unsigned int chr_silver_light_abd_TexWidth, chr_silver_light_abd_TexHeight;
	lodepng::decode(chr_silver_light_abd, chr_silver_light_abd_TexWidth, chr_silver_light_abd_TexHeight, "../textures/chr_silver_light_abd.png");
	drawMesh(imageBuffer, zBuffer, Silver_light, chr_silver_light_abd, chr_silver_light_abd_TexWidth, chr_silver_light_abd_TexHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, EMISSIVE, camWorldPos, MeshTransform, worldToCamera, projection, lights, width, height);
	std::cout << "Mesh Silver_light Drawn" << std::endl;


	////////////////////////////////////////////////	CHARMY	////////////////////////////////////////////////////////////

	Mesh Charmy_black = loadMeshFile("../models/charmy_black.obj");
	std::vector<uint8_t> chr_charmy_black_abd;
	unsigned int chr_charmy_black_abd_TexWidth, chr_charmy_black_abd_TexHeight;
	lodepng::decode(chr_charmy_black_abd, chr_charmy_black_abd_TexWidth, chr_charmy_black_abd_TexHeight, "../textures/chr_charmy_black_abd.png");
	drawMesh(imageBuffer, zBuffer, Charmy_black, chr_charmy_black_abd, chr_charmy_black_abd_TexWidth, chr_charmy_black_abd_TexHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, MeshTransform, worldToCamera, projection, lights, width, height);
	std::cout << "Mesh Charmy_black Drawn" << std::endl;

	Mesh Charmy_cloth = loadMeshFile("../models/charmy_cloth.obj");
	std::vector<uint8_t> chr_charmy_cloth_abd;
	unsigned int chr_charmy_cloth_abd_TexWidth, chr_charmy_cloth_abd_TexHeight;
	lodepng::decode(chr_charmy_cloth_abd, chr_charmy_cloth_abd_TexWidth, chr_charmy_cloth_abd_TexHeight, "../textures/chr_charmy_cloth_abd.png");
	drawMesh(imageBuffer, zBuffer, Charmy_cloth, chr_charmy_cloth_abd, chr_charmy_cloth_abd_TexWidth, chr_charmy_cloth_abd_TexHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, MeshTransform, worldToCamera, projection, lights, width, height);
	std::cout << "Mesh Charmy_cloth Drawn" << std::endl;

	Mesh Charmy_skin = loadMeshFile("../models/charmy_skin.obj");
	std::vector<uint8_t> chr_charmy_skin_abd;
	unsigned int chr_charmy_skin_abd_TexWidth, chr_charmy_skin_abd_TexHeight;
	lodepng::decode(chr_charmy_skin_abd, chr_charmy_skin_abd_TexWidth, chr_charmy_skin_abd_TexHeight, "../textures/chr_charmy_skin_abd.png");
	drawMesh(imageBuffer, zBuffer, Charmy_skin, chr_charmy_skin_abd, chr_charmy_skin_abd_TexWidth, chr_charmy_skin_abd_TexHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, MeshTransform, worldToCamera, projection, lights, width, height);
	std::cout << "Mesh Charmy_skin Drawn" << std::endl;

	////////////////////////////////////////////////	ESPIO	////////////////////////////////////////////////////////////

	Mesh Espio_body = loadMeshFile("../models/espio_body.obj");
	std::vector<uint8_t> chr_espio_body_abd;
	unsigned int chr_espio_body_abd_TexWidth, chr_espio_body_abd_TexHeight;
	lodepng::decode(chr_espio_body_abd, chr_espio_body_abd_TexWidth, chr_espio_body_abd_TexHeight, "../textures/chr_espio_skin_abd.png");
	drawMesh(imageBuffer, zBuffer, Espio_body, chr_espio_body_abd, chr_espio_body_abd_TexWidth, chr_espio_body_abd_TexHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, MeshTransform, worldToCamera, projection, lights, width, height);
	std::cout << "Mesh Espio_body Drawn" << std::endl;

	Mesh Espio_cloth = loadMeshFile("../models/espio_cloth.obj");
	std::vector<uint8_t> chr_espio_cloth_abd;
	unsigned int chr_espio_cloth_abd_TexWidth, chr_espio_cloth_abd_TexHeight;
	lodepng::decode(chr_espio_cloth_abd, chr_espio_cloth_abd_TexWidth, chr_espio_cloth_abd_TexHeight, "../textures/chr_espio_cloth_abd.png");
	drawMesh(imageBuffer, zBuffer, Espio_cloth, chr_espio_cloth_abd, chr_espio_cloth_abd_TexWidth, chr_espio_cloth_abd_TexHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, MeshTransform, worldToCamera, projection, lights, width, height);
	std::cout << "Mesh Espio_cloth Drawn" << std::endl;

	Mesh Espio_skin = loadMeshFile("../models/espio_skin.obj");
	std::vector<uint8_t> chr_espio_skin_abd;
	unsigned int chr_espio_skin_abd_TexWidth, chr_espio_skin_abd_TexHeight;
	lodepng::decode(chr_espio_skin_abd, chr_espio_skin_abd_TexWidth, chr_espio_skin_abd_TexHeight, "../textures/chr_espio_skin_abd.png");
	drawMesh(imageBuffer, zBuffer, Espio_skin, chr_espio_skin_abd, chr_espio_skin_abd_TexWidth, chr_espio_skin_abd_TexHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, MeshTransform, worldToCamera, projection, lights, width, height);
	std::cout << "Mesh Espio_skin Drawn" << std::endl;

	Mesh Espio_leather = loadMeshFile("../models/espio_leather.obj");
	std::vector<uint8_t> chr_espio_leather_abd;
	unsigned int chr_espio_leather_abd_TexWidth, chr_espio_leather_abd_TexHeight;
	lodepng::decode(chr_espio_leather_abd, chr_espio_leather_abd_TexWidth, chr_espio_leather_abd_TexHeight, "../textures/chr_espio_parts_abd.png");
	drawMesh(imageBuffer, zBuffer, Espio_leather, chr_espio_leather_abd, chr_espio_leather_abd_TexWidth, chr_espio_leather_abd_TexHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, MeshTransform, worldToCamera, projection, lights, width, height);
	std::cout << "Mesh Espio_leather Drawn" << std::endl;

	Mesh Espio_eye = loadMeshFile("../models/espio_eye.obj");
	std::vector<uint8_t> chr_espio_eye_abd;
	unsigned int chr_espio_eye_abd_TexWidth, chr_espio_eye_abd_TexHeight;
	lodepng::decode(chr_espio_eye_abd, chr_espio_eye_abd_TexWidth, chr_espio_eye_abd_TexHeight, "../textures/chr_espio_eye_abd.png");
	drawMesh(imageBuffer, zBuffer, Espio_eye, chr_espio_eye_abd, chr_espio_eye_abd_TexWidth, chr_espio_eye_abd_TexHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, MeshTransform, worldToCamera, projection, lights, width, height);
	std::cout << "Mesh Espio_eye Drawn" << std::endl;

	////////////////////////////////////////////////	VECTOR	////////////////////////////////////////////////////////////

	Mesh Vector_cloth = loadMeshFile("../models/vector_cloth.obj");
	std::vector<uint8_t> chr_vector_cloth_abd;
	unsigned int chr_vector_cloth_abd_TexWidth, chr_vector_cloth_abd_TexHeight;
	lodepng::decode(chr_vector_cloth_abd, chr_vector_cloth_abd_TexWidth, chr_vector_cloth_abd_TexHeight, "../textures/chr_vector_cloth_abd.png");
	drawMesh(imageBuffer, zBuffer, Vector_cloth, chr_vector_cloth_abd, chr_vector_cloth_abd_TexWidth, chr_vector_cloth_abd_TexHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, MeshTransform, worldToCamera, projection, lights, width, height);
	std::cout << "Mesh Vector_cloth Drawn" << std::endl;

	Mesh Vector_skin = loadMeshFile("../models/vector_skin.obj");
	std::vector<uint8_t> chr_vector_skin_abd;
	unsigned int chr_vector_skin_abd_TexWidth, chr_vector_skin_abd_TexHeight;
	lodepng::decode(chr_vector_skin_abd, chr_vector_skin_abd_TexWidth, chr_vector_skin_abd_TexHeight, "../textures/chr_vector_skin_abd.png");
	drawMesh(imageBuffer, zBuffer, Vector_skin, chr_vector_skin_abd, chr_vector_skin_abd_TexWidth, chr_vector_skin_abd_TexHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, MeshTransform, worldToCamera, projection, lights, width, height);
	std::cout << "Mesh Vector_skin Drawn" << std::endl;

	Mesh Vector_leather = loadMeshFile("../models/vector_leather.obj");
	std::vector<uint8_t> chr_vector_leather_abd;
	unsigned int chr_vector_leather_abd_TexWidth, chr_vector_leather_abd_TexHeight;
	lodepng::decode(chr_vector_leather_abd, chr_vector_leather_abd_TexWidth, chr_vector_leather_abd_TexHeight, "../textures/chr_vector_parts_abd.png");
	drawMesh(imageBuffer, zBuffer, Vector_leather, chr_vector_leather_abd, chr_vector_leather_abd_TexWidth, chr_vector_leather_abd_TexHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, MeshTransform, worldToCamera, projection, lights, width, height);
	std::cout << "Mesh Vector_leather Drawn" << std::endl;

	Mesh Vector_eye = loadMeshFile("../models/vector_eye.obj");
	std::vector<uint8_t> chr_vector_eye_abd;
	unsigned int chr_vector_eye_abd_TexWidth, chr_vector_eye_abd_TexHeight;
	lodepng::decode(chr_vector_eye_abd, chr_vector_eye_abd_TexWidth, chr_vector_eye_abd_TexHeight, "../textures/chr_vector_eye_abd.png");
	drawMesh(imageBuffer, zBuffer, Vector_eye, chr_vector_eye_abd, chr_vector_eye_abd_TexWidth, chr_vector_eye_abd_TexHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, MeshTransform, worldToCamera, projection, lights, width, height);
	std::cout << "Mesh Vector_eye Drawn" << std::endl;

	////////////////////////////////////////////////	LAST: TRANSPARENT MESHES		////////////////////////////////////////////////////////////

	Mesh ev_obj_w9_hideconsole_glass740 = loadMeshFile("../models/ev_obj_w9_hideconsole_glass740.obj");
	std::vector<uint8_t> m9_glass740_kd1;
	unsigned int glass740_TexWidth, glass740_TexHeight;
	lodepng::decode(m9_glass740_kd1, glass740_TexWidth, glass740_TexHeight, "../textures/w9_glass740_abd_a.png");
	//drawMesh(imageBuffer, zBuffer, ConsoleMesh3, m9_glass740_kd1, glass740_TexWidth, glass740_TexHeight, MeshTransform, worldToClip, lights, width, height);
	drawMesh(imageBuffer, zBuffer, ev_obj_w9_hideconsole_glass740, m9_glass740_kd1, glass740_TexWidth, glass740_TexHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, MeshTransform, worldToCamera, projection, lights, width, height);
	std::cout << "Mesh ev_obj_w9_hideconsole_glass740 Drawn" << std::endl;

	Mesh w9a02_monitor721 = loadMeshFile("../models/w9a02_monitor721.obj");
	std::vector<uint8_t> m9_monitor721_kd1;
	unsigned int monitor721_TexWidth, monitor721_TexHeight;
	lodepng::decode(m9_monitor721_kd1, monitor721_TexWidth, monitor721_TexHeight, "../textures/w9_monitor720_abd.png");
	//drawMesh(imageBuffer, zBuffer, w9a02_monitor721, m9_monitor721_kd1, monitor721_TexWidth, monitor721_TexHeight, MeshTransform, worldToClip, lights, width, height);
	drawMesh(imageBuffer, zBuffer, w9a02_monitor721, m9_monitor721_kd1, monitor721_TexWidth, monitor721_TexHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, EMISSIVE, camWorldPos, MeshTransform, worldToCamera, projection, lights, width, height);
	std::cout << "Mesh w9a02_monitor721 Drawn" << std::endl;

	Mesh ev_obj_w9_hideconsole_monitor721 = loadMeshFile("../models/ev_obj_w9_hideconsole_monitor721.obj");
	//drawMesh(imageBuffer, zBuffer, ConsoleMesh1, m9_monitor721_kd1, monitor721_TexWidth, monitor721_TexHeight, MeshTransform, worldToClip, lights, width, height);
	drawMesh(imageBuffer, zBuffer, ev_obj_w9_hideconsole_monitor721, m9_monitor721_kd1, monitor721_TexWidth, monitor721_TexHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, EMISSIVE, camWorldPos, MeshTransform, worldToCamera, projection, lights, width, height);
	std::cout << "Mesh ev_obj_w9_hideconsole_monitor721 Drawn" << std::endl;

	Mesh Charmy_Wing = loadMeshFile("../models/Charmy_Wing.obj");
	//drawMesh(imageBuffer, zBuffer, w9a02_monitor721, m9_monitor721_kd1, monitor721_TexWidth, monitor721_TexHeight, MeshTransform, worldToClip, lights, width, height);
	drawMesh(imageBuffer, zBuffer, Charmy_Wing, chr_charmy_skin_abd, chr_charmy_skin_abd_TexWidth, chr_charmy_skin_abd_TexHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, EMISSIVE, camWorldPos, MeshTransform, worldToCamera, projection, lights, width, height);
	std::cout << "Mesh Charmy_Wing Drawn" << std::endl;

	Mesh Charmy_Lenz = loadMeshFile("../models/Charmy_Lenz.obj");
	std::vector<uint8_t> chr_charmy_lenz_abd;
	unsigned int chr_charmy_lenz_abd_TexWidth, chr_charmy_lenz_abd_TexHeight;
	lodepng::decode(chr_charmy_lenz_abd, chr_charmy_lenz_abd_TexWidth, chr_charmy_lenz_abd_TexHeight, "../textures/chr_charmy_lenz_abd.png");
	//drawMesh(imageBuffer, zBuffer, w9a02_monitor721, m9_monitor721_kd1, monitor721_TexWidth, monitor721_TexHeight, MeshTransform, worldToClip, lights, width, height);
	drawMesh(imageBuffer, zBuffer, Charmy_Lenz, chr_charmy_lenz_abd, chr_charmy_lenz_abd_TexWidth, chr_charmy_lenz_abd_TexHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, EMISSIVE, camWorldPos, MeshTransform, worldToCamera, projection, lights, width, height);
	std::cout << "Mesh Charmy_Lenz Drawn" << std::endl;

	std::cout << "All Meshes Drawn " << std::endl;

	// For debug - draw point lights as colored circles so we can see where they are
	drawPointLights(imageBuffer, width, height, lights);
	std::cout << "Lights Drawn " << std::endl;

	// Save the image to png.
	int errorCode;
	errorCode = lodepng::encode(outputFilename, imageBuffer, width, height);
	if (errorCode) { // check the error code, in case an error occurred.
		std::cout << "lodepng error encoding image: " << lodepng_error_text(errorCode) << std::endl;
		return errorCode;
	}

	saveZBufferImage("zBuffer.png", zBuffer, width, height);

}

int main()
{
	drawScene("output.png", ShadingMode::PHONG, 100.f);

	//return 0;
}
