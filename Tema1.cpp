#include "Tema1.h"

#include <iostream>
#include <queue>
#include <algorithm>

#include "transform2D.h"
#include "object2D.h"
#include "object2D.cpp"


using namespace std;
using namespace m1;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Tema1::Tema1() {
}

Tema1::~Tema1() {
	delete textRenderer;
}

void Tema1::FrameStart() {
	// Sterge ecranul si seteaza culoarea de fundal pe negru
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Setez viewportu-ul = rezolutia ferestrei
	glm::ivec2 res = window->GetResolution();
	glViewport(0, 0, res.x, res.y);
}

void Tema1::Update(float deltaTimeSeconds) {
	// Alege modul curent: Editor sau Joc
	if (isEditorModeActive) {
		DrawEditor();
	}
	else {
		UpdateCameraShake(deltaTimeSeconds);
		UpdateParticleEffects(deltaTimeSeconds);
		RunBreakoutGameLoop(deltaTimeSeconds);
	}
}

void Tema1::FrameEnd()
{
}





void Tema1::Init() {
	glm::ivec2 resolution = window->GetResolution();

	// Configurarea rezolutiei si a textRenderului
	textRenderer = new::gfxc::TextRenderer(window->props.selfDir, resolution.x, resolution.y);
	textRenderer->Load(PATH_JOIN(window->props.selfDir, RESOURCE_PATH::FONTS, "Hack-Bold.ttf"), 24);

	// Dimensiunea grilei si starea initiala (Drag & Drop)
	gridCols = 15;
	gridRows = 7;
	solidBlockScale = 0.9f;
	isEditorModeActive = true;

	gridLayout.assign(gridRows, vector<BlockType>(gridCols, BlockType::NONE));
	blocksCount = 0;
	isDraggingBlock = false;
	draggingBlockType = BlockType::NONE;

	// Creez meshu-urile necesare editorului si calculez layout-ul acestuia.
	CreateMeshes();
	CalculateEditorLayout();

	// Geometria rachetei (Paddle)
	paddleMovementInputX = 0.0f;
	CalculatePaddleGeometryAndBounds();
	paddleCurrentCenterX = paddleCenter.x;

	// Setari pentru camera
	auto camera = GetSceneCamera();
	camera->SetOrthographic(0, (float)resolution.x, 0, (float)resolution.y, 0.01f, 400);
	camera->SetPosition(glm::vec3(0, 0, 50));
	camera->SetRotation(glm::vec3(0, 0, 0));
	camera->Update();
	GetCameraInput()->SetActive(false);

	// Initializarea timerelor pentru Camera Shake.
	cameraShakeDuration = 0.15f;
	cameraShakeMagnitude = 8.0f;
	cameraShakeTimer = 0.0f;
	cameraShakeOffset = glm::vec2(0.0f, 0.0f);

	// Reseteaza jocul de breakout
	srand(time(NULL));
	ResetBreakoutGame();
}




void Tema1::OnInputUpdate(float deltaTime, int mods) {
	// Doar pentru jocul Breakout
	if (!isEditorModeActive) {
		paddleMovementInputX = 0.0f;
		// Deplasarea rachetei (Paddle)
		if (window->KeyHold(GLFW_KEY_LEFT)) {
			paddleMovementInputX -= paddleMovementSpeed * deltaTime;
		}
		if (window->KeyHold(GLFW_KEY_RIGHT)) {
			paddleMovementInputX += paddleMovementSpeed * deltaTime;
		}

		// Calculeaza noua pozitie X
		float newX = paddleCurrentCenterX + paddleMovementInputX;

		// Verifica limitele de miscare
		if (newX < paddleMinXBoundary) {
			paddleCurrentCenterX = paddleMinXBoundary;
		}
		else if (newX > paddleMaxXBoundary) {
			paddleCurrentCenterX = paddleMaxXBoundary;
		}
		else {
			// Seteaza pozitia finala
			paddleCurrentCenterX = newX;
		}
	}
}

void Tema1::OnKeyPress(int key, int mods) {
	// Doar pentru Breakout
	if (!isEditorModeActive) {

		// Verifica daca tasta apasata este SPACE si daca bila nu este deja in miscare
		if (key == 32 && !isBallLaunched) {
			isBallLaunched = true;

			// Calculeaza viteza necesara pe X și Y, asigurand ca viteza totala 
			// este exact 'ballSpeed' la un unghi de 45 de grade
			float velMagnitude = ballSpeed / (float)sqrt(2.0);
			// Seteaza componentele X si Y ale vitezei
			float velX = velMagnitude;
			float velY = velMagnitude;

			// Aplica vectorul de viteza bilei
			ballVelocity = glm::vec2(velX, velY);
		}
	}
}

void Tema1::OnKeyRelease(int key, int mods) {
}

void Tema1::OnMouseMove(int mouseX, int mouseY, int deltaX, int deltaY) {
	currentMousePosition = glm::ivec2(mouseX, mouseY);
}

void Tema1::OnMouseBtnPress(int mouseX, int mouseY, int button, int mods) {
	if (button == 1) {
		// Pentru drag & drop in editor
		int selectedBlock = SelectBlockFromEditor(mouseX, mouseY);

		if (selectedBlock != -1) {
			isDraggingBlock = true;
			draggingBlockType = GetBlockType(selectedBlock);
		}

		glm::ivec2 resolution = window->GetResolution();
		int mouseY_GameCoords = resolution.y - mouseY;

		// Verifica daca s-a apasat butonul de Start Game
		if (mouseX >= startButtonX && mouseX <= startButtonX + startButtonSize &&
			mouseY_GameCoords >= startButtonY && mouseY_GameCoords <= startButtonY + startButtonSize)
		{
			if (CheckAllConstraints()) {
				TransitionToBreakoutGame();
			}
		}
	}
}

void Tema1::OnMouseBtnRelease(int mouseX, int mouseY, int button, int mods) {
	if (button == 1) {
		// Finalizeaza drag & drop in editor
		if (isDraggingBlock) {
			// Converteste coordonatele mouse-ului in coordonatele grilei
			glm::ivec2 cell = GetGridCoordinatesFromMouse(mouseX, mouseY);
			int col = cell.x;
			int row = cell.y;

			// Verifica daca este in limite si daca blocul este SOLID
			if (IsInsideGridBounds(col, row) && draggingBlockType == BlockType::SOLID) {
				if (gridLayout[row][col] == BlockType::NONE) {
					blocksCount++;
				}
				gridLayout[row][col] = BlockType::SOLID;
			}

			isDraggingBlock = false;
			draggingBlockType = BlockType::NONE;
		}
	}

	// Logica pentru stergerea unui bloc cu left click
	if (button == 2) {
		glm::ivec2 cell = GetGridCoordinatesFromMouse(mouseX, mouseY);
		// Verifica daca este in limite si daca exista un bloc de sters
		if (IsInsideGridBounds(cell.x, cell.y) && gridLayout[cell.y][cell.x] != BlockType::NONE) {
			int r = cell.y;
			int c = cell.x;
			gridLayout[r][c] = BlockType::NONE;
			blocksCount--;
		}
	}
}

void Tema1::OnMouseScroll(int mouseX, int mouseY, int offsetX, int offsetY) {
}



// Functia care creeaza si incarca toate mesh-urile necesare editorului si jocului
void Tema1::CreateMeshes() {
	solid_block = "solid_block";
	float baseSize = 1.17f;

	std::vector<VertexFormat> solid_block_vertices =
	{
		VertexFormat(glm::vec3(0, 0, 0.1f), glm::vec3(0.35f, 0.35f, 0.35f)),
		VertexFormat(glm::vec3(baseSize, 0, 0.1f), glm::vec3(0.35f, 0.35f, 0.35f)),
		VertexFormat(glm::vec3(baseSize, baseSize, 0.1f), glm::vec3(0.35f, 0.35f, 0.35f)),
		VertexFormat(glm::vec3(0, baseSize, 0.1f), glm::vec3(0.35f, 0.35f, 0.35f)),
		VertexFormat(glm::vec3(baseSize / 2.0f, baseSize / 2.0f, 0.1f), glm::vec3(1.0f, 1.0f, 1.0f))
	};

	std::vector<unsigned int> solid_block_indices = {
		0, 4, 1,
		1, 4, 2,
		2, 4, 3,
		3, 4, 0
	};

	Mesh* mesh_solid_block = new Mesh(solid_block);
	mesh_solid_block->SetDrawMode(GL_TRIANGLES);
	mesh_solid_block->InitFromData(solid_block_vertices, solid_block_indices);
	AddMeshToList(mesh_solid_block);

	green_square_editor = "green_square_editor";
	Mesh* mesh_green_square_editor = object2D::CreateRectangleMesh(green_square_editor, glm::vec3(0, 0, 0), 1.0f, 1.0f, glm::vec3(0.2f, 0.9f, 0.2f), true);
	AddMeshToList(mesh_green_square_editor);

	green_start_button = "green_start_button";
	Mesh* mesh_green_start_button = object2D::CreateStartButtonMesh(green_start_button, glm::vec3(0.2f, 0.9f, 0.2f));
	AddMeshToList(mesh_green_start_button);

	red_start_button = "red_start_button";
	Mesh* mstart_red_new = object2D::CreateStartButtonMesh(red_start_button, glm::vec3(1.0f, 0.2f, 0.2f));
	AddMeshToList(mstart_red_new);

	blue_cell_editor = "blue_cell_editor";
	Mesh* mesh_blue_cell_editor = object2D::CreateRectangleMesh(blue_cell_editor, glm::vec3(0, 0, 0), 0.9f, 0.9f, glm::vec3(0.2f, 0.4f, 0.7f), true);
	AddMeshToList(mesh_blue_cell_editor);

	red_line_separator = "red_line_separator";
	Mesh* mesh_red_line_separator = object2D::CreateRectangleMesh(red_line_separator, glm::vec3(0, 0, 0), 1.0f, 1.0f, glm::vec3(1.0f, 0.2f, 0.2f), true);
	AddMeshToList(mesh_red_line_separator);

	blue_grid_frame_editor = "blue_grid_frame_editor";
	Mesh* mesh_blue_grid_frame_editor = object2D::CreateRectangleMesh(blue_grid_frame_editor, glm::vec3(0, 0, 0), 1.0f, 1.0f, glm::vec3(0.2f, 0.4f, 0.9f), true);
	AddMeshToList(mesh_blue_grid_frame_editor);

	breakout_ball = "game_ball_circle";
	Mesh* mesh_breakout_ball = object2D::CreateCircleMesh(breakout_ball, glm::vec3(0, 0, 0), 1.0f, glm::vec3(0.2f, 0.4f, 0.7f), 30);
	AddMeshToList(mesh_breakout_ball);

	yellow_brick = "game_yellow_brick";
	Mesh* mesh_yellow_brick = object2D::CreateRectangleMesh(yellow_brick, glm::vec3(0, 0, 0), 1.0f, 1.0f, glm::vec3(1.0f, 1.0f, 0.2f), true);
	AddMeshToList(mesh_yellow_brick);

	red_brick = "game_red_brick";
	Mesh* mesh_red_brick = object2D::CreateRectangleMesh(red_brick, glm::vec3(0, 0, 0), 1.0f, 1.0f, glm::vec3(1.0f, 0.2f, 0.2f), true);
	AddMeshToList(mesh_red_brick);

	dark_blue_brick = "game_dark_blue_brick";
	Mesh* mesh_dark_blue_brick = object2D::CreateRectangleMesh(dark_blue_brick, glm::vec3(0, 0, 0), 1.0f, 1.0f, glm::vec3(0.1f, 0.1f, 0.6f), true);
	AddMeshToList(mesh_dark_blue_brick);
}




void Tema1::DrawEditor() {
	// Deseneaza componentele editorului
	DrawGridBlueCells();
	DrawBlocksToChooseFromEditor();
	DrawGreenSquaresEditor();
	DrawRedSeparatorLine();
	DrawBlueGridFrame();
	DrawStartGameButton();

	// Deseneaza blocul fantoma daca se face drag & drop
	if (isDraggingBlock) {
		DrawGhostBlock(currentMousePosition.x, currentMousePosition.y);
	}
}

void Tema1::DrawGridBlueCells() {
	// Calculeaza factorii de ajustare necesari pentru centrarea precisa
	// a mesh-ului solid (care are o scara geometrica diferita) in interiorul celulei
	float drawnBlockScale = solidBlockScale;
	float centerOffset = (gridCellSize - drawnBlockScale * gridCellSize) / 2.0f;
	float solid_mesh_centering_offset = (drawnBlockScale * gridCellSize * 0.25f) / 2.0f;

	for (int r = 0; r < gridRows; r++) {
		for (int c = 0; c < gridCols; c++) {
			// Calculeaza pozitia fiecarei celule albastre in grid, pornind de la origine
			float cellX = (float)gridCellOrigin.x + c * (gridCellSize + gridSpacing);
			float cellY = (float)gridCellOrigin.y + r * (gridCellSize + gridSpacing);
			BlockType b = gridLayout[r][c];

			// Desenarea celulei albastre
			if (b == BlockType::NONE) {
				glm::mat3 model = glm::mat3(1);
				model *= transform2D::Translate(cellX + centerOffset, cellY + centerOffset);
				model *= transform2D::Scale(drawnBlockScale * gridCellSize, drawnBlockScale * gridCellSize);
				RenderMesh2D(meshes[blue_cell_editor], shaders["VertexColor"], model);
			}

			if (b == BlockType::NONE) continue;

			// Desenara blocului solid
			glm::mat3 m2 = glm::mat3(1);
			m2 *= transform2D::Translate(cellX + centerOffset, cellY + centerOffset);
			m2 *= transform2D::Translate(-solid_mesh_centering_offset, -solid_mesh_centering_offset);
			m2 *= transform2D::Scale(drawnBlockScale * gridCellSize, drawnBlockScale * gridCellSize);

			RenderMesh2D(meshes[solid_block], shaders["VertexColor"], m2);
		}
	}
}

void Tema1::DrawBlocksToChooseFromEditor() {
	// Deseneaza blocul solid in partea stanga, pentru selectie
	glm::ivec2 res = window->GetResolution();

	float startX = (float)solidBlockOrigin.x;
	float startY = (float)solidBlockOrigin.y;

	float cellY = (res.y - solidBlockSize) / 2.0f;
	float cellX = startX;

	solidBlockPositionInEditor = glm::vec2(cellX, cellY);

	glm::mat3 m_solid_block = glm::mat3(1);
	m_solid_block *= transform2D::Translate(cellX, cellY);
	m_solid_block *= transform2D::Scale(solidBlockSize, solidBlockSize);

	RenderMesh2D(meshes[solid_block], shaders["VertexColor"], m_solid_block);
}

void Tema1::DrawGreenSquaresEditor() {
	// Deseneaza patratele verzi care indica locurile ramase libere pentru blocuri
	glm::ivec2 res = window->GetResolution();
	// Calculeaza spatierea si pozitia de start a contorului pe ecran
	float spacing = gridCellSize * 0.55f;
	int originX = gridCellOrigin.x - 50;

	float topBarPadding = gridCellSize * 0.1f;
	int originY = res.y - gridCellSize - static_cast<int>(topBarPadding) - 50;

	// Itereaza si deseneaza doar pentru slot-urile care nu au fost inca folosite
	int availableSlots = maxAllowedBlocks - blocksCount;
	for (int i = 0; i < maxAllowedBlocks; i++) {
		if (i < availableSlots) {
			glm::mat3 m = glm::mat3(1);
			// Calculeaza pozitia X a patratului curent
			int x = originX + i * (gridCellSize + static_cast<int>(spacing));
			m *= transform2D::Translate((float)x, (float)originY);
			m *= transform2D::Scale((float)gridCellSize, (float)gridCellSize);

			RenderMesh2D(meshes[green_square_editor], shaders["VertexColor"], m);
		}
	}
}

void Tema1::DrawStartGameButton() {
	// Deseneaza butonul de Start Game in partea dreapta sus
	glm::ivec2 res = window->GetResolution();

	startButtonSize = gridCellSize;
	// Calculeaza spatierea si pozitia de start a butonului pe ecran
	float proportionalSpacing = gridCellSize * 0.55f;
	int blockCounterStartX = gridCellOrigin.x - 50;

	// Pozitia X este dupa ultimul patrat verde
	startButtonX = blockCounterStartX + maxAllowedBlocks * (gridCellSize + static_cast<int>(proportionalSpacing));
	float topBarPadding = gridCellSize * 0.1f;
	startButtonY = res.y - startButtonSize - static_cast<int>(topBarPadding) - 50;

	glm::mat3 m = glm::mat3(1);
	m *= transform2D::Translate((float)startButtonX, (float)startButtonY);
	m *= transform2D::Scale((float)startButtonSize, (float)startButtonSize);

	// Verifica daca toate constrangerile sunt indeplinite pentru a decide culoarea butonului
	bool is_valid = CheckAllConstraints();

	if (is_valid) {
		RenderMesh2D(meshes[green_start_button], shaders["VertexColor"], m);
	}
	else {
		RenderMesh2D(meshes[red_start_button], shaders["VertexColor"], m);
	}
}

void Tema1::DrawGhostBlock(int mx, int my) {
	// Deseneaza blocul fantoma sub mouse in timpul drag & drop-ului
	glm::ivec2 res = window->GetResolution();

	// Converteste coordonata Y a mouse-ului din Top-Down in sistemul de joc (Bottom-Up)
	int flipped = res.y - my;
	float final_ghost_block_size = solidBlockSize;
	float centerOffset = final_ghost_block_size / 2.0f;

	glm::mat3 m = glm::mat3(1);

	m *= transform2D::Translate((float)mx - centerOffset, (float)flipped - centerOffset);
	m *= transform2D::Scale(final_ghost_block_size, final_ghost_block_size);

	RenderMesh2D(meshes[solid_block], shaders["VertexColor"], m);
}

void Tema1::DrawRedSeparatorLine() {
	// Linia rosie care separa zona de selectie a blocurilor de zona grilei
	glm::ivec2 res = window->GetResolution();

	float totalWidth = (float)res.x;
	float lineX = totalWidth * 0.2f; // 20% din latimea ecranului

	float lineWidth = 1.0f;
	float lineHeight = (float)res.y; // Linia rosie se intinde pe toata inaltimea ecranului

	glm::mat3 model = glm::mat3(1);

	model *= transform2D::Translate(lineX, 0);
	model *= transform2D::Scale(lineWidth, lineHeight);

	RenderMesh2D(meshes[red_line_separator], shaders["VertexColor"], model);
}

void Tema1::DrawBlueGridFrame() {
	// Deseneaza cadrul albastru in jurul grilei editorului
	float frameThickness = 1.0f;
	float framePadding = gridCellSize * 0.2f;

	// Calculeaza dimensiunile totale ale grilei
	float gridTotalHeight = gridRows * gridCellSize + (gridRows - 1) * gridSpacing;
	float gridDisplayWidth = gridCols * gridCellSize + (gridCols - 1) * gridSpacing;
	
	// Calculeaza pozitia de start a cadrului
	float frameStartX = (float)gridCellOrigin.x - framePadding;
	float frameStartY = (float)gridCellOrigin.y - framePadding;

	// Calculeaza dimensiunile totale ale cadrului
	float frameTotalWidth = gridDisplayWidth + 2 * framePadding;
	float frameTotalHeight = gridTotalHeight + 2 * framePadding;

	// Deseneaza cele patru laturi ale cadrului
	glm::mat3 model_bottom = glm::mat3(1);
	model_bottom *= transform2D::Translate(frameStartX, frameStartY);
	model_bottom *= transform2D::Scale(frameTotalWidth, frameThickness);
	RenderMesh2D(meshes[blue_grid_frame_editor], shaders["VertexColor"], model_bottom);

	glm::mat3 model_top = glm::mat3(1);
	model_top *= transform2D::Translate(frameStartX, frameStartY + frameTotalHeight - frameThickness);
	model_top *= transform2D::Scale(frameTotalWidth, frameThickness);
	RenderMesh2D(meshes[blue_grid_frame_editor], shaders["VertexColor"], model_top);

	glm::mat3 model_left = glm::mat3(1);
	model_left *= transform2D::Translate(frameStartX, frameStartY);
	model_left *= transform2D::Scale(frameThickness, frameTotalHeight);
	RenderMesh2D(meshes[blue_grid_frame_editor], shaders["VertexColor"], model_left);

	glm::mat3 model_right = glm::mat3(1);
	model_right *= transform2D::Translate(frameStartX + frameTotalWidth - frameThickness, frameStartY);
	model_right *= transform2D::Scale(frameThickness, frameTotalHeight);
	RenderMesh2D(meshes[blue_grid_frame_editor], shaders["VertexColor"], model_right);
}



int Tema1::SelectBlockFromEditor(int mouseX, int mouseY) {
	glm::ivec2 res = this->window->GetResolution();
	const float LEFT_SIDE = 0.25f;
	// Calculeaza limita zonei de selectie a blocurilor
	float zoneBoundaryX = (float)res.x * LEFT_SIDE;
	float separatorPadding = 60.0f;
	float maxSelectionX = zoneBoundaryX - separatorPadding;

	// Verifica daca mouse-ul este in zona de selectie a blocurilor
	if (mouseX > 0 && mouseX < maxSelectionX) {
		return 0;
	}

	return -1;
}

BlockType Tema1::GetBlockType(int row) {
	// Returneaza tipul de bloc corespunzator randului selectat
	switch (row) {
	case 0: return BlockType::SOLID;
	default: return BlockType::NONE;
	}
}


glm::ivec2 Tema1::GetGridCoordinatesFromMouse(int mouseX, int mouseY) {
	glm::ivec2 res = window->GetResolution();
	int mouseY_BottomUp = res.y - mouseY;

	// Calculeaza distanta de la origine (distanceFromGridOriginX/Y)
	int distanceFromGridOriginX = mouseX - (gridCellOrigin.x);
	int distanceFromGridOriginY = mouseY_BottomUp - (gridCellOrigin.y);

	// Verifica daca mouse-ul este in afara grilei
	if (distanceFromGridOriginX < 0 || distanceFromGridOriginY < 0) return glm::ivec2(-1, -1);

	int cellSizeUnit = gridCellSize + gridSpacing;
	// Calculeaza coordonatele grilei (gridColumn/gridRow)
	int gridColumn = static_cast<int>(floor((float)distanceFromGridOriginX / cellSizeUnit));
	int gridRow = static_cast<int>(floor((float)distanceFromGridOriginY / cellSizeUnit));

	// Verifica daca mouse-ul este in interiorul unei celule (nu in spatiul de separare)
	int remainingX = distanceFromGridOriginX - gridColumn * cellSizeUnit;
	int remainingY = distanceFromGridOriginY - gridRow * cellSizeUnit;

	if (remainingX > gridCellSize || remainingY > gridCellSize) {
		return glm::ivec2(-1, -1);
	}

	return glm::ivec2(gridColumn, gridRow);
}

bool Tema1::IsInsideGridBounds(int gx, int gy) {
	// Verifica daca coordonatele grilei sunt in limite
	if (gx < 0 || gy < 0 || gx >= gridCols || gy >= gridRows) return false;
	return true;
}




bool Tema1::CheckAllConstraints() {
	// Verifica toate constrangerile pentru a permite inceperea jocului
	if (blocksCount > maxAllowedBlocks) return false;
	if (blocksCount == 0) return false;
	if (!CheckConnectivityConstraint()) return false;

	return true;
}


bool Tema1::CheckConnectivityConstraint() {
	// Verifica daca toate blocurile sunt conectate intre ele
	// Am folosit algoritmul BFS pentru a parcurge blocurile conectate
	glm::ivec2 traversalStartPoint = glm::ivec2(-1, -1);
	int totalBlocks = blocksCount;

	for (int r = 0; r < gridRows; r++) {
		for (int c = 0; c < gridCols; c++) {
			if (gridLayout[r][c] != BlockType::NONE) {
				traversalStartPoint = glm::ivec2(c, r);
				goto StartFound;
			}
		}
	}

StartFound:
	if (totalBlocks == 0) {
		return true;
	}

	std::queue<glm::ivec2> cellsToProcess;
	std::vector<std::vector<bool>> isVisited(gridRows, std::vector<bool>(gridCols, false));
	int connectedBlocksCount = 0;

	cellsToProcess.push(traversalStartPoint);
	isVisited[traversalStartPoint.y][traversalStartPoint.x] = true;

	// Directiile de explorare (delta Row / delta Column)
	const int dRow[] = { -1, 1, 0, 0 }; // Sus, Jos, 
	const int dCol[] = { 0, 0, -1, 1 }; // Stanga, Dreapta

	while (!cellsToProcess.empty()) {
		glm::ivec2 currentCell = cellsToProcess.front();
		cellsToProcess.pop();
		connectedBlocksCount++;

		for (int i = 0; i < 4; i++) {
			int neighborRow = currentCell.y + dRow[i];
			int neighborCol = currentCell.x + dCol[i];

			if (IsInsideGridBounds(neighborCol, neighborRow) &&
				!isVisited[neighborRow][neighborCol] &&
				gridLayout[neighborRow][neighborCol] != BlockType::NONE)
			{
				isVisited[neighborRow][neighborCol] = true;
				cellsToProcess.push(glm::ivec2(neighborCol, neighborRow));
			}
		}
	}

	return connectedBlocksCount == totalBlocks;
}


void Tema1::CalculateEditorLayout() {
	glm::ivec2 resolution = window->GetResolution();

	float screenWidth = (float)resolution.x;
	float screenHeight = (float)resolution.y;

	// Definirea constantelor si zonelor
	const float LEFT_ZONE_RATIO = 0.25f;
	float leftZoneWidth = screenWidth * LEFT_ZONE_RATIO;
	float rightZoneWidth = screenWidth - leftZoneWidth;

	float spacingRatio = 0.05f;
	float paddingRatio = 0.1f;
	const float GRID_MAX_WIDTH_FACTOR = 0.95f;

	// Determinarea dimensiunii celulei (gridCellSize)

	float baseUnit = screenHeight / gridRows;
	float minFixedVerticalSpace = baseUnit * (2.0f * paddingRatio);

	// Limita maxima pe X
	float maxGridWidth = rightZoneWidth * GRID_MAX_WIDTH_FACTOR;
	float factorX = gridCols + (gridCols - 1) * spacingRatio;
	float maxCellSizeX = maxGridWidth / factorX;

	// Limita maxima pe Y
	float availableGridHeight = screenHeight - minFixedVerticalSpace;
	float factorY = gridRows + (gridRows - 1) * spacingRatio;
	float maxCellSizeY = availableGridHeight / factorY;

	// Alege cea mai mica dimensiune
	gridCellSize = static_cast<int>(std::min(maxCellSizeX, maxCellSizeY));

	// Calculeaza spatiul de separare
	gridSpacing = gridCellSize * spacingRatio;

	// Dimensiunile toolbar-ului de sus si padding-urile
	float toolbarHeight = gridCellSize;
	float toolbarPadding = gridCellSize * paddingRatio;
	float verticalGap = gridCellSize;
	float bottomPadding = gridCellSize * paddingRatio;

	// Calcularea pozitiei de origine a grilei
	float gridTotalHeight = gridRows * gridCellSize + (gridRows - 1) * gridSpacing;
	float gridDisplayWidth = gridCols * gridCellSize + (gridCols - 1) * gridSpacing;

	// Limita superioara a barii de sus
	float topBarBottomY = resolution.y - toolbarHeight - toolbarPadding;
	float safeBottomBoundary = bottomPadding;
	float availableVerticalSpace = topBarBottomY - verticalGap - safeBottomBoundary;

	// Centrarea grilei in zona dreapta
	gridCellOrigin.y = safeBottomBoundary + (availableVerticalSpace - gridTotalHeight) / 2.0f;
	gridCellOrigin.x = leftZoneWidth + (rightZoneWidth - gridDisplayWidth) / 2;

	// Calcularea pozitiei si dimensiunii blocului solid din stanga
	solidBlockSize = gridCellSize * 0.9f;
	float offsetToLeft = 60.0f;
	float effectiveLeftZoneWidth = leftZoneWidth - offsetToLeft;
	float solidBlockOriginX = (effectiveLeftZoneWidth / 2.0f) - (solidBlockSize / 2.0f);

	solidBlockOrigin = glm::ivec2(
		(int)(solidBlockOriginX),
		0
	);
}

void Tema1::TransitionToBreakoutGame() {
	// Schimba starea jocului din editor in modul Breakout
	isEditorModeActive = false;
	ResetBreakoutGame();
}






void Tema1::ResetBreakoutGame() {
	// Reseteaza toate variabilele si starea jocului Breakout
	SetupGameBricks();
	CalculatePaddleGeometryAndBounds();
	ResetBallAndPaddlePosition();

	currentLives = 3;
	score = 0;
}

void Tema1::RunBreakoutGameLoop(float deltaTimeSeconds) {
	glm::ivec2 res = window->GetResolution();

	// Verifica daca mai sunt blocuri ramase
	int remainingBricks = 0;
	for (const std::vector<m1::Brick>& row : gameBricksData) {
		for (const m1::Brick& brick : row) {
			// Verifica daca caramida mai exista
			if (brick.isAlive) {
				remainingBricks++;
			}
		}
	}

	if (remainingBricks == 0) {
		isEditorModeActive = true;
		return;
	}

	if (!isBallLaunched) {
		ballPosition.x = paddleCurrentCenterX;
	}

	//	Actualizeaza pozitia bilei daca a fost lansata
	if (isBallLaunched) {
		ballPosition += ballVelocity * deltaTimeSeconds;
	}
	// Gestioneaza coliziunile bilei cu caramizile
	HandleBallBrickCollisions();


	float ball_collision_radius = ballRadius * 1.5f;
	// Coliziune cu marginea din stannga
	if (ballPosition.x - ball_collision_radius < 0) {
		ballPosition.x = ball_collision_radius;
		ballVelocity.x *= -1;
	}

	// Coliziune cu marginea din dreapta
	if (ballPosition.x + ball_collision_radius > res.x) {
		ballPosition.x = res.x - ball_collision_radius;
		ballVelocity.x *= -1;
	}


	// Coliziune cu tavanul
	if (ballPosition.y + ball_collision_radius > res.y) {
		ballPosition.y = res.y - ball_collision_radius;
		ballVelocity.y *= -1;
	}

	// Coliziune cu podeaua (pierdere viata)
	if (ballPosition.y - ball_collision_radius < 0) {
		currentLives--;
		if (currentLives <= 0) {
			isEditorModeActive = true;
		}
		else {
			paddleCurrentCenterX = paddleCenter.x;
			ResetBallAndPaddlePosition();
		}
	}

	// Coliziune cu paleta
	float paddle_half_width = paddleWidth / 2.0f;
	float paddle_min_x = paddleCurrentCenterX - paddle_half_width;
	float paddle_max_x = paddleCurrentCenterX + paddle_half_width;
	float scaledCellSize = gridCellSize * 0.5f;
	float paddle_min_y = paddleTopYBoundary - scaledCellSize;
	float paddle_max_y = paddleTopYBoundary;

	// Verifica coliziunea AABB - Circle
	if (ballPosition.x + ball_collision_radius > paddle_min_x &&
		ballPosition.x - ball_collision_radius < paddle_max_x &&
		ballPosition.y - ball_collision_radius < paddle_max_y &&
		ballPosition.y + ball_collision_radius > paddle_min_y)

	{
		if (ballVelocity.y < 0) {
			ballVelocity.y *= -1;
			ballPosition.y = paddle_max_y + ball_collision_radius;

			// Calculeaza unghiul de bounce in functie de pozitia pe paleta
			float relative_x = (ballPosition.x - paddleCurrentCenterX) / paddle_half_width;
			float max_bounce_angle = M_PI / 3.0f;
			float bounce_angle = relative_x * max_bounce_angle;

			// Calculeaza magnitudinea vitezei bilei
			float velMagnitude = glm::length(ballVelocity);

			if (velMagnitude < ballSpeed * 0.9f) 
				velMagnitude = ballSpeed;

			// Actualizeaza viteza bilei in functie de noul unghi
			ballVelocity.x = velMagnitude * (float)sin(bounce_angle);
			ballVelocity.y = velMagnitude * (float)cos(bounce_angle);

		}
	}

	DrawGameHUD();
	DrawBricks(deltaTimeSeconds);
	DrawBall();
	DrawPaddle();
	DrawParticleEffects();

}


void Tema1::DrawGameHUD() {
	// Desenare scor (Stanga sus)
	glm::ivec2 res = window->GetResolution();
	float scale = 1.0f;
	glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f);

	const float TEXT_Y_POSITION = res.y - 700.f;
	std::string scoreText = "Score: " + std::to_string((int)score);
	float scoreX = 20.0f;

	textRenderer->RenderText(scoreText, scoreX, TEXT_Y_POSITION, scale, color);

	// Desenare vieti ramase (Dreapta sus)
	std::string livesText = "Lives: " + std::to_string(currentLives);
	float livesX_Approx_Start = res.x - 140.0f;

	textRenderer->RenderText(livesText, livesX_Approx_Start, TEXT_Y_POSITION, scale, color);
}


void Tema1::DrawPaddle() {
	// Calculeaza unitatea de masura la scara jocului
	const float GAME_SCALE = 0.5f;
	float scaledCellSize = gridCellSize * GAME_SCALE;
	float scaledSpacing = gridSpacing * GAME_SCALE;
	float scaledBlockUnit = scaledCellSize + scaledSpacing; // Marimea unui bloc pe grila

	float drawnBlockScale = GAME_SCALE;

	// Calculeaza offset-ul de centrare pentru blocurile desenate
	float centeringOffsetCell = (scaledCellSize - drawnBlockScale * gridCellSize) / 2.0f;
	float centeringOffsetMesh = (drawnBlockScale * gridCellSize * 0.2f) / 2.0f;
	float totalCenteringOffset = centeringOffsetCell - centeringOffsetMesh;

	// Gasirea coltului stanga-jos al paddle-ului
	int firstBlockColumnIndex = gridCols;
	int firstBlockRowIndex = gridRows;

	// Determinarea pozitiei celei mai din stanga si cea mai de jos celule ocupate
	for (int r = 0; r < gridRows; r++) {
		for (int c = 0; c < gridCols; c++) {
			if (gridLayout[r][c] == BlockType::SOLID) {
				if (c < firstBlockColumnIndex) firstBlockColumnIndex = c;
				if (r < firstBlockRowIndex) firstBlockRowIndex = r;
			}
		}
	}
	// Calculul pozitiei paddle-ului pe ecran
	float paddleHalfWidth = paddleWidth / 2.0f;
	float paddleLeftEdgeX = paddleCurrentCenterX - paddleHalfWidth; // Marginea din stanga a paddle-ului
	// Pozitia Y fixa a paddle-ului
	const float FIXED_BOTTOM_Y = window->GetResolution().y * 0.1f;

	// Desenarea fiecarui bloc solid din paddle
	for (int r = 0; r < gridRows; r++) {
		for (int c = 0; c < gridCols; c++) {
			if (gridLayout[r][c] == BlockType::SOLID) {
				// Calculul pozitiei relative a blocului in paddle
				float relativeX = (c - firstBlockColumnIndex) * scaledBlockUnit;
				float relativeY = (r - firstBlockRowIndex) * scaledBlockUnit;
				// Pozitia finala a blocului pe ecran
				float finalBlockScreenX = paddleLeftEdgeX + relativeX;
				float finalBlockScreenY = FIXED_BOTTOM_Y + relativeY;

				// Dimensiunea de scalare
				float finalScale = drawnBlockScale * gridCellSize;
				float designOffsetY = GAME_SCALE; // Offset-ul vertical de design

				// Construirea matricei model (transformarea finala)
				glm::mat3 m = glm::mat3(1);

				// Muta blocul la pozitia absoluta, incluzand ajustarile de centrare si camera shake.
				m *= transform2D::Translate(
					finalBlockScreenX + totalCenteringOffset + cameraShakeOffset.x,
					finalBlockScreenY + totalCenteringOffset + designOffsetY + cameraShakeOffset.y
				);
				// Aplica scalarea la dimensiunea finala a blocului
				m *= transform2D::Scale(finalScale, finalScale);

				RenderMesh2D(meshes[solid_block], shaders["VertexColor"], m);
			}
		}
	}
}





void Tema1::CalculatePaddleGeometryAndBounds() {
	glm::ivec2 res = window->GetResolution();
	// Calculeaza dimensiunile paddle-ului pe baza layout-ului grilei
	const float gameScale = 0.5f;
	float scaledCellSize = gridCellSize * gameScale;
	float scaledSpacing = gridSpacing * gameScale;
	float scaledBlockUnit = scaledCellSize + scaledSpacing;
	float screenWidth = (float)res.x;

	int minCol = gridCols;
	int maxCol = -1;
	int minRow = gridRows;
	int maxRow = -1;
	// Gasirea limitelor blocurilor solide in grila
	bool foundBlock = false;
	for (int r = 0; r < gridRows; r++) {
		for (int c = 0; c < gridCols; c++) {
			if (gridLayout[r][c] == BlockType::SOLID) {
				foundBlock = true;
				if (c < minCol) minCol = c;
				if (c > maxCol) maxCol = c;
				if (r < minRow) minRow = r;
				if (r > maxRow) maxRow = r;
			}
		}
	}

	// Calculul dimensiunilor paddle-ului
	float totalCols = (float)(maxCol - minCol + 1);
	paddleWidth = totalCols * scaledBlockUnit - scaledSpacing;
	float totalRows = (float)(maxRow - minRow + 1);
	paddleHeight = totalRows * scaledBlockUnit - scaledSpacing;

	// Setarea pozitiei initiale a paddle-ului (Fix pe Oy, centrat pe Ox)
	const float FIXED_BOTTOM_Y = res.y * 0.1f;

	paddleCenter.y = FIXED_BOTTOM_Y + paddleHeight / 2.0f;
	paddleTopYBoundary = FIXED_BOTTOM_Y + paddleHeight;
	paddleCenter.x = screenWidth / 2.0f;

	// Setarea limitelor de miscare pe Ox
	float paddleRadiusX = paddleWidth / 2.0f;
	paddleMinXBoundary = paddleRadiusX;
	paddleMaxXBoundary = screenWidth - paddleRadiusX;
}





void Tema1::ResetBallAndPaddlePosition() {
	// Reseteaza pozitia bilei deasupra paletei
	const float VISUAL_PADDING_Y = 5.0f;

	if (paddleTopYBoundary > 0) {
		initialBallPosition = glm::vec2(
			paddleCurrentCenterX,
			paddleTopYBoundary + ballRadius + VISUAL_PADDING_Y
		);
	}

	ballPosition = initialBallPosition;
	ballVelocity = glm::vec2(0, 0);
	isBallLaunched = false;
}

void Tema1::SetupGameBricks() {
	glm::ivec2 resolution = window->GetResolution();

	// Goleste datele existente despre caramizi
	gameBricksData.clear();
	float screenWidth = (float)resolution.x;

	// Definirea configuratiei grilei de caramizi
	const float BRICK_PADDING_X = 5.0f;
	const float BRICK_PADDING_Y = 5.0f;
	const int NUM_ROWS = 5;
	const int NUM_COLS = 15;

	const float BRICK_ZONE_TOP_Y = resolution.y * 0.9f;
	const float BRICK_ZONE_BOTTOM_Y = resolution.y * 0.55f;

	// Latimea totala disponibila pentru caramizi
	float availableWidth = screenWidth - 20 * BRICK_PADDING_X;
	float brickWidth = availableWidth / NUM_COLS - BRICK_PADDING_X;

	// Inaltimea totala disponibila pentru caramizi
	float availableHeight = BRICK_ZONE_TOP_Y - BRICK_ZONE_BOTTOM_Y;
	float brickHeight = (availableHeight / NUM_ROWS) - BRICK_PADDING_Y;

	// Calculul pozitiei de start X (centrat pe ecran)
	float effectiveGridWidth = NUM_COLS * (brickWidth + BRICK_PADDING_X) + BRICK_PADDING_X;
	float gridStartX = (screenWidth - effectiveGridWidth) / 2.0f + BRICK_PADDING_X;

	// Pozitia de start Y
	float currentBottomY = BRICK_ZONE_BOTTOM_Y;

	// Configurarea caramizilor pe randuri
	std::vector<std::tuple<int, glm::vec3, float>> brick_config = {
		{1, glm::vec3(0.2f, 0.4f, 0.7f), 1.0f},
		{2, glm::vec3(1.0f, 1.0f, 0.2f), 1.01f},
		{3, glm::vec3(1.0f, 0.2f, 0.2f), 1.02f},
		{2, glm::vec3(1.0f, 1.0f, 0.2f), 1.01f},
		{1, glm::vec3(0.2f, 0.4f, 0.7f), 1.0f}
	};

	// Crearea caramizilor in grila
	for (int r = 0; r < NUM_ROWS; r++) {
		std::vector<Brick> row;
		// Preia configuratia pentru randul curent
		int health = std::get<0>(brick_config[r]);
		glm::vec3 color = std::get<1>(brick_config[r]);
		float initialScale = std::get<2>(brick_config[r]);

		// Pozitia Y (coltul stanga-jos al caramizii)
		float rowStartY = currentBottomY + r * (brickHeight + BRICK_PADDING_Y) + BRICK_PADDING_Y;

		for (int c = 0; c < NUM_COLS; c++) {
			// Pozitia X (coltul stanga-jos al caramizii)
			float brickStartX = gridStartX + c * (brickWidth + BRICK_PADDING_X);

			row.push_back(Brick(
				glm::vec2(brickStartX, rowStartY),
				brickWidth,
				brickHeight,
				health,
				color,
				true,
				initialScale,
				0.0f
			));
		}
		gameBricksData.push_back(row);
	}
}

void Tema1::DrawBricks(float deltaTimeSeconds) {

	for (std::vector<Brick>& currentBrickRow : gameBricksData) {
		for (Brick& currentBrick : currentBrickRow) {

			if (!currentBrick.isAlive) continue;

			std::string meshName;
			float currentScaleFactor = currentBrick.initialScale; // Scara curenta pentru animatie

			// Logica animatiei de distrugere
			if (currentBrick.destroyTimer > 0.0f) {
				currentBrick.destroyTimer -= deltaTimeSeconds;

				if (currentBrick.destroyTimer <= 0.0f) {
					currentBrick.isAlive = false;
					continue;
				}
				// Calculeaza progresul animatiei
				float progress = currentBrick.destroyTimer / currentBrick.destroyDuration;
				currentScaleFactor = currentBrick.initialScale * progress;
				meshName = dark_blue_brick;
			}
			// Logica starii normale a caramizii
			else {
				if (currentBrick.health == 3) {
					meshName = red_brick;
				}
				else if (currentBrick.health == 2) {
					meshName = yellow_brick;
				}
				else if (currentBrick.health == 1) {
					meshName = dark_blue_brick;
				}
			}

			// Transformarea si desenarea caramizii
			float halfBrickWidth = currentBrick.width / 2.0f;
			float halfBrickHeight = currentBrick.height / 2.0f;

			glm::mat3 m = glm::mat3(1);

			m *= transform2D::Translate(
				currentBrick.position.x + halfBrickWidth + cameraShakeOffset.x,
				currentBrick.position.y + halfBrickHeight + cameraShakeOffset.y
			);
			m *= transform2D::Scale(currentScaleFactor * currentBrick.width,
				currentScaleFactor * currentBrick.height);
			m *= transform2D::Translate(-0.5f, -0.5f);

			RenderMesh2D(meshes[meshName], shaders["VertexColor"], m);
		}
	}
}

void Tema1::DrawBall() {
	// Deseneaza bila
	glm::mat3 model = glm::mat3(1);
	float drawRadius = ballRadius * 1.5f;

	model *= transform2D::Translate(ballPosition.x + cameraShakeOffset.x,
		ballPosition.y + cameraShakeOffset.y);
	model *= transform2D::Scale(drawRadius, drawRadius);

	RenderMesh2D(meshes[breakout_ball], shaders["VertexColor"], model);
}

void Tema1::HandleBallBrickCollisions() {
	float radius = ballRadius * 1.5f;
	// Configuratia caramizilor (health, culoare, scara initiala)
	std::vector<std::tuple<int, glm::vec3, float>> brick_config = {
		{1, glm::vec3(0.2f, 0.4f, 0.7f), 1.0f},
		{2, glm::vec3(1.0f, 1.0f, 0.2f), 1.01f},
		{3, glm::vec3(1.0f, 0.2f, 0.2f), 1.02f}
	};
	// Parcurge toate caramizile pentru a verifica coliziunea
	for (int r = 0; r < gameBricksData.size(); r++) {
		for (int c = 0; c < gameBricksData[r].size(); c++) {
			Brick& brick = gameBricksData[r][c];

			if (!brick.isAlive || brick.destroyTimer > 0.0f) continue;
			// Gaseste cel mai apropiat punct pe caramida fata de bila
			float closestPointX = ballPosition.x;
			float closestPointY = ballPosition.y;

			// Determina coordonatele celui mai apropiat punct de pe suprafata caramizii
			if (ballPosition.x < brick.position.x)
				closestPointX = brick.position.x;
			else if (ballPosition.x > brick.position.x + brick.width)
				closestPointX = brick.position.x + brick.width;

			if (ballPosition.y < brick.position.y)
				closestPointY = brick.position.y;
			else if (ballPosition.y > brick.position.y + brick.height)
				closestPointY = brick.position.y + brick.height;

			float distX = ballPosition.x - closestPointX;
			float distY = ballPosition.y - closestPointY;
			float distanceSq = (distX * distX) + (distY * distY);

			// Daca distanta este mai mica decat raza, avem coliziune
			if (distanceSq <= (radius * radius)) {

				// Raspunsul la impact
				glm::vec2 brickCenter = glm::vec2(brick.position.x + brick.width / 2.0f, brick.position.y + brick.height / 2.0f);
				// Activeaza efectul de camera shake si particule
				cameraShakeTimer = cameraShakeDuration;
				TriggerBrickBreakEffect(brickCenter);

				// Logica de distrugere a caramizii
				if (brick.health == 3) {
					brick.health = 2;
					brick.color = std::get<1>(brick_config[1]);
					brick.initialScale = std::get<2>(brick_config[1]);
				}
				else if (brick.health == 2) {
					brick.health = 1;
					brick.color = std::get<1>(brick_config[0]);
					brick.initialScale = std::get<2>(brick_config[0]);
				}
				else if (brick.health == 1) {
					brick.destroyTimer = brick.destroyDuration; // Activeaza animatia de distrugere
					score += 1; // Creste scorul
				}

				// Logica de reflexie a bilei
				float brickCenterX = brick.position.x + brick.width / 2.0f;
				float brickCenterY = brick.position.y + brick.height / 2.0f;

				// Calculeaza distanta dintre centrul bilei si centrul caramizii
				float deltaX = ballPosition.x - brickCenterX;
				float deltaY = ballPosition.y - brickCenterY;

				// Calculeaza scorul de impact ponderat pe fiecare axa (adica pe ce axa ar trebui sa ricoseze bila cand loveste caramida)
				float verticalImpactScore = (brick.width / 2.0f) * deltaY;
				float horizontalImpactScore = (brick.height / 2.0f) * deltaX;

				// Daca axa Y are un scor mai mare, ricoseaza pe Y (Sus/Jos)
				if (verticalImpactScore > horizontalImpactScore) {
					// Impact Sus/Jos (Reflexie Y)
					if (verticalImpactScore > -horizontalImpactScore) {
						ballVelocity.y *= -1; // pe Y (Sus)
						ballPosition.y = brick.position.y + brick.height + radius;
					}
					else {
						ballVelocity.x *= -1; // pe X (Stanga)
						ballPosition.x = brick.position.x - radius;
					}
				}
				// Impact Stanga/Dreapta (Reflexie X)
				else {
					if (verticalImpactScore > -horizontalImpactScore) {
						ballVelocity.x *= -1; // pe X (Dreapta)
						ballPosition.x = brick.position.x + brick.width + radius;
					}
					else {
						ballVelocity.y *= -1; // pe Y (Jos)
						ballPosition.y = brick.position.y - radius;
					}
				}
				return; // Iesim dupa prima coliziune detectata
			}
		}
	}
}





void Tema1::UpdateCameraShake(float deltaTimeSeconds) {
	// Actualizeaza efectul de camera shake
	if (cameraShakeTimer > 0.0f) {
		cameraShakeTimer -= deltaTimeSeconds;
		if (cameraShakeTimer <= 0.0f) {
			cameraShakeOffset = glm::vec2(0.0f, 0.0f);
			return;
		}

		// Calculeaza magnitudinea curenta a shake-ului (scade in timp)
		float timeRatio = cameraShakeTimer / cameraShakeDuration;
		float currentMagnitude = cameraShakeMagnitude * timeRatio;

		// Genereaza un offset aleator pentru shake
		cameraShakeOffset.x = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 2.0f * currentMagnitude;
		cameraShakeOffset.y = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 2.0f * currentMagnitude;
	}
	else {
		cameraShakeOffset = glm::vec2(0.0f, 0.0f);
	}
}

void Tema1::UpdateParticleEffects(float deltaTimeSeconds) {
	// Actualizeaza pozitia si durata de viata a particulelor
	std::vector<Particle> nextParticles;
	for (Particle & p : particles) {
		if (p.life > 0.0f) {
			// Actualizeaza viata si pozitia particulei
			p.life -= deltaTimeSeconds;
			p.position += p.velocity * deltaTimeSeconds;
			nextParticles.push_back(p);
		}
	}
	// Inlocuieste lista de particule cu cele ramase
	particles = nextParticles;
}

void Tema1::TriggerBrickBreakEffect(glm::vec2 centerPosition) {
	// Numarul de particule
	int numToEmit = 5;
	// Viteza maxima a particulelor
	const float MAX_PARTICLE_SPEED = 80.0f;

	// Emite particulele
	for (int i = 0; i < numToEmit; ++i) {
		Particle p;
		p.position = centerPosition;
		// Durata de viata intre 0.5 si 1.0 secunde
		p.initialLife = 0.5f + (static_cast<float>(rand()) / RAND_MAX) * 0.5f;
		p.life = p.initialLife;

		// Genereaza o directie aleatorie pe ambele axe
		float randomFactorX = (static_cast<float>(rand()) / RAND_MAX * 2.0f) - 1.0f;
		float randomFactorY = (static_cast<float>(rand()) / RAND_MAX * 2.0f) - 1.0f;

		// Aplica viteza maxima dorita
		float velX = randomFactorX * MAX_PARTICLE_SPEED;
		float velY = randomFactorY * MAX_PARTICLE_SPEED;

		// Normalizarea vitezei pentru a avea o directie aleatorie
		glm::vec2 randomVelocity = glm::vec2(velX, velY);
		float currentSpeed = glm::length(randomVelocity);

		if (currentSpeed > 0.0f) {
			// Normalizeaza vectorul de viteza
			randomVelocity = glm::normalize(randomVelocity);

			// Genereaza o viteza finala intre 30 si 80 unitati
			float finalSpeed = 30.0f + (static_cast<float>(rand()) / RAND_MAX) * 50.0f;

			// Seteaza viteza particulei
			p.velocity = randomVelocity * finalSpeed;
		}
		else {
			p.velocity = glm::vec2(0.0f, 0.0f); // Evita impartirea la zero
		}

		particles.push_back(p);
	}
}

void Tema1::DrawParticleEffects() {
	// Deseneaza particulele
	for (Particle & p : particles) {
		glm::mat3 m = glm::mat3(1);
		// Pozitioneaza particula, incluzand offset-ul de camera shake
		m *= transform2D::Translate(p.position.x + cameraShakeOffset.x, p.position.y + cameraShakeOffset.y);

		float size = 5.0f * (p.life / p.initialLife);
		m *= transform2D::Scale(size, size);

		RenderMesh2D(meshes[solid_block], shaders["VertexColor"], m);

	}
}