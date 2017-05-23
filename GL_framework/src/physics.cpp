#include <imgui\imgui.h>
#include <imgui\imgui_impl_glfw_gl3.h>
#include <cstdlib>
#include <math.h>
#include <iostream>
#include <glm\glm.hpp>
#include <glm\gtc\matrix_transform.hpp>
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

namespace Sphere {
	extern float mass;
	extern glm::vec3 CoM;
	extern glm::mat4 objMat;
	extern void updateSphere(glm::vec3 pos, float radius);
	extern void setupSphere(glm::vec3 pos, float radius);
	glm::vec3 velocity (0.f,0.f,0.f);
} 


glm::mat4 transformationMatrix;
float separation = 0.5;
glm::vec3 gravity (0, -9.81f, 0); //Vertical downward speed

float OffsetX, OffsetY = 5.0, OffsetZ;

struct vert {
	glm::vec3 position;
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
	float KiMod;
	float Ai;
	float Freq;
	float Phase;
};

bool show_test_window = false;
void GUI() {
	{	//FrameRate
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

		//TODO
	}

	// ImGui test window. Most of the sample code is in ImGui::ShowTestWindow()
	if (show_test_window) {
		ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiSetCond_FirstUseEver);
		ImGui::ShowTestWindow(&show_test_window);
	}
}

//Punteros a primeras posiciones guardadas para creación de arrays correspondientes:
float *vertsFloat;
vert *vertsStruct;
vert *vertsFirstStruct;

vector <wave> Waves;

void initCloth() {

	int structVertsX = ClothMesh::numCols - 1;
	int structVertsZ = ClothMesh::numRows - 1;

	vertsStruct = new vert[ClothMesh::numVerts]; //Generando array de vertices actuales
	vertsFirstStruct = new vert[ClothMesh::numVerts]; //Generando array de primeros vertices (Estado inicial)

													  //Inicializando waves:

	wave wave1;

	wave1.Position = glm::vec3(0.0f, 0.0f, 0.0f);
	wave1.Kvec = glm::vec3(1.0f, 0.0f, 0.0f);
	wave1.KiMod = glm::length(wave1.Kvec);
	wave1.Ai = 0.5f;
	wave1.Freq = 2.0f;
	wave1.Phase = 0.0f;

	wave wave2;

	wave2.Position = glm::vec3(20.0f, 20.0f, 20.0f);
	wave2.Kvec = glm::vec3(0.0f, 0.0f, -3.0f);
	wave2.KiMod = glm::length(wave2.Kvec);
	wave2.Ai = 0.25f;
	wave2.Freq = 4.0f;
	wave2.Phase = 0.0f;

	Waves.push_back(wave1);
	Waves.push_back(wave2);


	//Dando posicion inicial a cada vertice (Formar una malla):

	int v = 0;

	for (int i = 0; i < ClothMesh::numRows; i++) { //columnas
		for (int j = 0; j < ClothMesh::numCols; j++) { //filas
			vertsStruct[v].position.x = -4.f + separation * j;
			vertsStruct[v].position.y = 7.f;
			vertsStruct[v].position.z = -4.f + separation * i;

			vertsFirstStruct[v].position.x = -4.f + separation * j;
			vertsFirstStruct[v].position.y = 7.f;
			vertsFirstStruct[v].position.z = -4.f + separation * i;

			v++;
		}
	}

	//Conversor de posiciones a array de floats:
	vertsFloat = new float[ClothMesh::numVerts * 3];

	for (int i = 0; i < ClothMesh::numVerts; ++i) {
		vertsFloat[i * 3 + 0] = vertsStruct[i].position.x;
		vertsFloat[i * 3 + 1] = vertsStruct[i].position.y;
		vertsFloat[i * 3 + 2] = vertsStruct[i].position.z;
	}
}

float fullTime;


void updateSphereStuff(float dt) {

	//Actualizar centro de massa:
	glm::vec3 futureCoM = Sphere::CoM + dt * Sphere::velocity;
	//Actualizar velocidad:
	glm::vec3 futureVel = Sphere::velocity + dt*gravity/Sphere::mass;

	transformationMatrix = glm::translate(transformationMatrix, Sphere::CoM);
	Sphere::objMat = transformationMatrix;

	Sphere::CoM = futureCoM;
	Sphere::velocity = futureVel;
}

void updateClothStuff(float dt) {

	fullTime += dt;

	for (int i = 0; i < ClothMesh::numVerts; ++i) {

		//Actualizar posicion mediante formula de Gerstner:

		glm::vec3 waveInfluence1;
		glm::vec3 waveInfluence2;

		float waveInfluenceY1;
		float waveInfluenceY2;

		//Cada formula de oscilacion de cada wave, individualmente:
		waveInfluence1 = (Waves[0].Kvec / Waves[0].KiMod) * Waves[0].Ai * glm::sin(glm::dot(Waves[0].Kvec, vertsFirstStruct[i].position) - (Waves[0].Freq * fullTime) + Waves[0].Phase);
		waveInfluence2 = (Waves[1].Kvec / Waves[1].KiMod) * Waves[1].Ai * glm::sin(glm::dot(Waves[1].Kvec, vertsFirstStruct[i].position) - (Waves[1].Freq * fullTime) + Waves[1].Phase);

		waveInfluenceY1 = Waves[0].Ai * glm::cos(glm::dot(Waves[0].Kvec, vertsFirstStruct[i].position) - (Waves[0].Freq * fullTime) + Waves[0].Phase);
		waveInfluenceY2 = Waves[1].Ai * glm::cos(glm::dot(Waves[1].Kvec, vertsFirstStruct[i].position) - (Waves[1].Freq * fullTime) + Waves[1].Phase);

		//Debug:
		//cout << Waves[0].Kvec.x << " " << Waves[0].Kvec.y << " " << Waves[0].Kvec.z << endl;
		//cout << Waves[0].KiMod << endl;

		//Calcular nuevas posiciones mediante formula de Gerstner:
		vertsStruct[i].position = vertsFirstStruct[i].position - (waveInfluence1 + waveInfluence2);

		//Esta posicion sobrescribe la componente calculada anteriormente (que no era valida)
		vertsStruct[i].position.y = waveInfluenceY1 + waveInfluenceY2;

	}

	//Conversor de posiciones a array de floats:

	for (int i = 0; i < ClothMesh::numVerts; ++i) {
		vertsFloat[i * 3 + 0] = vertsStruct[i].position.x + OffsetX; //x
		vertsFloat[i * 3 + 1] = vertsStruct[i].position.y + OffsetY; //y
		vertsFloat[i * 3 + 2] = vertsStruct[i].position.z + OffsetZ; //z
	}
}



void PhysicsInit() {

	initCloth();

	Sphere::setupSphere(Sphere::CoM, 1);
	Sphere::velocity = glm::vec3(0.f, 0.f, 0.f);

	ClothMesh::updateClothMesh(vertsFloat);

}
void PhysicsUpdate(float dt) {

	updateClothStuff(dt);
	updateSphereStuff(dt);

	Sphere::updateSphere(Sphere::CoM, 1);
	ClothMesh::updateClothMesh(vertsFloat);
}

void PhysicsCleanup() {
	delete[] vertsStruct;
	delete[] vertsFloat;
}