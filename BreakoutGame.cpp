#include "BreakoutGame.h"

// Include-uri necesare
#include "components/simple_scene.h"
#include "transform2D.h"
#include <cmath>
#include <iostream>
#include <algorithm>
#include <cstdlib> 
#include <ctime> 

using namespace m1;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

//-----------------------------------------------------------------------------
// 🧩 CONSTRUCTORI & METODE PUBLICE
//-----------------------------------------------------------------------------

BreakoutGame::BreakoutGame() {
    isBallLaunched = false;
    currentLives = 2;
    score = 0;
    paddleCurrentCenterX = 0.0f; // Inițializare
    paddleMovementInputX = 0.0f;

    // Inițializare pentru a evita erorile la prima rulare
    cameraShakeTimer = 0.0f;
    cameraShakeOffset = glm::vec2(0.0f);

    // Inițializare random (pentru TriggerBrickBreakEffect)
    srand(static_cast<unsigned int>(time(0)));
}

BreakoutGame::~BreakoutGame() {
}

void BreakoutGame::Update(float deltaTimeSeconds, glm::ivec2 resolution) {
    UpdateCameraShake(deltaTimeSeconds);
    UpdateParticleEffects(deltaTimeSeconds);
    RunBreakoutGameLoop(deltaTimeSeconds, resolution);
}

void BreakoutGame::Draw(gfxc::SimpleScene* scene, float deltaTimeSeconds) {
    // Apelăm funcțiile de desenare, folosind 'scene' pentru RenderMesh2D
    DrawGameScene(scene);
    DrawBricks(scene, deltaTimeSeconds);
    DrawBall(scene);
    DrawParticleEffects(scene);
}

void BreakoutGame::ResetGame(float newPaddleCenterX) {
    // Setează starea inițială și poziția paletei
    score = 0;
    currentLives = 2;
    isBallLaunched = false;
    paddleCurrentCenterX = newPaddleCenterX;

    // Rezoluția trebuie să fie pasată la ResetGame pentru a calcula dimensiunile
    // Sau putem să o pasăm din Tema1.cpp la apelul SetupGameBricks.
    // De dragul simplității, o pasăm prin parametru în Tema1.cpp
    // Dar o scoatem de aici, deoarece nu o avem ca parametru.
    // Presupunem că SetupGameBricks va fi apelată separat dacă e nevoie de rezoluție.

    ResetBallAndPaddlePosition();
    cameraShakeTimer = 0.0f;
    cameraShakeOffset = glm::vec2(0.0f);
    particles.clear();
}

void BreakoutGame::SetPaddleMovement(float inputX) {
    paddleMovementInputX = inputX;
}

void BreakoutGame::LaunchBall(float speed) {
    if (!isBallLaunched) {
        isBallLaunched = true;
        ballSpeed = speed;

        // Lansare la 45 de grade în sus și oblic
        float velMagnitude = ballSpeed / (float)sqrt(2.0);
        float velX = velMagnitude;
        float velY = velMagnitude;

        ballVelocity = glm::vec2(velX, velY);
    }
}

//-----------------------------------------------------------------------------
// 🎮 LOGICA JOCULUI BREAKOUT (Mutată)
//-----------------------------------------------------------------------------

void BreakoutGame::RunBreakoutGameLoop(float deltaTimeSeconds, glm::ivec2 resolution) {

    // 1. VERIFICARE CONDIȚIE DE VICTORIE
    int remainingBricks = 0;
    for (const auto& row : gameBricksData) {
        for (const auto& brick : row) {
            if (brick.isAlive) {
                remainingBricks++;
            }
        }
    }

    if (remainingBricks == 0) {
        // Semnalizează Tema1 că jocul s-a terminat
        currentLives = -1; // Folosim o valoare pentru a semnaliza că Tema1 trebuie să treacă la Editor
        return;
    }

    // 2. Aplică mișcarea paletei
    UpdatePaddleMovement(resolution);

    // 3. Mișcare bilă
    if (!isBallLaunched) {
        ballPosition.x = paddleCurrentCenterX;
    }
    else {
        ballPosition += ballVelocity * deltaTimeSeconds;
    }

    // 4. Coliziuni bilă-cărămizi
    HandleBallBrickCollisions();

    // 5. Coliziuni bilă-margini
    glm::ivec2 res = resolution;
    float ball_collision_radius = ballRadius * 1.5f;

    // Coliziuni Margini Orizontale
    if (ballPosition.x - ball_collision_radius < 0) {
        ballPosition.x = ball_collision_radius;
        ballVelocity.x *= -1;
    }
    if (ballPosition.x + ball_collision_radius > res.x) {
        ballPosition.x = res.x - ball_collision_radius;
        ballVelocity.x *= -1;
    }

    // Coliziune Tavan
    if (ballPosition.y + ball_collision_radius > res.y) {
        ballPosition.y = res.y - ball_collision_radius;
        ballVelocity.y *= -1;
    }

    // Coliziune Podea / Pierdere Viață
    if (ballPosition.y - ball_collision_radius < 0) {
        currentLives--;
        if (currentLives < 0) {
            // Semnalizează Tema1 că jocul s-a terminat
        }
        else {
            ResetBallAndPaddlePosition();
        }
    }

    // 6. Coliziune Paletă
    float paddle_half_width = paddleWidth / 2.0f;
    float paddle_min_x = paddleCurrentCenterX - paddle_half_width;
    float paddle_max_x = paddleCurrentCenterX + paddle_half_width;

    // Notă: paddleTopYBoundary este setată din Tema1.cpp (dar nu folosim gridCellSize aici)
    float scaledCellSize = paddleWidth * 0.1f; // Estimare, pentru a folosi doar membrii BreakoutGame
    float paddle_min_y = paddleTopYBoundary - scaledCellSize;
    float paddle_max_y = paddleTopYBoundary;

    if (ballPosition.x + ball_collision_radius > paddle_min_x &&
        ballPosition.x - ball_collision_radius < paddle_max_x &&
        ballPosition.y - ball_collision_radius < paddle_max_y &&
        ballPosition.y > paddle_min_y)
    {
        if (ballVelocity.y < 0) {
            ballVelocity.y *= -1;
            ballPosition.y = paddle_max_y + ball_collision_radius;

            float relative_x = (ballPosition.x - paddleCurrentCenterX) / paddle_half_width;
            float max_bounce_angle = M_PI / 3.0f;
            float bounce_angle = relative_x * max_bounce_angle;
            float velMagnitude = glm::length(ballVelocity);
            if (velMagnitude < ballSpeed * 0.9f) velMagnitude = ballSpeed;

            ballVelocity.x = velMagnitude * (float)sin(bounce_angle);
            ballVelocity.y = velMagnitude * (float)cos(bounce_angle);

            if (ballVelocity.y < 0) ballVelocity.y *= -1;
        }
    }
}

void BreakoutGame::ResetBallAndPaddlePosition() {
    float ballDrawRadius = ballRadius * 1.5f;
    // Nu folosim gridCellSize, folosim paddleWidth pentru a calcula o înălțime relativă
    float estimatedPaddleHeight = paddleWidth * 0.2f;

    if (paddleTopYBoundary > 0) {
        initialBallPosition = glm::vec2(
            paddleCurrentCenterX,
            paddleTopYBoundary + (ballRadius * 0.25f) + 2.0f
        );
    }
    else {
        // Fallback dacă paddleTopYBoundary nu e setat
        initialBallPosition = glm::vec2(paddleCurrentCenterX, estimatedPaddleHeight + ballRadius + 2.0f);
    }

    ballPosition = initialBallPosition;
    ballVelocity = glm::vec2(0, 0);
    isBallLaunched = false;
}

void BreakoutGame::UpdatePaddleMovement(glm::ivec2 resolution) {
    // Aplică input-ul și limitează poziția
    float newX = paddleCurrentCenterX + paddleMovementInputX;
    paddleCurrentCenterX = glm::clamp(newX, paddleMinXBoundary, paddleMaxXBoundary);
}

void BreakoutGame::SetupGameBricks(glm::ivec2 resolution) {
    gameBricksData.clear();
    float screenWidth = (float)resolution.x;

    float paddingX = 5.0f;
    float paddingY = 5.0f;

    int numRows = 3;
    int numCols = 15;

    float totalWidthForBricks = screenWidth - (numCols + 1) * paddingX;
    float brickWidth = totalWidthForBricks / numCols;

    float brickZoneTopY = resolution.y * 0.85f;
    float brickZoneBottomY = resolution.y * 0.60f;

    float totalHeightForBricks = brickZoneTopY - brickZoneBottomY - (numRows + 1) * paddingY;
    float brickHeight = totalHeightForBricks / numRows;

    float currentBottomY = brickZoneBottomY + paddingY;

    float totalGridWidth = numCols * brickWidth + (numCols + 1) * paddingX;
    float startX = (screenWidth - totalGridWidth) / 2.0f + paddingX;


    std::vector<std::tuple<int, glm::vec3, float>> brick_config = {
        {1, glm::vec3(0.2f, 0.4f, 0.7f), 1.0f},
        {2, glm::vec3(1.0f, 1.0f, 0.2f), 1.01f},
        {3, glm::vec3(1.0f, 0.2f, 0.2f), 1.02f}
    };

    for (int r = 0; r < numRows; r++) {
        std::vector<Brick> row;

        int health = std::get<0>(brick_config[r]);
        glm::vec3 color = std::get<1>(brick_config[r]);
        float initialScale = std::get<2>(brick_config[r]);

        float yPos_BL = currentBottomY + r * (brickHeight + paddingY) + paddingY;

        for (int c = 0; c < numCols; c++) {
            float xPos_BL = startX + c * (brickWidth + paddingX);

            row.push_back(Brick(
                glm::vec2(xPos_BL, yPos_BL),
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

// ... Coliziuni, Draw-uri, Shake și Particule (implementări complete)

void BreakoutGame::HandleBallBrickCollisions() {
    float radius = ballRadius * 1.5f;

    // Configurarea Health/Culoare
    std::vector<std::tuple<int, glm::vec3, float>> brick_config = {
        {1, glm::vec3(0.2f, 0.4f, 0.7f), 1.0f},
        {2, glm::vec3(1.0f, 1.0f, 0.2f), 1.01f},
        {3, glm::vec3(1.0f, 0.2f, 0.2f), 1.02f}
    };

    for (int r = 0; r < gameBricksData.size(); r++) {
        for (int c = 0; c < gameBricksData[r].size(); c++) {
            Brick& brick = gameBricksData[r][c];

            if (!brick.isAlive || brick.destroyTimer > 0.0f) continue;

            float testX = ballPosition.x;
            float testY = ballPosition.y;

            if (ballPosition.x < brick.position.x)
                testX = brick.position.x;
            else if (ballPosition.x > brick.position.x + brick.width)
                testX = brick.position.x + brick.width;

            if (ballPosition.y < brick.position.y)
                testY = brick.position.y;
            else if (ballPosition.y > brick.position.y + brick.height)
                testY = brick.position.y + brick.height;

            float distX = ballPosition.x - testX;
            float distY = ballPosition.y - testY;
            float distanceSq = (distX * distX) + (distY * distY);

            if (distanceSq <= (radius * radius)) {
                glm::vec2 brickCenter = glm::vec2(brick.position.x + brick.width / 2.0f, brick.position.y + brick.height / 2.0f);
                cameraShakeTimer = cameraShakeDuration;
                TriggerBrickBreakEffect(brickCenter);

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
                    brick.destroyTimer = brick.destroyDuration;
                    score += 1;
                }

                float brickCenterX = brick.position.x + brick.width / 2.0f;
                float brickCenterY = brick.position.y + brick.height / 2.0f;

                float dx = ballPosition.x - brickCenterX;
                float dy = ballPosition.y - brickCenterY;

                float wy = (brick.width / 2.0f) * dy;
                float hx = (brick.height / 2.0f) * dx;

                if (wy > hx) {
                    if (wy > -hx) { // TOP
                        ballVelocity.y *= -1;
                        ballPosition.y = brick.position.y + brick.height + radius;
                    }
                    else { // LEFT
                        ballVelocity.x *= -1;
                        ballPosition.x = brick.position.x - radius;
                    }
                }
                else {
                    if (wy > -hx) { // RIGHT
                        ballVelocity.x *= -1;
                        ballPosition.x = brick.position.x + brick.width + radius;
                    }
                    else { // BOTTOM
                        ballVelocity.y *= -1;
                        ballPosition.y = brick.position.y - radius;
                    }
                }
                return;
            }
        }
    }
}

void BreakoutGame::UpdateCameraShake(float deltaTimeSeconds) {
    if (cameraShakeTimer > 0.0f) {
        cameraShakeTimer -= deltaTimeSeconds;
        if (cameraShakeTimer <= 0.0f) {
            cameraShakeOffset = glm::vec2(0.0f, 0.0f);
            return;
        }

        float timeRatio = cameraShakeTimer / cameraShakeDuration;
        float currentMagnitude = cameraShakeMagnitude * timeRatio;

        cameraShakeOffset.x = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 2.0f * currentMagnitude;
        cameraShakeOffset.y = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 2.0f * currentMagnitude;
    }
    else {
        cameraShakeOffset = glm::vec2(0.0f, 0.0f);
    }
}

void BreakoutGame::UpdateParticleEffects(float deltaTimeSeconds) {
    std::vector<Particle> nextParticles;
    for (auto& p : particles) {
        if (p.life > 0.0f) {
            p.life -= deltaTimeSeconds;
            p.position += p.velocity * deltaTimeSeconds;
            nextParticles.push_back(p);
        }
    }
    particles = nextParticles;
}

void BreakoutGame::TriggerBrickBreakEffect(glm::vec2 centerPosition) {
    int numToEmit = 5;
    for (int i = 0; i < numToEmit; ++i) {
        Particle p;
        p.position = centerPosition;
        p.initialLife = 0.5f + (static_cast<float>(rand()) / RAND_MAX) * 0.5f;
        p.life = p.initialLife;

        float speed = 30.0f + (static_cast<float>(rand()) / RAND_MAX) * 50.0f;
        float angle = (static_cast<float>(rand()) / RAND_MAX) * 2.0f * M_PI;
        p.velocity = glm::vec2(cos(angle) * speed, sin(angle) * speed);

        particles.push_back(p);
    }
}

//-----------------------------------------------------------------------------
// ✏️ METODE DE DESENARE
//-----------------------------------------------------------------------------

void BreakoutGame::DrawGameScene(gfxc::SimpleScene* scene) {
    // În BreakoutGame nu avem acces la gridLayout, deci nu putem desena paleta
    // folosind aceeași logică complexă din Tema1. Putem doar desena un singur bloc.

    // Vom desena doar un dreptunghi simplu pentru paletă, bazat pe starea sa curentă.
    // Presupunem că un mesh generic numit "paddle_mesh" este creat în Tema1.

    float halfWidth = paddleWidth / 2.0f;
    float paddleHeight = paddleWidth * 0.2f; // Folosim o proporție fixă
    const float FIXED_BOTTOM_Y = 0.1f * scene->window->GetResolution().y;

    glm::mat3 model = glm::mat3(1);
    model *= transform2D::Translate(cameraShakeOffset.x, cameraShakeOffset.y);
    model *= transform2D::Translate(paddleCurrentCenterX - halfWidth, FIXED_BOTTOM_Y);
    model *= transform2D::Scale(paddleWidth, paddleHeight);

    // Folosim un mesh generic (presupunem că e definit în Tema1::CreateMeshes)
    // Înlocuiți 'mesh_solid_block' cu un nume de mesh simplu pentru paletă dacă aveți unul.
    scene->RenderMesh2D(scene->GetMesh(mesh_solid_block), scene->GetShader("VertexColor"), model);
}

void BreakoutGame::DrawBall(gfxc::SimpleScene* scene) {
    glm::mat3 model = glm::mat3(1);
    float drawRadius = ballRadius * 1.5f;

    model *= transform2D::Translate(ballPosition.x + cameraShakeOffset.x,
        ballPosition.y + cameraShakeOffset.y);
    model *= transform2D::Scale(drawRadius, drawRadius);

    scene->RenderMesh2D(scene->GetMesh(mesh_game_ball_circle), scene->GetShader("VertexColor"), model);
}

void BreakoutGame::DrawBricks(gfxc::SimpleScene* scene, float deltaTimeSeconds) {
    std::vector<std::tuple<int, std::string, float>> brick_mesh_config = {
        {1, mesh_game_dark_blue_brick, 1.0f},
        {2, mesh_game_yellow_brick, 1.01f},
        {3, mesh_game_red_brick, 1.02f}
    };

    for (auto& row : gameBricksData) {
        for (auto& brick : row) {
            if (!brick.isAlive) continue;

            std::string meshName;
            float currentScale = brick.initialScale;

            if (brick.destroyTimer > 0.0f) {
                brick.destroyTimer -= deltaTimeSeconds;
                if (brick.destroyTimer <= 0.0f) {
                    brick.isAlive = false;
                    continue;
                }

                float progress = brick.destroyTimer / brick.destroyDuration;
                currentScale = brick.initialScale * progress;
                meshName = mesh_game_dark_blue_brick;
            }
            else {
                if (brick.health == 3) {
                    meshName = mesh_game_red_brick;
                }
                else if (brick.health == 2) {
                    meshName = mesh_game_yellow_brick;
                }
                else if (brick.health == 1) {
                    meshName = mesh_game_dark_blue_brick;
                }
                else {
                    continue;
                }
            }

            float halfWidth = brick.width / 2.0f;
            float halfHeight = brick.height / 2.0f;

            glm::mat3 model = glm::mat3(1);
            model *= transform2D::Translate(cameraShakeOffset.x, cameraShakeOffset.y);
            model *= transform2D::Translate(brick.position.x, brick.position.y);
            model *= transform2D::Translate(halfWidth, halfHeight);
            model *= transform2D::Scale(currentScale, currentScale);
            model *= transform2D::Translate(-halfWidth, -halfHeight);
            model *= transform2D::Scale(brick.width, brick.height);

            scene->RenderMesh2D(scene->GetMesh(meshName), scene->GetShader("VertexColor"), model);
        }
    }
}

void BreakoutGame::DrawParticleEffects(gfxc::SimpleScene* scene) {
    for (const auto& p : particles) {
        glm::mat3 model = glm::mat3(1);

        model *= transform2D::Translate(p.position.x + cameraShakeOffset.x, p.position.y + cameraShakeOffset.y);

        float size = 5.0f * (p.life / p.initialLife);
        model *= transform2D::Scale(size, size);

        scene->RenderMesh2D(scene->GetMesh(mesh_solid_block), scene->GetShader("VertexColor"), model);
    }
}