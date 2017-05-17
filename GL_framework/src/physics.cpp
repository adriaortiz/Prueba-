#include <imgui\imgui.h>
#include <imgui\imgui_impl_glfw_gl3.h>
#include <cstdlib>
#include <math.h>
#include <iostream>
#include <glm\glm.hpp>
#include <vector>

using namespace std;

namespace ClothMesh {
	extern const int numCols;
	extern const int numRows;
	extern const int numVerts;
	extern void setupClothMesh();
	extern void cleanupClothMesh();
	extern void updateClothMesh(float* array_data);
	extern void drawClothMesh();
}

float separation = 0.5;
float gravity = 0.f; //Vertical downward speed

struct vert {
	float posX, posY, posZ;
	float velX, velY, velZ;
	int id;
};

struct spring {
	float elasticity;
	float damping;
	float originalLength;
};

//Variables globals del punt:
struct wave
{
	glm::vec3 Position;
	glm::vec3 Kvec;
	float KiMod = glm::length(Kvec);
	float Ai;
	float Freq;
//	float Phase;
};

bool show_test_window = false;
void GUI() {
	{	//FrameRate
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

		//TODO
	}

	// ImGui test window. Most of the sample code is in ImGui::ShowTestWindow()
	if(show_test_window) {
		ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiSetCond_FirstUseEver);
		ImGui::ShowTestWindow(&show_test_window);
	}
}

//Punteros a primeras posiciones guardadas para creación de arrays correspondientes:
float *vertsFloat;
vert *vertsStructPrev;
vert *vertsStruct;
vert *vertsFirstStruct;

vector <wave> Waves;

void initCloth() {

	int structVertsX = ClothMesh::numCols - 1;
	int structVertsZ = ClothMesh::numRows - 1;

	vertsStructPrev = new vert[ClothMesh::numVerts]; //Generando array de vertices anteriores
	vertsStruct = new vert[ClothMesh::numVerts]; //Generando array de vertices actuales
	vertsFirstStruct = new vert[ClothMesh::numVerts]; //Generando array de primeros vertices (Estado inicial)

	//Inicializando waves:

	wave wave1;

	wave1.Position = glm::vec3(0.0f,0.0f,0.0f);
	wave1.Kvec = glm::vec3(1.0f, 0.0f, 0.0f);
	wave1.Ai = 0.5f;
	wave1.Freq = 2.0f;

	wave wave2;

	wave2.Position = glm::vec3(0.0f, 0.0f, 0.0f);
	wave2.Kvec = glm::vec3(0.0f, 0.0f, -3.0f);
	wave2.Ai = 0.25f;
	wave2.Freq = 4.0f;

	Waves.push_back(wave1);
	Waves.push_back(wave2);


	//Dando posicion inicial a cada vertice (Formar una malla):

	int v = 0;

	for (int i = 0; i < ClothMesh::numRows; i++) { //columnas
		for (int j = 0; j < ClothMesh::numCols; j++) { //filas
			vertsStruct[v].posX = -4.f + separation * j;
			vertsStruct[v].posY = 7.f;
			vertsStruct[v].posZ = -4.f + separation * i;

			vertsFirstStruct[v].posX = -4.f + separation * j;
			vertsFirstStruct[v].posY = 7.f;
			vertsFirstStruct[v].posZ = -4.f + separation * i;

			v++;
		}
	}

	//Velocidad inicial de cada vertice
	for (int i = 0; i < ClothMesh::numVerts; i++) {
		vertsStruct[i].velX = 0.f;
		vertsStruct[i].velY = 0.f;
		vertsStruct[i].velZ = 0.f;
	}

	//Conversor de posiciones a array de floats:
	vertsFloat = new float[ClothMesh::numVerts * 3];

	for (int i = 0; i < ClothMesh::numVerts; ++i) {
		vertsFloat[i * 3 + 0] = vertsStruct[i].posX;
		vertsFloat[i * 3 + 1] = vertsStruct[i].posY;
		vertsFloat[i * 3 + 2] = vertsStruct[i].posZ;
	}
}

float fullTime;

void updateCloth(float dt) {

	fullTime += dt;

	for (int i = 0; i < ClothMesh::numVerts; ++i) {

			//Guardado de posiciones actuales como anteriores:
			vertsStructPrev[i].posX = vertsStruct[i].posX;
			vertsStructPrev[i].posY = vertsStruct[i].posY;
			vertsStructPrev[i].posZ = vertsStruct[i].posZ;

			//Actualizar posicion mediante formula de Gerstner:

			float a, b, c, x, y, z;
																					//Cambiar KVEC a todas sus componentes por el struct de vertices pasado a VEC3
			a = glm::vec3(Waves[0].Kvec / Waves[0].KiMod).x * Waves[0].Ai * glm::sin(glm::dot(Waves[0].Kvec.x, vertsFirstStruct[i].posX) - Waves[0].Freq *fullTime);
			b = Waves[0].Ai * glm::cos(glm::dot(Waves[0].Kvec.y, vertsFirstStruct[i].posY) - Waves[0].Freq);
			c = glm::vec3(Waves[0].Kvec / Waves[0].KiMod).z * Waves[0].Ai * glm::sin(glm::dot(Waves[0].Kvec.z, vertsFirstStruct[i].posZ) - Waves[0].Freq);

			x = glm::vec3(Waves[1].Kvec / Waves[1].KiMod).x * Waves[1].Ai * glm::sin(glm::dot(Waves[1].Kvec.x, vertsFirstStruct[i].posX) - Waves[1].Freq);
			y = Waves[0].Ai * glm::cos(glm::dot(Waves[1].Kvec.y, vertsFirstStruct[i].posY) - Waves[1].Freq);
			z = glm::vec3(Waves[1].Kvec / Waves[1].KiMod).z * Waves[1].Ai * glm::sin(glm::dot(Waves[1].Kvec.z, vertsFirstStruct[i].posZ) - Waves[1].Freq);

			vertsStruct[i].posX = vertsFirstStruct[i].posX - a + x; //a + x es el sumatori
			vertsStruct[i].posY = b + y;
			vertsStruct[i].posZ = vertsFirstStruct[i].posZ - c + z;

			//Actualizar velocidad:
			vertsStruct[i].velX = vertsStruct[i].velX + 0.f * dt; //Vf = Vi + dt * Fi/m
			vertsStruct[i].velY = vertsStruct[i].velY + gravity * dt;
			vertsStruct[i].velZ = vertsStruct[i].velZ + 0.f * dt;
	}

	//Conversor de posiciones a array de floats:

	for (int i = 0; i < ClothMesh::numVerts; ++i) {
		vertsFloat[i * 3 + 0] = vertsStruct[i].posX; //x
		vertsFloat[i * 3 + 1] = vertsStruct[i].posY; //y
		vertsFloat[i * 3 + 2] = vertsStruct[i].posZ; //z
	}
}

void PhysicsInit() {

	initCloth();

	ClothMesh::updateClothMesh(vertsFloat);

}
void PhysicsUpdate(float dt) {

	updateCloth(dt);

	ClothMesh::updateClothMesh(vertsFloat);
}

void PhysicsCleanup() {
	delete[] vertsStruct;
	delete[] vertsStructPrev;
	delete[] vertsFloat;
}