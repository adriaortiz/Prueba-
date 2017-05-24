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
	extern float radius;
} 


glm::mat4 transformationMatrix;
float separation = 0.5;
float GravedadEnY = -9.81;
glm::vec3 gravity (0, GravedadEnY, 0); //Vertical downward speed

float OffsetX, OffsetY = 5.0, OffsetZ;

struct vert {
	glm::vec3 position;
	glm::vec3 DistVec;
	float DistMod;
};

struct spring {
	float elasticity;
	float damping;
	float originalLength;
};

//Variables globals del punt:
struct wave
{
public:
	glm::vec3 Position;
	glm::vec3 Kvec;
	float KiMod;
	float Ai;
	float Freq;
	float Phase;
};

wave wave1;



bool show_test_window = false;

void GUI() {
	{	//FrameRate
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::Text("Distancias entre vertices: ");
		ImGui::DragFloat("Wave 1 Kvec X", &wave1.Kvec.x, 1.f, 0.0f, 5.0f);
		ImGui::DragFloat("Wave1 Ai", &wave1.Ai, 0.5f, 0.0f, 20.0f);			//Nom a dins del GUI, Que modifica, Valor inicial, Valor m�nim, Valor m�xim
		ImGui::DragFloat("Wave1 Freq", &wave1.Freq, 10.0f, 5.0f, 20.0f);		//2.0,1.0,10.0
		ImGui::DragFloat("Wave1 Phase", &wave1.Phase, 0.0f, 0.0f, 1.0f);

		ImGui::Text("Gravedad: ");
		ImGui::DragFloat("Gravedad", &GravedadEnY, -9.81, -20.0, 0.0);						//Gravetat en y, valor inicial, valor m�nim, valor m�xim
																							//TODO
	}

	// ImGui test window. Most of the sample code is in ImGui::ShowTestWindow()
	if (show_test_window) {
		ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiSetCond_FirstUseEver);
		ImGui::ShowTestWindow(&show_test_window);
	}
}


/*void GUI() {
	{	//FrameRate
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		

		//ImGui::DragFloat("Dist second-Link", &D_2Distance, 0.5f, 0.0f, 1.0f);
		//ImGui::DragFloat("Max % links", &Max_Distance, 0.1f, 0.0f, 1.0f);					//El % maxim qn
		//ImGui::DragFloat("Dist Vertex X inicial", &InitX, 0.5f, 0.0f, 0.6f);
		//ImGui::DragFloat("Dist Vertex Z inicial", &InitZ, 0.5f, 0.0f, 0.6f);

		

		//Recorda que si son constants, no es poden modificar!
		//TODO
	}

	// ImGui test window. Most of the sample code is in ImGui::ShowTestWindow()
	if (show_test_window) {
		ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiSetCond_FirstUseEver);
		ImGui::ShowTestWindow(&show_test_window);
	}
}*/





//Punteros a primeras posiciones guardadas para creaci�n de arrays correspondientes:
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


//Variables para clacular la Buoyance
float fullTime;
glm::vec3 flotation;
glm::vec3 ForceSum;
glm::vec3 Y(0, 0.5, 0);
float Density = 2, G_plus = 9.81, Sub_Vol; //G_Plus es la gravetad positiva

// Los cuatro puntos mas cercanios al centro de masas de la esfera
vert Close_1;
vert Close_2;
vert Close_3;
vert Close_4;

void Finder() {
	Close_1 = vertsStruct[0];
	Close_2 = vertsStruct[1];
	Close_3 = vertsStruct[2];
	Close_4 = vertsStruct[3];

	for (int i = 0; i < ClothMesh::numVerts; i++) {
		vertsStruct[i].DistVec = glm::abs(Sphere::CoM - vertsStruct[i].position);
		vertsStruct[i].DistMod = glm::length(vertsStruct[i].DistVec);
		if(i > 3)
		{
			vert prov;
			vert aux;
			if(vertsStruct[i].DistMod < Close_1.DistMod)
			{
				prov = Close_1;
				Close_1 = vertsStruct[i];
				if (prov.DistMod < Close_2.DistMod)
					aux = Close_2;
					Close_2 = prov;
					if (aux.DistMod < Close_3.DistMod)
						prov = Close_3;
						Close_3 = aux;
						if (prov.DistMod < Close_4.DistMod)
							Close_4 = prov;

			}
			else if (vertsStruct[i].DistMod < Close_2.DistMod)
			{
				prov = Close_2;
				Close_2 = vertsStruct[i];
				if (prov.DistMod < Close_3.DistMod)
					aux = Close_3;
					Close_3 = prov;
						if (aux.DistMod < Close_4.DistMod)
							Close_4 = aux;
			}
			else if (vertsStruct[i].DistMod < Close_3.DistMod)
			{
				aux = Close_3;
				Close_3 = vertsStruct[i];
				if (aux.DistMod < Close_4.DistMod)
					Close_4 = aux;
			}
			else if (vertsStruct[i].DistMod < Close_4.DistMod)
			{
				Close_4 = vertsStruct[i];
			}
		}
		//vertsStruct[i].position ;
	}
	
	//for () {
	//
	//}


}


void updateSphereStuff(float dt) {
	float Mitja;
	//Actualizar centro de massa (Posicion):
	glm::vec3 futureCoM = Sphere::CoM + dt * Sphere::velocity;

	
	Finder();
	Mitja = (Close_1.position.y + Close_2.position.y + Close_3.position.y + Close_4.position.y) / 4;
	cout << Close_1.position.y << " " << Close_2.position.y << " " << Close_3.position.y << " " << Close_4.position.y << endl;

	if ((Sphere::CoM.y - Sphere::radius) >= Mitja) {
		flotation = glm::vec3(0.f, 0.f, 0.f);
	}
	else {
		//Sumatorio de Fuerzas
		//cout << "done" << endl;
		Sub_Vol = (Sphere::radius * Sphere::radius) * (Mitja - (Sphere::CoM.y - Sphere::radius));
		flotation = (Density * G_plus *  Sub_Vol) * Y;
	}
	ForceSum = gravity + flotation;
	//cout << ForceSum.x << " " << ForceSum.y << " " << ForceSum.z << endl;
	//Actualizar velocidad:
	glm::vec3 futureVel = Sphere::velocity + dt*ForceSum/Sphere::mass;

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

	Sphere::setupSphere(glm::vec3(0.0f, 8.0f, 0.0f), 1);
	Sphere::velocity = glm::vec3(0.0f, 0.0f, 0.0f);
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