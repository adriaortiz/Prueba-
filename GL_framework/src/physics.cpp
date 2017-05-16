#include <imgui\imgui.h>
#include <imgui\imgui_impl_glfw_gl3.h>
#include <cstdlib>
#include <math.h>
#include <iostream>

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
float gravity = -9.81f; //Vertical downward speed

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

struct plane {
	float nx, ny, nz;
	float d;
	char planeID;
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
plane *planesStruct;
vert *vertsStructPrev;
vert *vertsStruct;

void initCloth(){

	int structVertsX = ClothMesh::numCols - 1;
	int structVertsZ = ClothMesh::numRows - 1;

	planesStruct = new plane[6];
	vertsStructPrev = new vert[ClothMesh::numVerts]; //Generando array de vertices anteriores
	vertsStruct = new vert[ClothMesh::numVerts]; //Generando array de vertices actuales

	//Relleno manual de datos de planos:

	//A
	planesStruct[0].nx = 0; //Podrían calcularse los componentes de la normal creando 2 vectores sabiendo los puntos del plano y normalizando su multiplicación vectorial
	planesStruct[0].ny = 0;
	planesStruct[0].nz = 1;
	planesStruct[0].d = -5; //Tambien podría calcularse utilizando la formula del plano y aislando la d
	planesStruct[0].planeID = 'A';
	//B
	planesStruct[1].nx = 1;
	planesStruct[1].ny = 0;
	planesStruct[1].nz = 0;
	planesStruct[1].d = -5;
	planesStruct[1].planeID = 'B';
	//C
	planesStruct[2].nx = 0;
	planesStruct[2].ny = 0;
	planesStruct[2].nz = 1;
	planesStruct[2].d = 5;
	planesStruct[2].planeID = 'C';
	//D
	planesStruct[3].nx = 1;
	planesStruct[3].ny = 0;
	planesStruct[3].nz = 0;
	planesStruct[3].d = 5;
	planesStruct[3].planeID = 'D';
	//E
	planesStruct[4].nx = 0;
	planesStruct[4].ny = 1;
	planesStruct[4].nz = 0;
	planesStruct[4].d = -10;
	planesStruct[4].planeID = 'E';
	//F
	planesStruct[5].nx = 0;
	planesStruct[5].ny = 1;
	planesStruct[5].nz = 0;
	planesStruct[5].d = 0;
	planesStruct[5].planeID = 'F';

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

void collidePlane(plane thePlane, vert &vertexData) { //Función de rebote con elasticidad, debería aplicarse a un vertice que ha colisionado con un plano

	float e = 0.2; //Coeficiente de rebote
	
	float PVPProduct = dotPPos(thePlane, vertexData); //Plane Vertex Position Product

	vertexData.posX = vertexData.posX -(1 + e) * (PVPProduct + thePlane.d) * thePlane.nx;
	vertexData.posY = vertexData.posY - (1 + e) * (PVPProduct + thePlane.d) * thePlane.ny;
	vertexData.posZ = vertexData.posZ - (1 + e) * (PVPProduct + thePlane.d) * thePlane.nz;

	float PVVProduct = dotPVel(thePlane, vertexData); //Plane Vertex Velocity Product

	vertexData.velX = vertexData.velX - (1 + e) * PVVProduct * thePlane.nx;
	vertexData.velY = vertexData.velY - (1 + e) * PVVProduct * thePlane.ny;
	vertexData.velZ = vertexData.velZ - (1 + e) * PVVProduct * thePlane.nz;
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

			//Comprobar colision
			for (int j = 0; j < 6; j++) {
				//No entiendo porque, pero el plano F siempre da 1 = true (colisiona constantemente) despues de atravesar el plano por primera vez, no importa donde este la malla...
				//cout << isColliding(planesStruct[j], vertsStruct[i], vertsStructPrev[i]) << endl;
				if (isColliding(planesStruct[j], vertsStruct[i], vertsStructPrev[i]) == true) { //Si hay colision
					collidePlane(planesStruct[j], vertsStruct[i]); //Mirror de la velocidad y la posición
				}
			}
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
	delete[] planesStruct;
}