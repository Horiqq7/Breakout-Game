#pragma once

#include "components/simple_scene.h"
#include <vector>
#include <queue>
#include <tuple>
#include <string>
#include "utils/glm_utils.h"

namespace m1 {

    // Structurile raman aici (Brick si Particle)
    struct Brick {
        glm::vec2 position;
        float width;
        float height;
        int health;
        bool isAlive;
        glm::vec3 color;
        float initialScale;
        float destroyTimer;
        float destroyDuration = 0.3f;

        Brick(glm::vec2 pos, float w, float h, int hp, glm::vec3 col, bool alive, float scale, float timer)
            : position(pos), width(w), height(h), health(hp), isAlive(alive), color(col), initialScale(scale), destroyTimer(timer) {
        }
    };

    struct Particle {
        glm::vec2 position;
        glm::vec2 velocity;
        float life;
        float initialLife;
    };

    class BreakoutGame {
    public:
        BreakoutGame();
        ~BreakoutGame();

        // ----------------------------------------------------------------------
        // Interfață publică (folosită de Tema1)
        // ----------------------------------------------------------------------
        void Update(float deltaTimeSeconds, glm::ivec2 resolution);

        // NOU: Pentru a rezolva erorile de desenare, Draw va primi direct Tema1* (SimpleScene*)
        // și va folosi funcții ajutătoare publice pe Tema1.
        void Draw(gfxc::SimpleScene* scene, float deltaTimeSeconds);

        void ResetGame(float paddleCenterX);
        void SetPaddleMovement(float inputX);
        void LaunchBall(float speed);
        void SetupGameBricks(glm::ivec2 resolution); // Mutat în public

        // Variabile de stare publică (setate/citite de Tema1)
        bool isBallLaunched = false;
        int currentLives = 2;
        int score = 0;

        // Membrii Paletei și Bilei (mutat în public pentru accesul din Tema1)
        float paddleWidth = 100.0f;
        float paddleTopYBoundary = 0.0f;
        float paddleMovementSpeed = 300.0f;
        float paddleCurrentCenterX = 0.0f;
        float paddleMinXBoundary = 0.0f;
        float paddleMaxXBoundary = 0.0f;
        float ballSpeed = 400.0f;

    private:
        // ----------------------------------------------------------------------
        // Logică internă (rămâne private)
        // ----------------------------------------------------------------------
        void RunBreakoutGameLoop(float deltaTimeSeconds, glm::ivec2 resolution);
        void UpdatePaddleMovement(glm::ivec2 resolution);
        void HandleBallBrickCollisions();
        void ResetBallAndPaddlePosition();

        // Metode de desenare (actualizate pentru a folosi pointerul scene)
        void DrawGameScene(gfxc::SimpleScene* scene);
        void DrawBall(gfxc::SimpleScene* scene);
        void DrawBricks(gfxc::SimpleScene* scene, float deltaTimeSeconds);
        void DrawParticleEffects(gfxc::SimpleScene* scene);
        void UpdateCameraShake(float deltaTimeSeconds);
        void UpdateParticleEffects(float deltaTimeSeconds);
        void TriggerBrickBreakEffect(glm::vec2 centerPosition);


        // Stare internă
        float ballRadius = 5.0f;
        glm::vec2 ballPosition;
        glm::vec2 ballVelocity;
        glm::vec2 initialBallPosition;
        std::vector<std::vector<Brick>> gameBricksData;
        glm::vec2 paddleCenter; // Centrul constructiv
        float paddleMovementInputX;

        // Efecte
        float cameraShakeDuration = 0.15f;
        float cameraShakeMagnitude = 8.0f;
        float cameraShakeTimer = 0.0f;
        glm::vec2 cameraShakeOffset;
        std::vector<Particle> particles;

        // Nume Mesh
        const std::string mesh_game_ball_circle = "game_ball_circle";
        const std::string mesh_solid_block = "solid_block_pattern";
        const std::string mesh_game_red_brick = "game_red_brick";
        const std::string mesh_game_yellow_brick = "game_yellow_brick";
        const std::string mesh_game_dark_blue_brick = "game_dark_blue_brick";
    };
}