#include <Eigen/Dense>
#include <lodepng.h>
#include <json/json.hpp>
#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include "BVHNode.hpp"
#include "Triangle.hpp"
#include "Scene.hpp"
#include "Camera.hpp"
#include "PointLight.hpp"
#include "DirectionalLight.hpp"
#include "LambertianShader.hpp"
#include "TexturedLambertianShader.hpp"
#include "EmissionShader.hpp"
#include "PhongShader.hpp"
#include "MirrorShader.hpp"
#include "TexCoordTestShader.hpp"
#include "Model.hpp"
#include <fstream>

/// <summary>
/// Load a JSON config file using the nlohmann library.
/// </summary>
nlohmann::json loadConfig(const std::string& filename)
{
	std::ifstream configStream(filename);
	nlohmann::json config = nlohmann::json::parse(configStream);
	return config;
}

/// <summary>
/// Load an Eigen Vector3f from a config file.
/// Call as for example loadVec3FromConfig(config["myVector3"]);
/// </summary>
Eigen::Vector3f loadVec3FromConfig(const nlohmann::json& config)
{
	return Eigen::Vector3f(config[0], config[1], config[2]);
}

int main(int argc, char* argv[]) {

	// *** Load the config file ***
	auto config = loadConfig("../config/config.json");

	const int pixHeight = config["pixHeight"], pixWidth = config["pixWidth"];
	const int nChannels = 4;

	// *** Set up camera and output image ***
	Camera cam(
		loadVec3FromConfig(config["cameraPos"]),
		loadVec3FromConfig(config["cameraForward"]),
		loadVec3FromConfig(config["cameraUp"]),
		pixWidth, pixHeight,
		config["cameraFov"]);


	std::vector<uint8_t> outImage(pixHeight * pixWidth * nChannels);

	Eigen::Vector3f
		red(1.f, 0.f, 0.f),
		blue(0.f, 0.f, 1.f),
		aqua(0.f, .8f, .8f),
		cyan(0.f, 1.0f, 1.0f),
		lavender(178.f / 255.f, 164.f / 255.f, 212.f / 255.f);



	LambertianShader redLambertianShader(red);
	PhongShader PlasticShader(aqua, Eigen::Vector3f(1.f, 1.f, 1.f), 100.f);
	LambertianShader aquaLambertianShader(aqua);
	LambertianShader lavenderLambertianShader(lavender);
	MirrorShader mirrorShader;
	TexCoordTestShader texCoordTestShader;
	EmissionShader emissionShader(nullptr, 0, 0);

	// *** Set up scene ***
	Scene scene;

	// Optional code: here's how to add the spot mesh to the scene, using a BVH
	// Try enabling this and comparing it to the non-BVH version below!
		// *** Load shaders and textures ***

	std::vector<uint8_t> BurstTexture;
	unsigned int BurstWidth, BurstHeight;
	lodepng::decode(BurstTexture, BurstWidth, BurstHeight, "../textures/ggt_br1_body_abd.png");
	TexturedLambertianShader BurstShader(&BurstTexture, BurstWidth, BurstHeight); //diffuse texture only
	Model BurstModel("../models/Burst.obj");
	//scene.renderables.push_back(std::make_shared<BVHNode>(BurstModel, &BurstShader, 4, rotateY(M_PI))); // model, 

	std::vector<uint8_t> TestTexture;
	unsigned int TestWidth, TestHeight;
	lodepng::decode(TestTexture, TestWidth, TestHeight, "../textures/w3_brick201_abd.PNG");
	TexturedLambertianShader TestShader(&TestTexture, TestWidth, TestHeight); //diffuse texture only
	Model TestModel("../models/untitled.obj");
	//scene.renderables.push_back(std::make_shared<BVHNode>(TestModel, &TestShader, 0, rotateY(M_PI)));

		// Here's how to add the mesh without using the BVH.
	// Try comparing performance to the BVH version above.
	Model spotModel("../models/untitled.obj");
	//scene.renderables.push_back(std::make_shared<Mesh>(&spotShader, &spotModel));
	//scene.renderables.back()->modelToWorld(rotateY(M_PI / 4.0f));

	////////////////////////////////////////////////	w9a02		////////////////////////////////////////////////////////////

	std::vector<uint8_t> w3_brick001_abd;
	unsigned int w3_brick001_abd_Width, w3_brick001_abd_Height;
	lodepng::decode(w3_brick001_abd, w3_brick001_abd_Width, w3_brick001_abd_Height, "../textures/w3_brick001_abd.PNG");
	TexturedLambertianShader w9a02_brick02_Shader(&w3_brick001_abd, w3_brick001_abd_Width, w3_brick001_abd_Height); //diffuse texture only
	Model w9a02_brick02("../models/w9a02_brick02.obj");
	scene.renderables.push_back(std::make_shared<BVHNode>(w9a02_brick02, &w9a02_brick02_Shader, 0, rotateY(M_PI)));

	std::vector<uint8_t> w3_concrete003_abd;
	unsigned int w3_concrete003_abd_Width, w3_concrete003_abd_Height;
	lodepng::decode(w3_concrete003_abd, w3_concrete003_abd_Width, w3_concrete003_abd_Height, "../textures/w3_concrete003_abd.PNG");
	TexturedLambertianShader w9a02_concrete03_Shader(&w3_concrete003_abd, w3_concrete003_abd_Width, w3_concrete003_abd_Height); //diffuse texture only
	Model w9a02_concrete03("../models/w9a02_concrete03.obj");
	scene.renderables.push_back(std::make_shared<BVHNode>(w9a02_concrete03, &w9a02_concrete03_Shader, 0, rotateY(M_PI)));

	std::vector<uint8_t> w3_wall001_abd;
	unsigned int w3_wall001_abd_Width, w3_wall001_abd_Height;
	lodepng::decode(w3_wall001_abd, w3_wall001_abd_Width, w3_wall001_abd_Height, "../textures/w3_wall001_abd.PNG");
	TexturedLambertianShader w9a02_debris03_Shader(&w3_wall001_abd, w3_wall001_abd_Width, w3_wall001_abd_Height); //diffuse texture only
	Model w9a02_debris03("../models/w9a02_debris03.obj");
	scene.renderables.push_back(std::make_shared<BVHNode>(w9a02_debris03, &w9a02_debris03_Shader, 0, rotateY(M_PI)));

	std::vector<uint8_t> w3_metal405_abd;
	unsigned int w3_metal405_abd_Width, w3_metal405_abd_Height;
	lodepng::decode(w3_metal405_abd, w3_metal405_abd_Width, w3_metal405_abd_Height, "../textures/w3_metal405_abd.PNG");
	TexturedLambertianShader w9a02_drum01_Shader(&w3_metal405_abd, w3_metal405_abd_Width, w3_metal405_abd_Height); //diffuse texture only
	Model w9a02_drum01("../models/w9a02_drum01.obj");
	scene.renderables.push_back(std::make_shared<BVHNode>(w9a02_drum01, &w9a02_drum01_Shader, 0, rotateY(M_PI)));

	std::vector<uint8_t> w3_wood304_abd;
	unsigned int w3_wood304_abd_Width, w3_wood304_abd_Height;
	lodepng::decode(w3_wood304_abd, w3_wood304_abd_Width, w3_wood304_abd_Height, "../textures/w3_wood304_abd.PNG");
	TexturedLambertianShader w9a02_fy1_wood04_Shader(&w3_wood304_abd, w3_wood304_abd_Width, w3_wood304_abd_Height); //diffuse texture only
	Model w9a02_fy1_wood04("../models/w9a02_fy1_wood04.obj");
	scene.renderables.push_back(std::make_shared<BVHNode>(w9a02_fy1_wood04, &w9a02_fy1_wood04_Shader, 0, rotateY(M_PI)));

	std::vector<uint8_t> w3_iron402_abd;
	unsigned int w3_iron402_abd_Width, w3_iron402_abd_Height;
	lodepng::decode(w3_iron402_abd, w3_iron402_abd_Width, w3_iron402_abd_Height, "../textures/w3_iron402_abd.PNG");
	TexturedLambertianShader w9a02_iron02_Shader(&w3_iron402_abd, w3_iron402_abd_Width, w3_iron402_abd_Height); //diffuse texture only
	Model w9a02_iron02("../models/w9a02_iron02.obj");
	scene.renderables.push_back(std::make_shared<BVHNode>(w9a02_iron02, &w9a02_iron02_Shader, 0, rotateY(M_PI)));

	std::vector<uint8_t> w3_brick201_abd;
	unsigned int w3_brick201_abd_Width, w3_brick201_abd_Height;
	lodepng::decode(w3_brick201_abd, w3_brick201_abd_Width, w3_brick201_abd_Height, "../textures/w3_brick201_abd.PNG");
	TexturedLambertianShader w9a02_km1_brick01_Shader(&w3_brick201_abd, w3_brick201_abd_Width, w3_brick201_abd_Height); //diffuse texture only
	Model w9a02_km1_brick01("../models/w9a02_km1_brick01.obj");
	scene.renderables.push_back(std::make_shared<BVHNode>(w9a02_km1_brick01, &w9a02_km1_brick01_Shader, 0, rotateY(M_PI)));

	std::vector<uint8_t> w3_concrete201_abd;
	unsigned int w3_concrete201_abd_Width, w3_concrete201_abd_Height;
	lodepng::decode(w3_concrete201_abd, w3_concrete201_abd_Width, w3_concrete201_abd_Height, "../textures/w3_concrete201_abd.PNG");
	TexturedLambertianShader w9a02_km1_concrete01_Shader(&w3_concrete201_abd, w3_concrete201_abd_Width, w3_concrete201_abd_Height); //diffuse texture only
	Model w9a02_km1_concrete01("../models/w9a02_km1_concrete01.obj");
	scene.renderables.push_back(std::make_shared<BVHNode>(w9a02_km1_concrete01, &w9a02_km1_concrete01_Shader, 0, rotateY(M_PI)));

	std::vector<uint8_t> w3_wall201_abd;
	unsigned int w3_wall201_abd_Width, w3_wall201_abd_Height;
	lodepng::decode(w3_wall201_abd, w3_wall201_abd_Width, w3_wall201_abd_Height, "../textures/w3_wall201_abd.PNG");
	TexturedLambertianShader w9a02_km1_wall01_Shader(&w3_wall201_abd, w3_wall201_abd_Width, w3_wall201_abd_Height); //diffuse texture only
	Model w9a02_km1_wall01("../models/w9a02_km1_wall01.obj");
	scene.renderables.push_back(std::make_shared<BVHNode>(w9a02_km1_wall01, &w9a02_km1_wall01_Shader, 0, rotateY(M_PI)));

	std::vector<uint8_t> w3_metal102_abd;
	unsigned int w3_metal102_abd_Width, w3_metal102_abd_Height;
	lodepng::decode(w3_metal102_abd, w3_metal102_abd_Width, w3_metal102_abd_Height, "../textures/w3_metal102_abd.PNG");
	TexturedLambertianShader w9a02_metal02_Shader(&w3_metal102_abd, w3_metal102_abd_Width, w3_metal102_abd_Height); //diffuse texture only
	Model w9a02_metal02("../models/w9a02_metal02.obj");
	scene.renderables.push_back(std::make_shared<BVHNode>(w9a02_metal02, &w9a02_metal02_Shader, 0, rotateY(M_PI)));

	std::vector<uint8_t> w3_metal104_abd;
	unsigned int w3_metal104_abd_Width, w3_metal104_abd_Height;
	lodepng::decode(w3_metal104_abd, w3_metal104_abd_Width, w3_metal104_abd_Height, "../textures/w3_metal104_abd.PNG");
	TexturedLambertianShader w9a02_metal05_Shader(&w3_metal104_abd, w3_metal104_abd_Width, w3_metal104_abd_Height); //diffuse texture only
	Model w9a02_metal05("../models/w9a02_metal05.obj");
	scene.renderables.push_back(std::make_shared<BVHNode>(w9a02_metal05, &w9a02_metal05_Shader, 0, rotateY(M_PI)));

	std::vector<uint8_t> w9_metal101_abd;
	unsigned int w9_metal101_abd_Width, w9_metal101_abd_Height;
	lodepng::decode(w9_metal101_abd, w9_metal101_abd_Width, w9_metal101_abd_Height, "../textures/w9_metal101_abd.PNG");
	TexturedLambertianShader w9a02_metal101_kd1_Shader(&w9_metal101_abd, w9_metal101_abd_Width, w9_metal101_abd_Height); //diffuse texture only
	Model w9a02_metal101_kd1("../models/w9a02_metal101_kd1.obj");
	scene.renderables.push_back(std::make_shared<BVHNode>(w9a02_metal101_kd1, &w9a02_metal101_kd1_Shader, 0, rotateY(M_PI)));

	std::vector<uint8_t> w9_metal151_abd;
	unsigned int w9_metal151_abd_Width, w9_metal151_abd_Height;
	lodepng::decode(w9_metal151_abd, w9_metal151_abd_Width, w9_metal151_abd_Height, "../textures/w9_metal151_abd.PNG");
	TexturedLambertianShader w9a02_metal151_Shader(&w9_metal151_abd, w9_metal151_abd_Width, w9_metal151_abd_Height); //diffuse texture only
	Model w9a02_metal151("../models/w9a02_metal151.obj");
	scene.renderables.push_back(std::make_shared<BVHNode>(w9a02_metal151, &w9a02_metal151_Shader, 0, rotateY(M_PI)));

	std::vector<uint8_t> w9_metal154_abd;
	unsigned int w9_metal154_abd_Width, w9_metal154_abd_Height;
	lodepng::decode(w9_metal154_abd, w9_metal154_abd_Width, w9_metal154_abd_Height, "../textures/w9_metal154_abd.PNG");
	TexturedLambertianShader w9a02_metal154_Shader(&w9_metal154_abd, w9_metal154_abd_Width, w9_metal154_abd_Height); //diffuse texture only
	Model w9a02_metal154("../models/w9a02_metal154.obj");
	scene.renderables.push_back(std::make_shared<BVHNode>(w9a02_metal154, &w9a02_metal154_Shader, 0, rotateY(M_PI)));

	std::vector<uint8_t> w9_metal159_abd;
	unsigned int w9_metal159_abd_Width, w9_metal159_abd_Height;
	lodepng::decode(w9_metal159_abd, w9_metal159_abd_Width, w9_metal159_abd_Height, "../textures/w9_metal159_abd.PNG");
	TexturedLambertianShader w9a02_metal159_Shader(&w9_metal159_abd, w9_metal159_abd_Width, w9_metal159_abd_Height); //diffuse texture only
	Model w9a02_metal159("../models/w9a02_metal159.obj");
	scene.renderables.push_back(std::make_shared<BVHNode>(w9a02_metal159, &w9a02_metal159_Shader, 0, rotateY(M_PI)));

	std::vector<uint8_t> w2_metal709_abd;
	unsigned int w2_metal709_abd_Width, w2_metal709_abd_Height;
	lodepng::decode(w2_metal709_abd, w2_metal709_abd_Width, w2_metal709_abd_Height, "../textures/w2_metal709_abd.PNG");
	TexturedLambertianShader w9a02_metal709_Shader(&w2_metal709_abd, w2_metal709_abd_Width, w2_metal709_abd_Height); //diffuse texture only
	Model w9a02_metal709("../models/w9a02_metal709.obj");
	//scene.renderables.push_back(std::make_shared<BVHNode>(w9a02_metal709, &w9a02_metal709_Shader, 0, rotateY(M_PI)));

	std::vector<uint8_t> w9_metal854_abd;
	unsigned int w9_metal854_abd_Width, w9_metal854_abd_Height;
	lodepng::decode(w9_metal854_abd, w9_metal854_abd_Width, w9_metal854_abd_Height, "../textures/w9_metal854_abd.PNG");
	TexturedLambertianShader w9a02_metal854_Shader(&w9_metal854_abd, w9_metal854_abd_Width, w9_metal854_abd_Height); //diffuse texture only
	Model w9a02_metal854("../models/w9a02_metal854.obj");
	//scene.renderables.push_back(std::make_shared<BVHNode>(w9a02_metal854, &w9a02_metal854_Shader, 0, rotateY(M_PI)));

	std::vector<uint8_t> w9_metal901_abd;
	unsigned int w9_metal901_abd_Width, w9_metal901_abd_Height;
	lodepng::decode(w9_metal901_abd, w9_metal901_abd_Width, w9_metal901_abd_Height, "../textures/w9_metal901_abd.PNG");
	TexturedLambertianShader w9a02_metal901_Shader(&w9_metal901_abd, w9_metal901_abd_Width, w9_metal901_abd_Height); //diffuse texture only
	Model w9a02_metal901("../models/w9a02_metal901.obj");
	scene.renderables.push_back(std::make_shared<BVHNode>(w9a02_metal901, &w9a02_metal901_Shader, 0, rotateY(M_PI)));

	std::vector<uint8_t> w9_metal903_abd;
	unsigned int w9_metal903_abd_Width, w9_metal903_abd_Height;
	lodepng::decode(w9_metal903_abd, w9_metal903_abd_Width, w9_metal903_abd_Height, "../textures/w9_metal903_abd.PNG");
	TexturedLambertianShader w9a02_metal903_Shader(&w9_metal903_abd, w9_metal903_abd_Width, w9_metal903_abd_Height); //diffuse texture only
	Model w9a02_metal903("../models/w9a02_metal903.obj");
	//scene.renderables.push_back(std::make_shared<BVHNode>(w9a02_metal903, &w9a02_metal903_Shader, 0, rotateY(M_PI)));

	std::vector<uint8_t> w9_metal909_abd;
	unsigned int w9_metal909_abd_Width, w9_metal909_abd_Height;
	lodepng::decode(w9_metal909_abd, w9_metal909_abd_Width, w9_metal909_abd_Height, "../textures/w9_metal909_abd.PNG");
	TexturedLambertianShader w9a02_metal909_Shader(&w9_metal909_abd, w9_metal909_abd_Width, w9_metal909_abd_Height); //diffuse texture only
	Model w9a02_metal909("../models/w9a02_metal909.obj");
	//scene.renderables.push_back(std::make_shared<BVHNode>(w9a02_metal909, &w9a02_metal909_Shader, 0, rotateY(M_PI)));

	std::vector<uint8_t> w9_metal913_abd;
	unsigned int w9_metal913_abd_Width, w9_metal913_abd_Height;
	lodepng::decode(w9_metal913_abd, w9_metal913_abd_Width, w9_metal913_abd_Height, "../textures/w9_metal913_abd.PNG");
	TexturedLambertianShader w9a02_metal913mc_Shader(&w9_metal913_abd, w9_metal913_abd_Width, w9_metal913_abd_Height); //diffuse texture only
	Model w9a02_metal913mc("../models/w9a02_metal913mc.obj");
	//scene.renderables.push_back(std::make_shared<BVHNode>(w9a02_metal913mc, &w9a02_metal913mc_Shader, 0, rotateY(M_PI)));

	std::vector<uint8_t> w3_metal402_abd;
	unsigned int w3_metal402_abd_Width, w3_metal402_abd_Height;
	lodepng::decode(w3_metal402_abd, w3_metal402_abd_Width, w3_metal402_abd_Height, "../textures/w3_metal402_abd.PNG");
	TexturedLambertianShader w9a02_metalnail001_Shader(&w3_metal402_abd, w3_metal402_abd_Width, w3_metal402_abd_Height); //diffuse texture only
	Model w9a02_metalnail001("../models/w9a02_metalnail001.obj");
	//scene.renderables.push_back(std::make_shared<BVHNode>(w9a02_metalnail001, &w9a02_metalnail001_Shader, 0, rotateY(M_PI)));

	std::vector<uint8_t> w9_metal702_abd;
	unsigned int w9_metal702_abd_Width, w9_metal702_abd_Height;
	lodepng::decode(w9_metal702_abd, w9_metal702_abd_Width, w9_metal702_abd_Height, "../textures/w9_metal702_abd.PNG");
	TexturedLambertianShader w9a02_metalwall701_Shader(&w9_metal702_abd, w9_metal702_abd_Width, w9_metal702_abd_Height); //diffuse texture only
	Model w9a02_metalwall701("../models/w9a02_metalwall701.obj");
	scene.renderables.push_back(std::make_shared<BVHNode>(w9a02_metalwall701, &w9a02_metalwall701_Shader, 0, rotateY(M_PI)));

	std::vector<uint8_t> w9_metal703_abd;
	unsigned int w9_metal703_abd_Width, w9_metal703_abd_Height;
	lodepng::decode(w9_metal703_abd, w9_metal703_abd_Width, w9_metal703_abd_Height, "../textures/w9_metal703_abd.PNG");
	TexturedLambertianShader w9a02_metalwall703_Shader(&w9_metal703_abd, w9_metal703_abd_Width, w9_metal703_abd_Height); //diffuse texture only
	Model w9a02_metalwall703("../models/w9a02_metalwall703.obj");
	//scene.renderables.push_back(std::make_shared<BVHNode>(w9a02_metalwall703, &w9a02_metalwall703_Shader, 0, rotateY(M_PI)));

	std::vector<uint8_t> w9_metal705_abd;
	unsigned int w9_metal705_abd_Width, w9_metal705_abd_Height;
	lodepng::decode(w9_metal705_abd, w9_metal705_abd_Width, w9_metal705_abd_Height, "../textures/w9_metal705_abd.PNG");
	TexturedLambertianShader w9a02_metalwall705_Shader(&w9_metal705_abd, w9_metal705_abd_Width, w9_metal705_abd_Height); //diffuse texture only
	Model w9a02_metalwall705("../models/w9a02_metalwall705.obj");
	//scene.renderables.push_back(std::make_shared<BVHNode>(w9a02_metalwall705, &w9a02_metalwall705_Shader, 0, rotateY(M_PI)));

	std::vector<uint8_t> w9_monitor720_abd;
	unsigned int w9_monitor720_abd_Width, w9_monitor720_abd_Height;
	lodepng::decode(w9_monitor720_abd, w9_monitor720_abd_Width, w9_monitor720_abd_Height, "../textures/w9_monitor720_abd.PNG");
	TexturedLambertianShader w9a02_monitor721_Shader(&w9_monitor720_abd, w9_monitor720_abd_Width, w9_monitor720_abd_Height); //diffuse texture only
	Model w9a02_monitor721("../models/w9a02_monitor721.obj");
	scene.renderables.push_back(std::make_shared<BVHNode>(w9a02_monitor721, &w9a02_monitor721_Shader, 0, rotateY(M_PI)));


	Model w9a02_my1_brick01("../models/w9a02_my1_brick01.obj");
	//scene.renderables.push_back(std::make_shared<BVHNode>(w9a02_my1_brick01, &w9a02_brick02, 0, rotateY(M_PI)));

	std::vector<uint8_t> w3_concrete001_abd;
	unsigned int w3_concrete001_abd_Width, w3_concrete001_abd_Height;
	lodepng::decode(w3_concrete001_abd, w3_concrete001_abd_Width, w3_concrete001_abd_Height, "../textures/w3_concrete001_abd.PNG");
	TexturedLambertianShader w9a02_my1_concrete01_Shader(&w3_concrete001_abd, w3_concrete001_abd_Width, w3_concrete001_abd_Height); //diffuse texture only
	Model w9a02_my1_concrete01("../models/w9a02_my1_concrete01.obj");
	//scene.renderables.push_back(std::make_shared<BVHNode>(w9a02_my1_concrete01, &w9a02_my1_concrete01_Shader, 0, rotateY(M_PI)));

	std::vector<uint8_t> w3_iron001_abd;
	unsigned int w3_iron001_abd_Width, w3_iron001_abd_Height;
	lodepng::decode(w3_iron001_abd, w3_iron001_abd_Width, w3_iron001_abd_Height, "../textures/w3_iron001_abd.PNG");
	TexturedLambertianShader w9a02_my1_iron01_Shader(&w3_iron001_abd, w3_iron001_abd_Width, w3_iron001_abd_Height); //diffuse texture only
	Model w9a02_my1_iron01("../models/w9a02_my1_iron01.obj");
	//scene.renderables.push_back(std::make_shared<BVHNode>(w9a02_my1_iron01, &w9a02_my1_iron01_Shader, 0, rotateY(M_PI)));

	
	Model w9a02_my1_wall01("../models/w9a02_my1_wall01.obj");
	//scene.renderables.push_back(std::make_shared<BVHNode>(w9a02_my1_wall01, &w9a02_debris03_Shader, 0, rotateY(M_PI)));

	std::vector<uint8_t> w9_neon723_ems;
	unsigned int w9_neon723_ems_Width, w9_neon723_ems_Height;
	lodepng::decode(w9_neon723_ems, w9_neon723_ems_Width, w9_neon723_ems_Height, "../textures/w9_neon723_ems.PNG");
	EmissionShader w9a02_neon745_Shader(&w9_neon723_ems, w9_neon723_ems_Width, w9_neon723_ems_Height); //diffuse texture only
	Model w9a02_neon745("../models/w9a02_neon745.obj");
	scene.renderables.push_back(std::make_shared<BVHNode>(w9a02_neon745, &w9a02_neon745_Shader, 0, rotateY(M_PI)));

	Model w9a02_neon746("../models/w9a02_neon746.obj");
	//scene.renderables.push_back(std::make_shared<BVHNode>(w9a02_neon746, &w9a02_neon745_Shader, 0, rotateY(M_PI)));

	Model w9a02_neon747("../models/w9a02_neon747.obj");
	//scene.renderables.push_back(std::make_shared<BVHNode>(w9a02_neon747, &w9a02_neon745_Shader, 0, rotateY(M_PI)));

	Model w9a02_neon748("../models/w9a02_neon748.obj");
	//scene.renderables.push_back(std::make_shared<BVHNode>(w9a02_neon748, &w9a02_neon745_Shader, 0, rotateY(M_PI)));

	std::vector<uint8_t> w9_pipe901_abd;
	unsigned int w9_pipe901_abd_Width, w9_pipe901_abd_Height;
	lodepng::decode(w9_pipe901_abd, w9_pipe901_abd_Width, w9_pipe901_abd_Height, "../textures/w9_pipe901_abd.PNG");
	TexturedLambertianShader w9a02_pipe901_Shader(&w9_pipe901_abd, w9_pipe901_abd_Width, w9_pipe901_abd_Height); //diffuse texture only
	Model w9a02_pipe901("../models/w9a02_pipe901.obj");
	//scene.renderables.push_back(std::make_shared<BVHNode>(w9a02_pipe901, &w9a02_pipe901_Shader, 0, rotateY(M_PI)));

	std::vector<uint8_t> w3_wall203_abd;
	unsigned int w3_wall203_abd_Width, w3_wall203_abd_Height;
	lodepng::decode(w3_wall203_abd, w3_wall203_abd_Width, w3_wall203_abd_Height, "../textures/w3_wall203_abd.PNG");
	TexturedLambertianShader w9a02_wall03_Shader(&w3_wall203_abd, w3_wall203_abd_Width, w3_wall203_abd_Height); //diffuse texture only
	Model w9a02_wall03("../models/w9a02_wall03.obj");
	//scene.renderables.push_back(std::make_shared<BVHNode>(w9a02_wall03, &w9a02_wall03_Shader, 0, rotateY(M_PI)));

	std::vector<uint8_t> w3_wall205_abd;
	unsigned int w3_wall205_abd_Width, w3_wall205_abd_Height;
	lodepng::decode(w3_wall205_abd, w3_wall205_abd_Width, w3_wall205_abd_Height, "../textures/w3_wall205_abd.PNG");
	TexturedLambertianShader w9a02_wall05_Shader(&w3_wall205_abd, w3_wall205_abd_Width, w3_wall205_abd_Height); //diffuse texture only
	Model w9a02_wall05("../models/w9a02_wall05.obj");
	//scene.renderables.push_back(std::make_shared<BVHNode>(w9a02_wall05, &w9a02_wall05_Shader, 0, rotateY(M_PI)));

	std::vector<uint8_t> w3_woodburn401_abd;
	unsigned int w3_woodburn401_abd_Width, w3_woodburn401_abd_Height;
	lodepng::decode(w3_woodburn401_abd, w3_woodburn401_abd_Width, w3_woodburn401_abd_Height, "../textures/w3_woodburn401_abd.PNG");
	TexturedLambertianShader w9a02_wood01_Shader(&w3_woodburn401_abd, w3_woodburn401_abd_Width, w3_woodburn401_abd_Height); //diffuse texture only
	Model w9a02_wood01("../models/w9a02_wood01.obj");
	//scene.renderables.push_back(std::make_shared<BVHNode>(w9a02_wood01, &w9a02_wood01_Shader, 0, rotateY(M_PI)));

	std::vector<uint8_t> w3_wood005_abd;
	unsigned int w3_wood005_abd_Width, w3_wood005_abd_Height;
	lodepng::decode(w3_wood005_abd, w3_wood005_abd_Width, w3_wood005_abd_Height, "../textures/w3_wood005_abd.PNG");
	TexturedLambertianShader w9a02_wood05_Shader(&w3_wood005_abd, w3_wood005_abd_Width, w3_wood005_abd_Height); //diffuse texture only
	Model w9a02_wood05("../models/w9a02_wood05.obj");
	//scene.renderables.push_back(std::make_shared<BVHNode>(w9a02_wood05, &w9a02_wood05_Shader, 0, rotateY(M_PI)));

	////////////////////////////////////////////////	CONSOLE		////////////////////////////////////////////////////////////
	 
	std::vector<uint8_t> w9_glass740_abd_a;
	unsigned int w9_glass740_abd_a_Width, w9_glass740_abd_a_Height;
	lodepng::decode(w9_glass740_abd_a, w9_glass740_abd_a_Width, w9_glass740_abd_a_Height, "../textures/w9_glass740_abd_a.PNG");
	EmissionShader ev_obj_w9_hideconsole_glass740_Shader(&w9_glass740_abd_a, w9_glass740_abd_a_Width, w9_glass740_abd_a_Height); //diffuse texture only
	Model ev_obj_w9_hideconsole_glass740("../models/ev_obj_w9_hideconsole_glass740.obj");
	scene.renderables.push_back(std::make_shared<BVHNode>(ev_obj_w9_hideconsole_glass740, &PlasticShader, 0, rotateY(M_PI)));

	
	Model ev_obj_w9_hideconsole_metal901("../models/ev_obj_w9_hideconsole_metal901.obj");
	scene.renderables.push_back(std::make_shared<BVHNode>(ev_obj_w9_hideconsole_metal901, &w9a02_metal901_Shader, 0, rotateY(M_PI)));

	Model ev_obj_w9_hideconsole_metalwall702("../models/ev_obj_w9_hideconsole_metalwall702.obj");
	scene.renderables.push_back(std::make_shared<BVHNode>(ev_obj_w9_hideconsole_metalwall702, &w9a02_metalwall701_Shader, 0, rotateY(M_PI)));

	Model ev_obj_w9_hideconsole_monitor721("../models/ev_obj_w9_hideconsole_monitor721.obj");
	scene.renderables.push_back(std::make_shared<BVHNode>(ev_obj_w9_hideconsole_monitor721, &w9a02_monitor721_Shader, 0, rotateY(M_PI)));
	
	////////////////////////////////////////////////	DOORS		////////////////////////////////////////////////////////////
	
	Model ev_obj_w9_hidedoors_metal154("../models/ev_obj_w9_hidedoors_metal154.obj");	
	scene.renderables.push_back(std::make_shared<BVHNode>(ev_obj_w9_hidedoors_metal154, &w9a02_metal154_Shader, 0, rotateY(M_PI)));

	std::vector<uint8_t> w9_metal701_abd;
	unsigned int w9_metal701_abd_Width, w9_metal701_abd_Height;
	lodepng::decode(w9_metal701_abd, w9_metal701_abd_Width, w9_metal701_abd_Height, "../textures/w9_metal701_abd.PNG");
	TexturedLambertianShader ev_obj_w9_hidedoors_metal701_Shader(&w9_metal701_abd, w9_metal701_abd_Width, w9_metal701_abd_Height); //diffuse texture only
	Model ev_obj_w9_hidedoors_metal701("../models/ev_obj_w9_hidedoors_metal701.obj");
	scene.renderables.push_back(std::make_shared<BVHNode>(ev_obj_w9_hidedoors_metal701, &ev_obj_w9_hidedoors_metal701_Shader, 0, rotateY(M_PI)));


	std::vector<uint8_t> w9_metal744_abd;
	unsigned int w9_metal744_abd_Width, w9_metal744_abd_Height;
	lodepng::decode(w9_metal744_abd, w9_metal744_abd_Width, w9_metal744_abd_Height, "../textures/w9_metal744_abd.PNG");
	TexturedLambertianShader ev_obj_w9_hidedoors_metal702_Shader(&w9_metal744_abd, w9_metal744_abd_Width, w9_metal744_abd_Height); //diffuse texture only
	Model ev_obj_w9_hidedoors_metal702("../models/ev_obj_w9_hidedoors_metal702.obj");
	scene.renderables.push_back(std::make_shared<BVHNode>(ev_obj_w9_hidedoors_metal702, &ev_obj_w9_hidedoors_metal702_Shader, 0, rotateY(M_PI)));

	
	////////////////////////////////////////////////	MONITOR		////////////////////////////////////////////////////////////
	
	Model ev_obj_w9_hidemonitor_metal154("../models/ev_obj_w9_hidemonitor_metal154.obj");
	scene.renderables.push_back(std::make_shared<BVHNode>(ev_obj_w9_hidemonitor_metal154, &w9a02_metal154_Shader, 0, rotateY(M_PI)));

	Model ev_obj_w9_hidemonitor_movtex("../models/ev_obj_w9_hidemonitor_movtex.obj");
	scene.renderables.push_back(std::make_shared<BVHNode>(ev_obj_w9_hidemonitor_movtex, &w9a02_monitor721_Shader, 0, rotateY(M_PI)));

	
	////////////////////////////////////////////////	AMY	    	////////////////////////////////////////////////////////////
	////////////////////////////////////////////////	KNUCKLES	////////////////////////////////////////////////////////////
	////////////////////////////////////////////////	SILVER		////////////////////////////////////////////////////////////
	////////////////////////////////////////////////	CHARMY		////////////////////////////////////////////////////////////
	////////////////////////////////////////////////	ESPIO		////////////////////////////////////////////////////////////
	////////////////////////////////////////////////	VECTOR		////////////////////////////////////////////////////////////


	////////////////////////////////////////////////	LIGHTS		////////////////////////////////////////////////////////////
	// *** Add lights to scene ***
	Eigen::Vector3f ambientLight(0.1f, 0.1f, 0.1f);

	std::vector<std::unique_ptr<Light>> lightSources;

	//RADIUS = 0, BLENDER X Y Z = -X Z Y
	lightSources.push_back(std::make_unique<PointLight>(Eigen::Vector3f(-3.407f, 6.018f, -2.217f), Eigen::Vector3f(0.491f, 0.423f, 0.141) * 30)); //position, intensity || rt_w9a02_s01_ceilingup__0013
	lightSources.push_back(std::make_unique<PointLight>(Eigen::Vector3f(3.0f, 6.018f, -2.217f), Eigen::Vector3f(0.491f, 0.423f, 0.141) * 30)); //position, intensity || rt_w9a02_s01_ceilingup__0014
	lightSources.push_back(std::make_unique<PointLight>(Eigen::Vector3f(3.0f, 6.018f, 2.217f), Eigen::Vector3f(0.491f, 0.423f, 0.141) * 30)); //position, intensity || rt_w9a02_s01_ceilingup__0015
	lightSources.push_back(std::make_unique<PointLight>(Eigen::Vector3f(-3.407f, 6.018f, 2.217f), Eigen::Vector3f(0.491f, 0.423f, 0.141) * 30)); //position, intensity || rt_w9a02_s01_ceilingup__0016

	lightSources.push_back(std::make_unique<PointLight>(Eigen::Vector3f(5.0f, 2.936f, 0.256f), Eigen::Vector3f(0.0f, 0.491f, 0.491) * 18)); //position, intensity || rt_w9a02_s01_monitor__0023

	lightSources.push_back(std::make_unique<PointLight>(Eigen::Vector3f(0.979f, 3.43f, 4.378f), Eigen::Vector3f(0.491f, 0.328f, 0.141f) * 12)); //position, intensity || rt_w9a02_s01_wall__0019
	lightSources.push_back(std::make_unique<PointLight>(Eigen::Vector3f(-5.703f, 3.43f, 4.378f), Eigen::Vector3f(0.491f, 0.328f, 0.141f) * 12)); //position, intensity || rt_w9a02_s01_wall__0020

	// *** Render the scene ***
	// Shuffling the scanline order gets better CPU usage between threads
	// when some lines take longer to render than others.
	std::vector<unsigned int> scanlines(pixHeight);
	for (int i = 0; i < pixHeight; ++i) scanlines[i] = i;

	if (config["shuffleScanlines"]) {
		std::random_device rd;
		std::mt19937 g(rd());
		std::shuffle(scanlines.begin(), scanlines.end(), g);
	}

	auto startTime = std::chrono::steady_clock::now();

	Ray ray = cam.getRay(531, 325);
	HitInfo hitInfo;
	scene.intersect(ray, 1e-6f, 1e6f, hitInfo, VISIBLE_BITMASK);
	float x = hitInfo.hitT;


	#pragma omp parallel for
	for (int y = 0; y < pixHeight; ++y) {
		for (int x = 0; x < pixWidth; ++x) {
			Ray ray = cam.getRay(x, scanlines[y]);
			HitInfo hitInfo;
			if (scene.intersect(ray, 1e-6f, 1e6f, hitInfo, VISIBLE_BITMASK)) {
				Eigen::Vector3f color = hitInfo.shader->getColor(
					hitInfo, &scene,
					lightSources, ambientLight,
					0, config["maxBounces"]);

				color.x() = std::min(color.x(), 1.f);
				color.y() = std::min(color.y(), 1.f);
				color.z() = std::min(color.z(), 1.f);


				int line = (pixHeight - scanlines[y]) - 1;
				outImage[(x + line * pixWidth) * nChannels + 0] = color.x() * 255;
				outImage[(x + line * pixWidth) * nChannels + 1] = color.y() * 255;
				outImage[(x + line * pixWidth) * nChannels + 2] = color.z() * 255;
				outImage[(x + line * pixWidth) * nChannels + 3] = 255;
			}
			else {
				int line = (pixHeight - scanlines[y]) - 1;
				outImage[(x + line * pixWidth) * nChannels + 0] = 0;
				outImage[(x + line * pixWidth) * nChannels + 1] = 0;
				outImage[(x + line * pixWidth) * nChannels + 2] = 0;
				outImage[(x + line * pixWidth) * nChannels + 3] = 255;
			}
		}
		if (omp_get_thread_num() == omp_get_num_threads()-1) {
			std::clog << "\rScanlines remaining: " << (pixHeight - y) << ' ' << std::flush;
		}

	}

	auto renderTime = std::chrono::steady_clock::now() - startTime;

	std::cout << "Render duration " << std::chrono::duration_cast<std::chrono::milliseconds>(renderTime).count() * 1e-3f << " seconds." << std::endl;

	// *** Save the output image ***
	int errorCode;
	errorCode = lodepng::encode(config["outputFilename"], outImage, pixWidth, pixHeight);
	if (errorCode) { // check the error code, in case an error occurred.
		std::cout << "lodepng error encoding image: " << lodepng_error_text(errorCode) << std::endl;
		return errorCode;
	}

	return 0;
}
