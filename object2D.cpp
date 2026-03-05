#include "object2D.h"

#include <vector>
#include <cmath>

#include "core/engine.h"
#include "core/gpu/vertex_format.h"


using namespace std;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace object2D
{
	Mesh* CreateRectangleMesh(const std::string& name, glm::vec3 leftBottomCorner,
		float width, float height, glm::vec3 color, bool fill)
	{
		// Definirea colturile dreptunghiului
		glm::vec3 corner = leftBottomCorner;
		std::vector<VertexFormat> vertices =
		{
			VertexFormat(corner, color),
			VertexFormat(corner + glm::vec3(width, 0, 0), color),
			VertexFormat(corner + glm::vec3(width, height, 0), color),
			VertexFormat(corner + glm::vec3(0, height, 0), color)
		};

		Mesh* rectangle = new Mesh(name);
		std::vector<unsigned int> indices;

		if (!fill)
		{
			indices = { 0, 1, 2, 3 };
			rectangle->SetDrawMode(GL_LINE_LOOP);
		}
		else
		{
			indices = {
				0, 1, 2,
				0, 2, 3
			};
			rectangle->SetDrawMode(GL_TRIANGLES);
		}

		rectangle->InitFromData(vertices, indices);
		return rectangle;
	}

	Mesh* CreateCircleMesh(const std::string& name, glm::vec3 center, float radius, glm::vec3 color, int segments)
	{
		std::vector<VertexFormat> vertices;
		std::vector<unsigned int> indices;

		// Adauga varful central al discului.
		vertices.push_back(VertexFormat(center, color));
		indices.push_back(0);

		// Calculeaza si adauga varfurile de pe circumferinta folosind trigonometria
		for (int i = 0; i <= segments; ++i) {
			float angle = 2.0f * M_PI * static_cast<float>(i) / static_cast<float>(segments);
			float x = center.x + radius * cos(angle);
			float y = center.y + radius * sin(angle);
			vertices.push_back(VertexFormat(glm::vec3(x, y, 0), color));
			indices.push_back(i + 1);
		}

		Mesh* circle = new Mesh(name);
		// Seteaza modul de desenare GL_TRIANGLE_FAN pentru a umple cercul de la centru
		circle->SetDrawMode(GL_TRIANGLE_FAN);
		circle->InitFromData(vertices, indices);
		return circle;
	}

	Mesh* CreateStartButtonMesh(const std::string& name, glm::vec3 color)
	{
		std::vector<VertexFormat> vertices =
		{
			VertexFormat(glm::vec3(0, 0, 0), color),
			VertexFormat(glm::vec3(0, 1, 0), color),
			VertexFormat(glm::vec3(1, 1, 0), color),
			VertexFormat(glm::vec3(1, 0, 0), color),
			VertexFormat(glm::vec3(0.5f, 0.5f, 0), color)
		};

		std::vector<unsigned int> indices = {
			1, 4, 0,
			0, 4, 3,
			2, 4, 1
		};

		Mesh* mesh = new Mesh(name);
		mesh->SetDrawMode(GL_TRIANGLES);
		mesh->InitFromData(vertices, indices);
		return mesh;
	}
} // namespace object2D