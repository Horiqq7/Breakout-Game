#pragma once

#include "components/simple_scene.h"
#include <components/text_renderer.h>
#include <vector>
#include <queue>
#include <tuple>

namespace gfxc {
	class TextRenderer;
}

namespace m1 {
	enum class BlockType {
		NONE = 0,
		SOLID = 1 // Tipul de bloc folosit in editor
	};

	// Structura care defineste o caramida din jocul Breakout
	struct Brick {
		glm::vec2 position;
		float width;
		float height;
		int health;
		bool isAlive;
		glm::vec3 color;

		float initialScale;
		float destroyTimer;
		float destroyDuration = 0.25f; // Durata animatiei de disparitie

		Brick(glm::vec2 pos, float w, float h, int hp, glm::vec3 col, bool alive, float scale, float timer)
			: position(pos), width(w), height(h), health(hp), isAlive(alive), color(col), initialScale(scale), destroyTimer(timer) {
		}
	};

	class Tema1 : public gfxc::SimpleScene {
	public:
		Tema1();
		~Tema1();

		void Init() override;

	private:
		// Metodele esentiale apelate de motorul de joc (FrameStart, Update, FrameEnd)
		void FrameStart() override;
		void Update(float deltaTimeSeconds) override;
		void FrameEnd() override;

		// Metodele care gestioneaza interactiunile directe cu tastatura si mouse-ul
		void OnInputUpdate(float deltaTime, int mods) override;
		void OnKeyPress(int key, int mods) override;
		void OnKeyRelease(int key, int mods) override;
		void OnMouseMove(int mouseX, int mouseY, int deltaX, int deltaY) override;
		void OnMouseBtnPress(int mouseX, int mouseY, int button, int mods) override;
		void OnMouseBtnRelease(int mouseX, int mouseY, int button, int mods) override;
		void OnMouseScroll(int mouseX, int mouseY, int offsetX, int offsetY) override;

	private:
		// Metoda care creeaza si incarca toate mesh-urile necesare editorului si jocului
		void CreateMeshes();

		// Metodele care deseneaza componentele vizuale ale editorului
		void DrawEditor();
		void DrawGridBlueCells();
		void DrawBlocksToChooseFromEditor();
		void DrawGreenSquaresEditor();
		void DrawStartGameButton();
		void DrawGhostBlock(int mx, int my);
		void DrawRedSeparatorLine();
		void DrawBlueGridFrame();

		// Metode pentru plasarea blocurilor in editor si verificarea constrangerilor
		int SelectBlockFromEditor(int mouseX, int mouseY);
		BlockType GetBlockType(int row);

		glm::ivec2 GetGridCoordinatesFromMouse(int mouseX, int mouseY); // Pentru a determina celula din grila sub mouse
		bool IsInsideGridBounds(int gx, int gy);

		bool CheckAllConstraints();
		bool CheckConnectivityConstraint(); // Verifica daca toate blocurile sunt conectate


		void CalculateEditorLayout(); // Calculeaza pozitiile si dimensiunile layout-ului
		bool isEditorModeActive;
		void TransitionToBreakoutGame();

	private:
		// Metode pentru jocul Breakout si initializarea acestuia
		void ResetBreakoutGame();
		void RunBreakoutGameLoop(float deltaTimeSeconds);
		void DrawGameHUD();
		void DrawPaddle();

		// Calculeaza latimea, inaltimea si limitele rachetei (Paddle)
		void CalculatePaddleGeometryAndBounds();

		void ResetBallAndPaddlePosition(); // Reseteaza bila pe racheta
		void SetupGameBricks(); // Configureaza caramizile de nivel
		void DrawBricks(float deltaTimeSeconds);
		void DrawBall();
		void HandleBallBrickCollisions(); // Gestioneaza coliziunile bila-caramida

		// Metode pentru gestionarea efectelor vizuale
		void UpdateCameraShake(float deltaTimeSeconds);
		void UpdateParticleEffects(float deltaTimeSeconds);
		void TriggerBrickBreakEffect(glm::vec2 centerPosition);
		void DrawParticleEffects();

	protected:
		int gridCols, gridRows;
		glm::ivec2 gridCellOrigin;
		float gridCellSize;
		float gridSpacing;
		float solidBlockScale; // Factor de scalare pentru blocul din editor

		std::vector<std::vector<BlockType>> gridLayout;
		int blocksCount; // Numarul de blocuri plasate
		int solidBlockSize;
		glm::ivec2 solidBlockOrigin;
		glm::vec2 solidBlockPositionInEditor;
		bool isDraggingBlock;
		BlockType draggingBlockType;
		glm::ivec2 currentMousePosition;

		// Butoanele de start
		int startButtonX;
		int startButtonY;
		int startButtonSize;
		const int maxAllowedBlocks = 10;
		// Numele mesh-urilor folosite la randare
		std::string solid_block,
			green_square_editor,
			green_start_button,
			red_start_button,
			blue_cell_editor,
			red_line_separator,
			blue_grid_frame_editor,
			breakout_ball,
			yellow_brick,
			red_brick,
			dark_blue_brick;

		float ballRadius = 5.0f;
		glm::vec2 ballPosition;
		glm::vec2 ballVelocity;
		float ballSpeed = 400.0f;
		bool isBallLaunched;
		glm::vec2 initialBallPosition; // Pozitia pe racheta

		glm::vec2 paddleCenter;
		float paddleTopYBoundary; // Marginea de sus a rachetei
		float paddleMovementInputX;
		float paddleMovementSpeed = 300.0f;
		float paddleCurrentCenterX; // Pozitia X curenta (controlata de jucator)
		float paddleMinXBoundary; // Limita de miscare in stanga
		float paddleMaxXBoundary; // Limita de miscare in dreapta
		float paddleWidth;
		float paddleHeight;

		int initialLivesCount;
		int currentLives;
		int score;
		std::vector<std::vector<Brick>> gameBricksData; // Matricea cu caramizile de nivele diferite

		
		float cameraShakeDuration;
		float cameraShakeMagnitude;
		float cameraShakeTimer;
		glm::vec2 cameraShakeOffset; // Decalajul camerei

		// Particule
		struct Particle {
			glm::vec2 position;
			glm::vec2 velocity;
			float life;
			float initialLife;
		};
		std::vector<Particle> particles;

		gfxc::TextRenderer* textRenderer;
	};
}