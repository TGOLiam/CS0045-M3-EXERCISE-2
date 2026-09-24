#define GL_SILENCE_DEPRECATION
#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>

struct Position {
    float x, y;
};

struct Color {
    float r, g, b;
};

enum GameColor {
    RED,
    GREEN,
    BLUE
};

struct Player {
    Position center;
    float size;
    GameColor color;
};

struct Barrier {
    Position center;
    float width;
    float height;
    float velocity;
    GameColor color;
};

const int WINDOW_WIDTH = 700;
const int WINDOW_HEIGHT = 500;
const float PLAY_ASPECT = static_cast<float>(WINDOW_WIDTH) / WINDOW_HEIGHT;

const float BARRIER_START_X = 1.55f;
const float STARTING_BARRIER_SPEED = 0.45f;
const float BARRIER_SPEED_INCREASE = 0.06f;
const float MAX_BARRIER_SPEED = 1.20f;

Player player = {
    Position{0.0f, 0.0f},
    0.22f,
    RED
};

Barrier barrier = {
    Position{BARRIER_START_X, 0.0f},
    0.12f,
    0.70f,
    STARTING_BARRIER_SPEED,
    RED
};

int score = 0;
bool gameRunning = true;
float deltaTime = 0.0f;

Color getColor(GameColor color) {
    switch (color) {
        case RED:   return Color{1.0f, 0.10f, 0.10f};
        case GREEN: return Color{0.10f, 1.0f, 0.20f};
        case BLUE:  return Color{0.15f, 0.35f, 1.0f};
    }

    return Color{1.0f, 1.0f, 1.0f};
}

std::string getColorName(GameColor color) {
    switch (color) {
        case RED:   return "RED";
        case GREEN: return "GREEN";
        case BLUE:  return "BLUE";
    }

    return "UNKNOWN";
}

GameColor randomGameColor() {
    return static_cast<GameColor>(std::rand() % 3);
}

void drawPlayer() {
    Color color = getColor(player.color);
    float halfSize = player.size / 2.0f;

    glColor3f(color.r, color.g, color.b);
    glBegin(GL_QUADS);
        glVertex2f(player.center.x - halfSize, player.center.y - halfSize);
        glVertex2f(player.center.x + halfSize, player.center.y - halfSize);
        glVertex2f(player.center.x + halfSize, player.center.y + halfSize);
        glVertex2f(player.center.x - halfSize, player.center.y + halfSize);
    glEnd();

    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_LINE_LOOP);
        glVertex2f(player.center.x - halfSize, player.center.y - halfSize);
        glVertex2f(player.center.x + halfSize, player.center.y - halfSize);
        glVertex2f(player.center.x + halfSize, player.center.y + halfSize);
        glVertex2f(player.center.x - halfSize, player.center.y + halfSize);
    glEnd();
}

void drawBarrier() {
    Color color = getColor(barrier.color);
    float halfWidth = barrier.width / 2.0f;
    float halfHeight = barrier.height / 2.0f;

    glColor3f(color.r, color.g, color.b);
    glBegin(GL_QUADS);
        glVertex2f(barrier.center.x - halfWidth, barrier.center.y - halfHeight);
        glVertex2f(barrier.center.x + halfWidth, barrier.center.y - halfHeight);
        glVertex2f(barrier.center.x + halfWidth, barrier.center.y + halfHeight);
        glVertex2f(barrier.center.x - halfWidth, barrier.center.y + halfHeight);
    glEnd();

    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_LINE_LOOP);
        glVertex2f(barrier.center.x - halfWidth, barrier.center.y - halfHeight);
        glVertex2f(barrier.center.x + halfWidth, barrier.center.y - halfHeight);
        glVertex2f(barrier.center.x + halfWidth, barrier.center.y + halfHeight);
        glVertex2f(barrier.center.x - halfWidth, barrier.center.y + halfHeight);
    glEnd();
}

void drawBitmapString(float x, float y, const std::string& text) {
    glRasterPos2f(x, y);
    for (char character : text) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, character);
    }
}

void drawDashboard() {
    glColor3f(1.0f, 1.0f, 1.0f);
    drawBitmapString(-1.33f, 0.92f, "Score: " + std::to_string(score));
    drawBitmapString(0.72f, 0.92f,
                     "Player Color: " + getColorName(player.color));
    drawBitmapString(-1.33f, -0.88f, "Controls: R = Red, G = Green, B = Blue");
    drawBitmapString(-1.33f, -0.96f,
                     "Match the incoming barrier. Mismatch = Game Over.");
}

bool colorsMatch() {
    return player.color == barrier.color;
}

void spawnNextBarrier() {
    barrier.center = Position{BARRIER_START_X, 0.0f};
    barrier.color = randomGameColor();
}

void checkGateCollision() {
    float playerHalfSize = player.size / 2.0f;
    float barrierHalfWidth = barrier.width / 2.0f;
    float barrierHalfHeight = barrier.height / 2.0f;

    bool overlapsX =
        player.center.x + playerHalfSize >= barrier.center.x - barrierHalfWidth &&
        player.center.x - playerHalfSize <= barrier.center.x + barrierHalfWidth;
    bool overlapsY =
        player.center.y + playerHalfSize >= barrier.center.y - barrierHalfHeight &&
        player.center.y - playerHalfSize <= barrier.center.y + barrierHalfHeight;

    if (!overlapsX || !overlapsY) {
        return;
    }

    if (colorsMatch()) {
        score++;
        barrier.velocity = std::min(
            barrier.velocity + BARRIER_SPEED_INCREASE,
            MAX_BARRIER_SPEED
        );
        std::cout << "Correct match! Score: " << score
                  << ", speed: " << barrier.velocity << std::endl;
        spawnNextBarrier();
    } else {
        gameRunning = false;
        std::cout << "Color mismatch. GAME OVER. Final score: "
                  << score << std::endl;
    }
}

void resetGame() {
    score = 0;
    gameRunning = true;
    player.color = RED;
    barrier.velocity = STARTING_BARRIER_SPEED;
    spawnNextBarrier();
}

void updateBarrier(int value) {
    (void)value;

    static int previousTime = glutGet(GLUT_ELAPSED_TIME);
    int currentTime = glutGet(GLUT_ELAPSED_TIME);
    deltaTime = (currentTime - previousTime) / 1000.0f;
    previousTime = currentTime;
    deltaTime = std::min(deltaTime, 0.05f);

    if (gameRunning) {
        barrier.center.x -= barrier.velocity * deltaTime;
        checkGateCollision();
    }

    glutPostRedisplay();
    glutTimerFunc(16, updateBarrier, 0);
}

void keyboard(unsigned char key, int x, int y) {
    (void)x;
    (void)y;

    if (!gameRunning) {
        if (key == 'r' || key == 'R') {
            resetGame();
        }
        return;
    }

    switch (key) {
        case 'r': case 'R': player.color = RED;   break;
        case 'g': case 'G': player.color = GREEN; break;
        case 'b': case 'B': player.color = BLUE;  break;
    }

    glutPostRedisplay();
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    if (gameRunning) {
        drawPlayer();
        drawBarrier();
        drawDashboard();
    } else {
        glColor3f(1.0f, 1.0f, 1.0f);
        drawBitmapString(-0.53f, 0.10f, "COLOR MISMATCH - GAME OVER");
        drawBitmapString(-0.27f, 0.00f,
                         "Score: " + std::to_string(score));
        drawBitmapString(-0.39f, -0.10f, "Press R to Restart");
    }

    glutSwapBuffers();
}

void reshape(int width, int height) {
    glViewport(0, 0, width, height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-PLAY_ASPECT, PLAY_ASPECT, -1.0, 1.0, -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutCreateWindow("Chroma-Shift: The Color-Matching Gate");

    std::srand(static_cast<unsigned int>(std::time(nullptr)));
    glClearColor(0.03f, 0.03f, 0.08f, 1.0f);
    resetGame();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutTimerFunc(16, updateBarrier, 0);
    glutMainLoop();
    return 0;
}
