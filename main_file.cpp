#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stdlib.h>
#include <stdio.h>
#include "myCube.h"
#include "constants.h"
#include <iostream>
#include "lodepng.h"
#include "shaderprogram.h"
#include "allmodels.h"
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include "myTeapot.h"
#include <cstdlib>
#include <map>
#include <random>

enum Model {FISH2, FISH3, FISH4, FISH5, FISH6, FISH7, FISH8, FISH9, FISH10, FISH11};
enum Texture { TEX_FISH5, TEX_FISH6, TEX_FISH7, TEX_FISH8, TEX_FISH9, TEX_FISH10, TEX_FISH11, TEX_FISH12, TEX_FISH13, TEX_FISH14};

#define GLM_FORCE_RADIANS

#define ROCK1 0
#define ROCK2 1
#define ROCK3 2
#define ROCK4 3
#define ROCK5 4
#define ROCK6 5
#define PLANTS 6
#define FISH2 7
#define FISH3 8
#define FISH4 9
#define FISH5 10
#define FISH6 11
#define FISH7 12
#define FISH8 13
#define FISH9 14
#define FISH10 15
#define FISH11 16
#define KOSTKA 17

#define TEX_ROCK1 0
#define TEX_ROCK2 1 
#define TEX_ROCK3 2 
#define TEX_ROCK4 3 
#define TEX_ROCK5 4 
#define TEX_ROCK6 5 
#define TEX_BOTTOM 6
#define TEX_PLANT 7
#define TEX_FISH5 8
#define TEX_FISH6 9
#define TEX_FISH7 10
#define TEX_FISH8 11
#define TEX_FISH9 12
#define TEX_FISH10 13
#define TEX_FISH11 14
#define TEX_FISH12 15
#define TEX_FISH13 16
#define TEX_FISH14 17
#define TEX_BIALY 18

float x;
float speed = 1.5;

//Pozycja światła
glm::vec4 light1 = glm::vec4(-10, 6, 0, 1);
glm::vec4 light2 = glm::vec4(10, 6, 0, 1);

struct MyVertex {
	std::vector<glm::vec4> Vertices; // Wektor przechowujący wierzchołki
	std::vector<glm::vec4> Normals; // Wektor przechowujący wektory normalne
	std::vector<glm::vec2> TexCoords; // Wektor przechowujący współrzędne tekstur
	std::vector<unsigned int> Indices; // W jaki sposób łaczyć wierzchołki w trójkąty
};

std::vector<MyVertex> models; //tablica modeli
std::vector<GLuint> texs; //tablica tekstur

ShaderProgram* waterShader;
ShaderProgram* phongShader;
ShaderProgram* glassShader;

glm::vec3 cameraPos = glm::vec3(0.0f, 10.0f, 0.0f); //początowa pozycja kamery
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f); //początkowy kierunek kamery
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f); //oś poruszania się

float pitch = 0.0f; //obrót wokół osi X
float yaw = -90.0f; //obrót wokół osi Y
bool firstMouse = true; //czy ruszono myszką

//obsługa błędów
void error_callback(int error, const char* description) {
	fputs(description, stderr);
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	const float cameraSpeed = 0.25f;
	//Dodawanie/Odejmowanie wektora kierunkowego w zależności od kierunku jazdy
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		cameraPos += cameraSpeed * cameraFront;
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		cameraPos -= cameraSpeed * cameraFront;
	//Dodawanie/Odejmowanie wektora prostopadłego do kierunku i grawitacji w zależności od kierunku jazdy
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
	//Wyłączanie okna
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{	
	const float sensitivity = 0.1f;
	static float lastX = 300, lastY = 300;
	static bool firstMouse = true;

	//Jeśli ruszysz myszką zaktualizuj wartości xpos i ypos
	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	//Obliczanie różnicy pozycji myszy
	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos;
	lastX = xpos;
	lastY = ypos;

	//Kontrola czułości myszy
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	//Aktualizacja kątów obrotu kamery
	yaw += xoffset;
	pitch += yoffset;

	if (pitch > 89.0f)
		pitch = 89.0f;
	if (pitch < -89.0f)
		pitch = -89.0f;

	//obliczanie składowych wektora kierunku patrzenia kamery
	glm::vec3 front(
		cos(glm::radians(yaw)) * cos(glm::radians(pitch)),
		sin(glm::radians(pitch)),
		sin(glm::radians(yaw)) * cos(glm::radians(pitch))
	);
	cameraFront = glm::normalize(front);
}

//Funkcja do wgrywania modelu za pomocą biblioteki Assimp z wykładu
void loadModel(std::string filename, int model_i) {
	models.push_back(MyVertex());
	Assimp::Importer importer;

	const aiScene* scene = importer.ReadFile(filename,
		aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenSmoothNormals);

	std::cout << importer.GetErrorString() << std::endl;

	aiMesh* mesh = scene->mMeshes[0];
	for (int i = 0; i < mesh->mNumVertices; i++) {

		aiVector3D vertex = mesh->mVertices[i];
		models[model_i].Vertices.push_back(glm::vec4(vertex.x, vertex.y, vertex.z, 1));

		aiVector3D normal = mesh->mNormals[i];
		models[model_i].Normals.push_back(glm::vec4(normal.x, normal.y, normal.z, 0));

		unsigned int liczba_zest = mesh->GetNumUVChannels();
		unsigned int wymiar_wsp_tex = mesh->mNumUVComponents[0];

		aiVector3D texCoord = mesh->mTextureCoords[0][i];
		models[model_i].TexCoords.push_back(glm::vec2(texCoord.x, texCoord.y));
	}
	for (int i = 0; i < mesh->mNumFaces; i++) {
		aiFace& face = mesh->mFaces[i];
		for (int j = 0; j < face.mNumIndices; j++) {
			models[model_i].Indices.push_back(face.mIndices[j]);
		}
	}
}

//Funkcja do wgrywania tekstur z labów
void readTexture(const char* filename, int tex_i) {
	texs.push_back(0);
	GLuint tex;
	glActiveTexture(GL_TEXTURE0);
	std::vector<unsigned char> image;
	unsigned width, height;
	unsigned error = lodepng::decode(image, width, height, filename);
	if (error) std::cout << "decoder error " << error << ": " << lodepng_error_text(error) << std::endl;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, 4, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, (unsigned char*)image.data());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	texs[tex_i] = tex;
}

void initOpenGLProgram(GLFWwindow* window) {
	initShaders();
	glClearColor(0,0,0, 1);
	glEnable(GL_DEPTH_TEST);

	//renderowanie obiektów przezroczystych
	glEnable(GL_BLEND); 
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glfwSetKeyCallback(window, keyCallback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetCursorPosCallback(window, mouse_callback);

	readTexture("./img/rock0.png", TEX_ROCK1);
	readTexture("./img/rock1.png", TEX_ROCK2);
	readTexture("./img/rock2.png", TEX_ROCK3);
	readTexture("./img/rock3.png", TEX_ROCK4);
	readTexture("./img/rock4.png", TEX_ROCK5);
	readTexture("./img/rock5.png", TEX_ROCK6);
	readTexture("./models/sand1.png", TEX_BOTTOM);
	readTexture("./img/lisc.png", TEX_PLANT);
	readTexture("./models/TropicalFish01.png", TEX_FISH5);
	readTexture("./models/TropicalFish02.png", TEX_FISH6);
	readTexture("./models/TropicalFish03.png", TEX_FISH7);
	readTexture("./models/TropicalFish04.png", TEX_FISH8);
	readTexture("./models/TropicalFish05.png", TEX_FISH9);
	readTexture("./models/TropicalFish06.png", TEX_FISH10);
	readTexture("./models/TropicalFish13.png", TEX_FISH11);
	readTexture("./models/TropicalFish11.png", TEX_FISH12);
	readTexture("./models/TropicalFish09.png", TEX_FISH13);
	readTexture("./models/TropicalFish14.png", TEX_FISH14);
	readTexture("./img/bialy.png", TEX_BIALY);

	loadModel(std::string("models/Rock0.obj"), ROCK1);
	loadModel(std::string("models/Rock1.fbx"), ROCK2);
	loadModel(std::string("models/Rock2.fbx"), ROCK3);
	loadModel(std::string("models/Rock3.fbx"), ROCK4);
	loadModel(std::string("models/Rock4.fbx"), ROCK5);
	loadModel(std::string("models/Rock5.fbx"), ROCK6);
	loadModel(std::string("models/plants.obj"), PLANTS);
	loadModel(std::string("models/plants1.obj"), PLANTS); 
	loadModel(std::string("models/TropicalFish01.obj"), FISH2);
	loadModel(std::string("models/TropicalFish02.obj"), FISH3);
	loadModel(std::string("models/TropicalFish03.obj"), FISH4);
	loadModel(std::string("models/TropicalFish04.obj"), FISH5);
	loadModel(std::string("models/TropicalFish05.obj"), FISH6);
	loadModel(std::string("models/TropicalFish06.obj"), FISH7);
	loadModel(std::string("models/TropicalFish13.obj"), FISH8);
	loadModel(std::string("models/TropicalFish11.obj"), FISH9);
	loadModel(std::string("models/TropicalFish09.obj"), FISH10);
	loadModel(std::string("models/TropicalFish14.obj"), FISH11);
	loadModel(std::string("models/lampa.obj"), KOSTKA);

	waterShader = new ShaderProgram("v_water.glsl", NULL, "f_water.glsl");
	phongShader = new ShaderProgram("v_phong.glsl", NULL, "f_phong.glsl");
	glassShader = new ShaderProgram("v_glass.glsl", NULL, "f_glass.glsl");
}

void freeOpenGLProgram(GLFWwindow* window) {
	freeShaders();
}

void drawGlass(glm::mat4 P, glm::mat4 V, glm::mat4 M) {
	glassShader->use();

	glUniformMatrix4fv(glassShader->u("P"), 1, false, glm::value_ptr(P));
	glUniformMatrix4fv(glassShader->u("V"), 1, false, glm::value_ptr(V));
	glUniformMatrix4fv(glassShader->u("M"), 1, false, glm::value_ptr(M));

	glEnableVertexAttribArray(glassShader->a("vertex"));
	glVertexAttribPointer(glassShader->a("vertex"), 4, GL_FLOAT, false, 0, myCubeVertices);

	glEnableVertexAttribArray(glassShader->a("normal"));
	glVertexAttribPointer(glassShader->a("normal"), 4, GL_FLOAT, false, 0, myCubeVertexNormals);

	glDrawArrays(GL_TRIANGLES, 0, myCubeVertexCount);

	glDisableVertexAttribArray(glassShader->a("vertex"));
	glDisableVertexAttribArray(glassShader->a("normal"));
}

void drawButtom(glm::mat4 P, glm::mat4 V, glm::mat4 M) {
	float bottomTexCoords[] = {
		4.0f, 4.0f,	  0.0f, 0.0f,    0.0f, 4.0f,
		4.0f, 4.0f,   4.0f, 0.0f,    0.0f, 0.0f,
	};

	float bottomVertices[] = {
		8.0f,-14.0f, 6.0f,1.0f,
		8.0f, 14.0f,-6.0f,1.0f,
		8.0f,-14.0f,-6.0f,1.0f,

		8.0f,-14.0f, 6.0f,1.0f,
		8.0f, 14.0f, 6.0f,1.0f,
		8.0f, 14.0f,-6.0f,1.0f,
	};

	float bottomVertexNormals[] = {
		1.0f, -1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, -1.0f, 0.0f,
		1.0f, -1.0f, -1.0f, 0.0f,

		1.0f, -1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, -1.0f, 0.0f,
	};

	int bottomVertexCount = 6;

	waterShader->use();

	glUniformMatrix4fv(waterShader->u("P"), 1, false, glm::value_ptr(P));
	glUniformMatrix4fv(waterShader->u("V"), 1, false, glm::value_ptr(V));
	glUniformMatrix4fv(waterShader->u("M"), 1, false, glm::value_ptr(M));

	glEnableVertexAttribArray(waterShader->a("vertex"));
	glVertexAttribPointer(waterShader->a("vertex"), 4, GL_FLOAT, false, 0, bottomVertices);

	glEnableVertexAttribArray(waterShader->a("normal"));
	glVertexAttribPointer(waterShader->a("normal"), 4, GL_FLOAT, false, 0, bottomVertexNormals);

	glEnableVertexAttribArray(waterShader->a("texCoord"));
	glVertexAttribPointer(waterShader->a("texCoord"), 2, GL_FLOAT, false, 0, bottomTexCoords);

	//powtarzanie tekstury metodą repeat
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texs[TEX_BOTTOM]);
	glUniform1i(waterShader->u("tex"), 0);

	glDrawArrays(GL_TRIANGLES, 0, bottomVertexCount);

	glDisableVertexAttribArray(waterShader->a("vertex"));
	glDisableVertexAttribArray(waterShader->a("normal"));
	glDisableVertexAttribArray(waterShader->a("texCoord"));
}

void drawModel(glm::mat4 P, glm::mat4 V, glm::mat4 M, int model_i, int texture) {
	waterShader->use();

	glUniformMatrix4fv(waterShader->u("P"), 1, false, glm::value_ptr(P));
	glUniformMatrix4fv(waterShader->u("V"), 1, false, glm::value_ptr(V));
	glUniformMatrix4fv(waterShader->u("M"), 1, false, glm::value_ptr(M));
	glUniform4fv(waterShader->u("light1"), 1, glm::value_ptr(light1));
	glUniform4fv(waterShader->u("light2"), 1, glm::value_ptr(light2));

	glEnableVertexAttribArray(waterShader->a("vertex"));
	glVertexAttribPointer(waterShader->a("vertex"), 4, GL_FLOAT, false, 0, models[model_i].Vertices.data());

	glEnableVertexAttribArray(waterShader->a("texCoord"));
	glVertexAttribPointer(waterShader->a("texCoord"), 2, GL_FLOAT, false, 0, models[model_i].TexCoords.data());

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texs[texture]);
	glUniform1i(waterShader->u("tex"), 0);

	glDrawElements(GL_TRIANGLES, models[model_i].Indices.size(), GL_UNSIGNED_INT, models[model_i].Indices.data());

	glDisableVertexAttribArray(waterShader->a("vertex"));
	glDisableVertexAttribArray(waterShader->a("normal"));
	glDisableVertexAttribArray(waterShader->a("color"));
}

void drawRocks(glm::mat4 P, glm::mat4 V) {
	glm::mat4 rocks = glm::mat4(1.0f);

	rocks = glm::translate(rocks, glm::vec3(0.0f, -7.85f, 0.0f));
	rocks = glm::scale(rocks, glm::vec3(0.125f, 0.125f, 0.125f));

	glm::mat4 rock1 = glm::translate(rocks, glm::vec3(0.0f, -2.0f, 0.0f));
	drawModel(P, V, rock1, ROCK1, TEX_ROCK1);

	glm::mat4 rock2 = glm::translate(rocks, glm::vec3(70.0f, -2.0f, 0.0f));
	rock2 = glm::scale(rock2, glm::vec3(0.020f, 0.020f, 0.020f));
	drawModel(P, V, rock2, ROCK2, TEX_ROCK2);

	glm::mat4 rock3 = glm::translate(rocks, glm::vec3(-70.0f, -2.0f, 0.0f));
	rock3 = glm::scale(rock3, glm::vec3(0.020f, 0.020f, 0.020f));
	drawModel(P, V, rock3, ROCK3, TEX_ROCK3);

	glm::mat4 rock4 = glm::translate(rocks, glm::vec3(-40.0f, 0.0f, -10.0f));
	rock4 = glm::scale(rock4, glm::vec3(0.010f, 0.010f, 0.010f));
	drawModel(P, V, rock4, ROCK4, TEX_ROCK4);

	glm::mat4 rock5 = glm::translate(rocks, glm::vec3(40.0f, 0.0f, 10.0f));
	rock5 = glm::scale(rock5, glm::vec3(0.030f, 0.030f, 0.030f));
	drawModel(P, V, rock5, ROCK5, TEX_ROCK5);

	glm::mat4 rock6 = glm::translate(rocks, glm::vec3(-80.0f, 0.0f, 20.0f));
	rock6 = glm::scale(rock6, glm::vec3(0.10f, 0.10f, 0.10f));
	drawModel(P, V, rock6, ROCK6, TEX_ROCK6);
}

//Rysowanie lampy
void drawLight(glm::mat4 P, glm::mat4 V, glm::mat4 M) {
	glm::mat4 Mf7 = glm::rotate(M, 0.0f, glm::vec3(1.0f, 0.0f, 0.0f));
	Mf7 = glm::scale(Mf7, glm::vec3(0.05f, 0.05f, 0.05f));
	drawModel(P, V, Mf7, KOSTKA, TEX_BIALY);
}

int plants_random = 0; //zmienna do losowania tylko za pierwszm rysowaniem modelu ryby
float plants_x[15]; //tablica wspolrzednych x ryby
float plants_y[15]; //tablica wspolrzednych y ryby

void drawPlants(glm::mat4 P, glm::mat4 V) {
	glm::mat4 M = glm::mat4(1.0f);
	M = glm::scale(M, glm::vec3(0.25f, 0.25f, 0.25f));

	for (int i = 0; i < 15; ++i) {
		if (plants_random == 0)
		{
			plants_y[i] = (rand() % 20) - 10;
			plants_x[i] = (rand() % 50) - 25;
		}
		glm::mat4 plant1 = glm::scale(M, glm::vec3(1.5f, 1.5f, 1.5f));
		plant1 = glm::translate(plant1, glm::vec3(plants_x[i], -20.2f, plants_y[i]));
		drawModel(P, V, plant1, PLANTS, TEX_PLANT);
	}
	plants_random = 1;
}

//Mapa modeli z dopsowanymi do nich tesksturami
std::map<int, int> fishTextureMap = {
	{FISH2, TEX_FISH5},
	{FISH3, TEX_FISH6},
	{FISH4, TEX_FISH7},
	{FISH5, TEX_FISH8},
	{FISH6, TEX_FISH9},
	{FISH7, TEX_FISH10},
	{FISH8, TEX_FISH11},
	{FISH9, TEX_FISH12},
	{FISH10, TEX_FISH13},
	{FISH11, TEX_FISH14}
};

struct Fish {
	glm::vec3 position; 
	bool movingRight; //kierunek poruszania się ryby
	float rotationAngle;
	float speed;
};

const int fishCount = 10;
Fish fishes[fishCount];

void initializeFishes() {
	std::srand(static_cast<unsigned int>(std::time(0)));
	for (int i = 0; i < fishCount; ++i) {
		float randomX = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 10.0f - 5.0f; //losowanie liczby do pozycji początkowej od -5 do 5
		fishes[i].position = glm::vec3(0.0f, randomX, randomX);
		fishes[i].movingRight = true;
		fishes[i].rotationAngle = 0.0f;
		fishes[i].speed = 0.1f + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / 0.05f)); //losowanie liczby do prędkości od 0.10 do 15
	}
}

void updateFishState(Fish& fish) {
	if (fish.movingRight) {
		fish.position.x += fish.speed;
		if (fish.position.x >= 10.0f) {
			fish.movingRight = false;
			fish.rotationAngle = 3.14f;
		}
	}
	else {
		fish.position.x -= fish.speed;
		if (fish.position.x <= -10.0f) {
			fish.movingRight = true;
			fish.rotationAngle = 0.0f;
		}
	}
}

//Uruchom ruch każdej rybki
void updateAllFishes() {
	for (int i = 0; i < fishCount; ++i) {
		updateFishState(fishes[i]);
	}
}

int czy_losowac = 0;
int rybki[10];
int model_ryb[10];

void drawAllFishes(glm::mat4 P, glm::mat4 V) {
	for (int i = 0; i < fishCount; ++i) {
		glm::mat4 FishMatrix = glm::mat4(1.0f);
		glm::vec3 fishScale = glm::vec3(0.005f, 0.005f, 0.005f);

		FishMatrix = glm::translate(glm::mat4(1.0f), fishes[i].position);
		FishMatrix = glm::rotate(FishMatrix, 1.57f, glm::vec3(0.0f, 1.0f, 0.0f));
		FishMatrix = glm::rotate(FishMatrix, fishes[i].rotationAngle, glm::vec3(0.0f, 1.0f, 0.0f));
		FishMatrix = glm::scale(FishMatrix, fishScale);

		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<> dis(FISH2, FISH11);
		int randomFishType = dis(gen);

		if (czy_losowac == 0)
		{
			rybki[i] = fishTextureMap[randomFishType];
			model_ryb[i] = randomFishType;
		}
		drawModel(P, V, FishMatrix, static_cast<Model>(model_ryb[i]), static_cast<Texture>(rybki[i]));
	}
	czy_losowac = 1;
}

void drawScene(GLFWwindow* window, float angle) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glm::mat4 M = glm::mat4(1.0f);
	glm::mat4 V = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
	glm::mat4 P = glm::perspective(glm::radians(50.0f), 1.0f, 1.0f, 50.0f);

	glm::mat4 Ml1 = glm::translate(M, glm::vec3(light1[0], light1[1], light1[2]));
	drawLight(P, V, Ml1);
	glm::mat4 Ml2 = glm::translate(M, glm::vec3(light2[0], light2[1], light2[2]));
	drawLight(P, V, Ml2);

	glm::mat4 M1 = glm::rotate(M, -PI / 2, glm::vec3(0.0f, 0.0f, 1.0f));
	glm::mat4 Mg = glm::scale(M1, glm::vec3(8.0f, 14.0f, 6.0f));
	
	drawButtom(P, V, M1);
	updateAllFishes();
	drawAllFishes(P, V);
	drawRocks(P, V);
	drawPlants(P, V);
	drawGlass(P, V, Mg);

	glfwSwapBuffers(window);
}

int main(void)
{
	initializeFishes();
	GLFWwindow* window;
	srand(time(0));

	glfwSetErrorCallback(error_callback);

	if (!glfwInit()) {
		fprintf(stderr, "Nie można zainicjować GLFW.\n");
		exit(EXIT_FAILURE);
	}

	window = glfwCreateWindow(800, 800, "OpenGL", NULL, NULL);

	if (!window)
	{
		fprintf(stderr, "Nie można utworzyć okna.\n");
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Nie można zainicjować GLEW.\n");
		exit(EXIT_FAILURE);
	}

	initOpenGLProgram(window);

	float angle = 0;
	glfwSetTime(0);

	while (!glfwWindowShouldClose(window))
	{
		angle += speed * glfwGetTime();
		glfwSetTime(0);
		drawScene(window, angle);
		glfwPollEvents();
	}

	freeOpenGLProgram(window);

	glfwDestroyWindow(window);
	glfwTerminate();
	exit(EXIT_SUCCESS);
}