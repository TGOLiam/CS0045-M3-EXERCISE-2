#define GL_SILENCE_DEPRECATION
#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <string>
#include <vector>

#define PI 3.14159265358979323846f
#define FULL_TURN (2.0f * PI)

struct Position {
    float x, y;
};

struct Color {
    float r, g, b;
};

enum BurstType {
    CONCENTRIC_SQUARES = 1,
    DIAMOND_LATTICE = 2,
    STARBURST_TRIANGLES = 3
};

struct Burst {
    Position center;
    Color color;
    BurstType type;
    float radius;
    float expansionSpeed;
    float brightness;
    float age;
    float lifetime;
    float rotation;
};

const int WINDOW_WIDTH = 700;
const int WINDOW_HEIGHT = 500;
const float PLAY_ASPECT = static_cast<float>(WINDOW_WIDTH) / WINDOW_HEIGHT;
const int TIMER_INTERVAL_MS = 16;

std::vector<Burst> bursts;
BurstType currentMode = CONCENTRIC_SQUARES;
bool timerRunning = false;
int previousUpdateTime = 0;

void updateBursts(int value);

float randomFloat(float minimum, float maximum) {
    float amount = static_cast<float>(std::rand()) /
                   static_cast<float>(RAND_MAX);
    return minimum + amount * (maximum - minimum);
}

Color randomBrightColor() {
    return Color{
        randomFloat(0.45f, 1.0f),
        randomFloat(0.45f, 1.0f),
        randomFloat(0.45f, 1.0f)
    };
}

std::string getModeName(BurstType type) {
    switch (type) {
        case CONCENTRIC_SQUARES:  return "Squares";
        case DIAMOND_LATTICE:     return "Diamonds";
        case STARBURST_TRIANGLES: return "Triangles";
    }

    return "Unknown";
}

void startAnimationTimer() {
    if (timerRunning) {
        return;
    }

    timerRunning = true;
    previousUpdateTime = glutGet(GLUT_ELAPSED_TIME);
    glutTimerFunc(TIMER_INTERVAL_MS, updateBursts, 0);
}

void spawnBurst(BurstType type, float x, float y) {
    bursts.push_back({
        Position{x, y},
        randomBrightColor(),
        type,
        0.03f,
        randomFloat(0.28f, 0.42f),
        1.0f,
        0.0f,
        randomFloat(1.8f, 2.5f),
        randomFloat(0.0f, 45.0f)
    });

    startAnimationTimer();
    glutPostRedisplay();
}

void spawnRandomBurst(BurstType type) {
    // Keep bursts out of the right sidebar and away from the HUD edges.
    float x = randomFloat(-1.18f, 0.58f);
    float y = randomFloat(-0.76f, 0.76f);
    spawnBurst(type, x, y);
}

void setBurstColor(const Burst& burst) {
    glColor3f(
        burst.color.r * burst.brightness,
        burst.color.g * burst.brightness,
        burst.color.b * burst.brightness
    );
}

void drawRotatedSquare(Position center, float halfSize, float angleDegrees) {
    float angle = angleDegrees * PI / 180.0f;
    float cosine = cosf(angle);
    float sine = sinf(angle);

    const float corners[4][2] = {
        {-halfSize, -halfSize},
        { halfSize, -halfSize},
        { halfSize,  halfSize},
        {-halfSize,  halfSize}
    };

    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < 4; i++) {
        float rotatedX = corners[i][0] * cosine - corners[i][1] * sine;
        float rotatedY = corners[i][0] * sine + corners[i][1] * cosine;
        glVertex2f(center.x + rotatedX, center.y + rotatedY);
    }
    glEnd();
}

void drawDiamond(Position center, float radius) {
    glBegin(GL_LINE_LOOP);
        glVertex2f(center.x,          center.y + radius);
        glVertex2f(center.x + radius, center.y);
        glVertex2f(center.x,          center.y - radius);
        glVertex2f(center.x - radius, center.y);
    glEnd();
}

void drawConcentricSquares(const Burst& burst) {
    for (int i = 1; i <= 4; i++) {
        float scale = static_cast<float>(i) / 4.0f;
        drawRotatedSquare(
            burst.center,
            burst.radius * scale,
            burst.rotation + burst.age * 35.0f * (i % 2 == 0 ? -1.0f : 1.0f)
        );
    }
}

void drawDiamondLattice(const Burst& burst) {
    drawDiamond(burst.center, burst.radius);
    drawDiamond(burst.center, burst.radius * 0.55f);

    float offset = burst.radius * 0.72f;
    float smallRadius = burst.radius * 0.26f;
    drawDiamond(Position{burst.center.x + offset, burst.center.y}, smallRadius);
    drawDiamond(Position{burst.center.x - offset, burst.center.y}, smallRadius);
    drawDiamond(Position{burst.center.x, burst.center.y + offset}, smallRadius);
    drawDiamond(Position{burst.center.x, burst.center.y - offset}, smallRadius);
}

void drawStarburstTriangles(const Burst& burst) {
    const int triangleCount = 12;
    float innerRadius = burst.radius * 0.30f;
    float outerRadius = burst.radius;
    float halfWidthAngle = 0.10f;

    glBegin(GL_TRIANGLES);
    for (int i = 0; i < triangleCount; i++) {
        float angle = i * FULL_TURN / triangleCount + burst.age * 0.8f;

        glVertex2f(
            burst.center.x + cosf(angle) * outerRadius,
            burst.center.y + sinf(angle) * outerRadius
        );
        glVertex2f(
            burst.center.x + cosf(angle - halfWidthAngle) * innerRadius,
            burst.center.y + sinf(angle - halfWidthAngle) * innerRadius
        );
        glVertex2f(
            burst.center.x + cosf(angle + halfWidthAngle) * innerRadius,
            burst.center.y + sinf(angle + halfWidthAngle) * innerRadius
        );
    }
    glEnd();
}

void drawBurst(const Burst& burst) {
    setBurstColor(burst);

    switch (burst.type) {
        case CONCENTRIC_SQUARES:  drawConcentricSquares(burst);  break;
        case DIAMOND_LATTICE:     drawDiamondLattice(burst);     break;
        case STARBURST_TRIANGLES: drawStarburstTriangles(burst); break;
    }
}

void drawBitmapString(float x, float y, const std::string& text) {
    glRasterPos2f(x, y);
    for (char character : text) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, character);
    }
}

void drawSidebar() {
    glColor3f(0.07f, 0.07f, 0.14f);
    glBegin(GL_QUADS);
        glVertex2f(0.76f, -1.0f);
        glVertex2f(PLAY_ASPECT, -1.0f);
        glVertex2f(PLAY_ASPECT, 1.0f);
        glVertex2f(0.76f, 1.0f);
    glEnd();

    glColor3f(0.35f, 0.45f, 0.70f);
    glBegin(GL_LINES);
        glVertex2f(0.76f, -1.0f);
        glVertex2f(0.76f, 1.0f);
    glEnd();

    glColor3f(1.0f, 1.0f, 1.0f);
    drawBitmapString(0.82f, 0.88f, "BURST STUDIO");
    drawBitmapString(0.82f, 0.68f, "1  Squares");
    drawBitmapString(0.82f, 0.57f, "2  Diamonds");
    drawBitmapString(0.82f, 0.46f, "3  Triangles");
    drawBitmapString(0.82f, 0.30f, "SPACE Repeat");
    drawBitmapString(0.82f, 0.05f,
                     "Active: " + std::to_string(bursts.size()));
    drawBitmapString(0.82f, -0.08f,
                     "Mode: " + getModeName(currentMode));
}

void updateBursts(int value) {
    (void)value;

    int currentTime = glutGet(GLUT_ELAPSED_TIME);
    float deltaTime = (currentTime - previousUpdateTime) / 1000.0f;
    previousUpdateTime = currentTime;
    deltaTime = std::min(deltaTime, 0.05f);

    for (Burst& burst : bursts) {
        burst.age += deltaTime;
        burst.radius += burst.expansionSpeed * deltaTime;
        burst.brightness = std::max(0.0f, 1.0f - burst.age / burst.lifetime);
    }

    bursts.erase(
        std::remove_if(
            bursts.begin(),
            bursts.end(),
            [](const Burst& burst) {
                return burst.age >= burst.lifetime;
            }
        ),
        bursts.end()
    );

    glutPostRedisplay();

    if (!bursts.empty()) {
        glutTimerFunc(TIMER_INTERVAL_MS, updateBursts, 0);
    } else {
        timerRunning = false;
    }
}

void keyboard(unsigned char key, int x, int y) {
    (void)x;
    (void)y;

    switch (key) {
        case '1':
            currentMode = CONCENTRIC_SQUARES;
            spawnRandomBurst(currentMode);
            break;
        case '2':
            currentMode = DIAMOND_LATTICE;
            spawnRandomBurst(currentMode);
            break;
        case '3':
            currentMode = STARBURST_TRIANGLES;
            spawnRandomBurst(currentMode);
            break;
        case ' ':
            spawnRandomBurst(currentMode);
            break;
    }
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    for (const Burst& burst : bursts) {
        drawBurst(burst);
    }

    drawSidebar();
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
    glutCreateWindow("Dynamic Fireworks & Shape Burst Studio");

    std::srand(static_cast<unsigned int>(std::time(nullptr)));
    glClearColor(0.01f, 0.01f, 0.04f, 1.0f);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutMainLoop();
    return 0;
}
