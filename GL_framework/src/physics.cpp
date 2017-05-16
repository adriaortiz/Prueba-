#include <imgui\imgui.h>
#include <imgui\imgui_impl_glfw_gl3.h>
#include <cstdlib>
#include <math.h>
#include <iostream>
#include <glm\glm.hpp>

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
wave *wavesStruct;

void initCloth() {

	int structVertsX = ClothMesh::numCols - 1;
	int structVertsZ = ClothMesh::numRows - 1;

	vertsStructPrev = new vert[ClothMesh::numVerts]; //Generando array de vertices anteriores
	vertsStruct = new vert[ClothMesh::numVerts]; //Generando array de vertices actuales
	wavesStruct = new wave[2];

	//Inicializando waves:

	wavesStruct[0].Position = glm::vec3(0.0f,0.0f,0.0f);
	wavesStruct[0].Kvec = glm::vec3(1.0f, 0.0f, 0.0f);
	wavesStruct[0].Ai = 0.5f;
	wavesStruct[0].Freq = 2.0f;

	wavesStruct[1].Position = glm::vec3(0.0f, 0.0f, 0.0f);
	wavesStruct[1].Kvec = glm::vec3(0.0f, 0.0f, -3.0f);
	wavesStruct[1].Ai = 0.25f;
	wavesStruct[1].Freq = 4.0f;

	//Dando posicion inicial a cada vertice (Formar una malla):

	int v = 0;

	for (int i = 0; i < ClothMesh::numRows; i++) { //columnas
		for (int j = 0; j < ClothMesh::numCols; j++) { //filas
			vertsStruct[v].posX = -4.f + separation * j;
			vertsStruct[v].posY = 7.f;
			vertsStruct[v].posZ = -4.f + separation * i;
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

float dotPPos(plane thePlane, vert vertexCoords) { //Función dot product entre plano y coordenadas de vertice
	
	float x = thePlane.nx * vertexCoords.posX;
	float y = thePlane.ny * vertexCoords.posY;
	float z = thePlane.nz * vertexCoords.posZ;

	float result = x + y + z;
	return result;
}

float dotPVel(plane thePlane, vert vertexCoords) { //Función dot product entre plano y componentes de velocidad de vertice

	float x = thePlane.nx * vertexCoords.velX;
	float y = thePlane.ny * vertexCoords.velY;
	float z = thePlane.nz * vertexCoords.velZ;

	float result = x + y + z;
	return result;
}

bool isColliding(plane thePlane, vert vertexCoords, vert prevVertexCoords) { //Función comprobadora de colisión entre plano y vertice

	//Formula de colision:
	if ((dotPPos(thePlane, prevVertexCoords) + thePlane.d) * (dotPPos(thePlane, vertexCoords) + thePlane.d) <= 0) return true;
	else return false;
}

void updateCloth(float dt) {

	for (int i = 0; i < ClothMesh::numVerts; ++i) {
		if (i != 0 && i != 13) {

			//Guardado de posiciones actuales como anteriores:
			vertsStructPrev[i].posX = vertsStruct[i].posX;
			vertsStructPrev[i].posY = vertsStruct[i].posY;
			vertsStructPrev[i].posZ = vertsStruct[i].posZ;

			//Actualizar posicion:
			vertsStruct[i].posX = vertsStruct[i].posX + vertsStruct[i].velX * dt;
			vertsStruct[i].posY = vertsStruct[i].posY + vertsStruct[i].velY * dt;
			vertsStruct[i].posZ = vertsStruct[i].posZ + vertsStruct[i].velZ * dt;

			//Actualizar velocidad:
			vertsStruct[i].velX = vertsStruct[i].velX + 0.f * dt; //Vf = Vi + dt * Fi/m
			vertsStruct[i].velY = vertsStruct[i].velY + gravity * dt;
			vertsStruct[i].velZ = vertsStruct[i].velZ + 0.f * dt;
		}
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