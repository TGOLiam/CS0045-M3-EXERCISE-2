#define GL_SILENCE_DEPRECATION
#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>
#include <vector>

#define PI M_PI
#define FULL_TURN (2.0f * PI)

struct Position {
    float x, y;
};

struct Color {
    float r, g, b;
};

enum Direction {
    STOPPED,
    UP,
    LEFT,
    DOWN,
    RIGHT
};

struct Ship {
    Position center;
    Color color;
    float size;
    float velocity;
    Direction direction;
};

struct Asteroid {
    Position center;
    Color color;
    float size;
    float velocity;
};

const float MAX_VELOCITY = 1.50f;
const float ACCELERATION = 3.00f;
const float DECELERATION = 3.00f;
const float ASTEROID_RADIUS = 0.10f;
const float ASTEROID_SPAWN_INTERVAL = 1.0f;
const float SHIP_COLLISION_RADIUS = 0.14f;
const int MAX_ASTEROIDS = 10;
const int WINDOW_WIDTH = 700;
const int WINDOW_HEIGHT = 500;
const float PLAY_ASPECT = static_cast<float>(WINDOW_WIDTH) / WINDOW_HEIGHT;

Ship ship;
std::vector<Asteroid> asteroids;

int lives = 3;
int score = 0;
bool gameRunning = true;
bool thrusting = false;
float deltaTime = 0.0f;
float asteroidSpawnTimer = 0.0f;

float randomAsteroidX() {
    return -1.25f +
           (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX)) * 2.50f;
}

Asteroid makeAsteroid(float velocity) {
    return {
        Position{randomAsteroidX(), 1.05f},
        Color{1.0f, 1.0f, 1.0f},
        ASTEROID_RADIUS,
        velocity
    };
}

void resetAsteroid(Asteroid& asteroid) {
    asteroid.center.x = randomAsteroidX();
    asteroid.center.y = 1.05f;
}

void drawCircle(Position center, float radius, Color color) {
    const int sides = 40;

    glColor3f(color.r, color.g, color.b);
    glBegin(GL_TRIANGLE_FAN);
        glVertex2f(center.x, center.y);
        for (int i = 0; i <= sides; i++) {
            float angle = i * FULL_TURN / sides;
            glVertex2f(
                center.x + radius * cosf(angle),
                center.y + radius * sinf(angle)
            );
        }
    glEnd();
}

void drawShip(Position center, Color color) {
    glColor3f(color.r, color.g, color.b);
    glBegin(GL_POLYGON);
        glVertex2f(center.x - 0.14f, center.y - 0.06f);
        glVertex2f(center.x + 0.14f, center.y - 0.06f);
        glVertex2f(center.x,         center.y - 0.16f);
    glEnd();

    drawCircle(center, 0.10f, color);
}

void drawAsteroid(Position center, float radius, Color color) {
    glColor3f(color.r, color.g, color.b);
    glBegin(GL_LINE_LOOP);
        glVertex2f(center.x - radius * 0.70f, center.y + radius * 0.65f);
        glVertex2f(center.x - radius * 0.05f, center.y + radius * 0.95f);
        glVertex2f(center.x + radius * 0.72f, center.y + radius * 0.60f);
        glVertex2f(center.x + radius * 0.92f, center.y - radius * 0.10f);
        glVertex2f(center.x + radius * 0.45f, center.y - radius * 0.78f);
        glVertex2f(center.x - radius * 0.25f, center.y - radius * 0.90f);
        glVertex2f(center.x - radius * 0.88f, center.y - radius * 0.35f);
        glVertex2f(center.x - radius * 0.95f, center.y + radius * 0.18f);
    glEnd();
}

void drawBitmapString(float x, float y, const std::string& text) {
    glRasterPos2f(x, y);
    for (char character : text) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, character);
    }
}

void drawHUD() {
    glColor3f(1.0f, 1.0f, 1.0f);
    drawBitmapString(-1.33f, 0.92f, "Score: " + std::to_string(score));
    drawBitmapString(1.03f, 0.92f, "Lives: " + std::to_string(lives));
}

bool checkCollision(const Ship& player, const Asteroid& asteroid) {
    float xDistance = player.center.x - asteroid.center.x;
    float yDistance = player.center.y - asteroid.center.y;
    float minimumDistance = SHIP_COLLISION_RADIUS + asteroid.size;

    return xDistance * xDistance + yDistance * yDistance <=
           minimumDistance * minimumDistance;
}

void checkShipAsteroidCollisions() {
    for (Asteroid& asteroid : asteroids) {
        if (!checkCollision(ship, asteroid)) {
            continue;
        }

        lives--;
        std::cout << "Ship hit! Lives remaining: " << lives << std::endl;
        resetAsteroid(asteroid);

        if (lives <= 0) {
            lives = 0;
            gameRunning = false;
            thrusting = false;
            ship.direction = STOPPED;
            ship.velocity = 0.0f;
            std::cout << "GAME OVER" << std::endl;
        }

        // Only one life can be lost during a single update.
        return;
    }
}

void removeOutOfBoundsAsteroids() {
    for (std::size_t i = 0; i < asteroids.size();) {
        if (asteroids[i].center.y + asteroids[i].size < -1.0f) {
            asteroids.erase(asteroids.begin() + i);
            score += 10;
        } else {
            i++;
        }
    }
}

void resetGame() {
    lives = 3;
    score = 0;
    gameRunning = true;
    thrusting = false;
    asteroidSpawnTimer = 0.0f;

    ship = {
        Position{0.0f, -0.60f},
        Color{1.0f, 1.0f, 1.0f},
        1.0f,
        0.0f,
        STOPPED
    };

    asteroids.clear();
    asteroids.push_back(makeAsteroid(0.25f));
    asteroids.push_back(makeAsteroid(0.55f));
}

void updateGame(int value) {
    (void)value;

    static int previousTime = glutGet(GLUT_ELAPSED_TIME);
    int currentTime = glutGet(GLUT_ELAPSED_TIME);
    deltaTime = (currentTime - previousTime) / 1000.0f;
    previousTime = currentTime;
    deltaTime = fminf(deltaTime, 0.05f);

    if (gameRunning) {
        if (thrusting && ship.direction != STOPPED) {
            ship.velocity += ACCELERATION * deltaTime;
            ship.velocity = fminf(ship.velocity, MAX_VELOCITY);
        } else if (ship.velocity > 0.0f) {
            ship.velocity -= DECELERATION * deltaTime;

            if (ship.velocity <= 0.0f) {
                ship.velocity = 0.0f;
                ship.direction = STOPPED;
            }
        }

        switch (ship.direction) {
            case UP:    ship.center.y += ship.velocity * deltaTime; break;
            case LEFT:  ship.center.x -= ship.velocity * deltaTime; break;
            case DOWN:  ship.center.y -= ship.velocity * deltaTime; break;
            case RIGHT: ship.center.x += ship.velocity * deltaTime; break;
            case STOPPED: break;
        }

        ship.center.x = fmaxf(-1.26f, fminf(ship.center.x, 1.26f));
        ship.center.y = fmaxf(-0.84f, fminf(ship.center.y, 0.84f));

        asteroidSpawnTimer += deltaTime;
        if (asteroidSpawnTimer >= ASTEROID_SPAWN_INTERVAL &&
            static_cast<int>(asteroids.size()) < MAX_ASTEROIDS) {
            float speed = 0.25f +
                (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX)) * 0.40f;
            asteroids.push_back(makeAsteroid(speed));
            asteroidSpawnTimer = 0.0f;
        }

        for (Asteroid& asteroid : asteroids) {
            asteroid.center.y -= asteroid.velocity * deltaTime;
        }

        checkShipAsteroidCollisions();
        removeOutOfBoundsAsteroids();
    }

    glutPostRedisplay();
    glutTimerFunc(16, updateGame, 0);
}

void keyboard(unsigned char key, int x, int y) {
    (void)x;
    (void)y;

    if ((key == 'r' || key == 'R') && !gameRunning) {
        resetGame();
        return;
    }

    if (!gameRunning) {
        return;
    }

    switch (key) {
        case 'w': case 'W': ship.direction = UP;    thrusting = true; break;
        case 'a': case 'A': ship.direction = LEFT;  thrusting = true; break;
        case 's': case 'S': ship.direction = DOWN;  thrusting = true; break;
        case 'd': case 'D': ship.direction = RIGHT; thrusting = true; break;
    }
}

void keyboardUp(unsigned char key, int x, int y) {
    (void)x;
    (void)y;

    Direction releasedDirection = STOPPED;
    switch (key) {
        case 'w': case 'W': releasedDirection = UP;    break;
        case 'a': case 'A': releasedDirection = LEFT;  break;
        case 's': case 'S': releasedDirection = DOWN;  break;
        case 'd': case 'D': releasedDirection = RIGHT; break;
    }

    if (ship.direction == releasedDirection) {
        thrusting = false;
    }
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    if (gameRunning) {
        drawShip(ship.center, ship.color);
        for (const Asteroid& asteroid : asteroids) {
            drawAsteroid(asteroid.center, asteroid.size, asteroid.color);
        }
        drawHUD();
    } else {
        glColor3f(1.0f, 1.0f, 1.0f);
        drawBitmapString(-0.68f, 0.0f, "GAME OVER - Press R to Restart");
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
    glutCreateWindow("Space Survival Game");

    std::srand(static_cast<unsigned int>(std::time(nullptr)));
    glClearColor(0.02f, 0.02f, 0.08f, 1.0f);
    resetGame();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutKeyboardUpFunc(keyboardUp);
    glutTimerFunc(16, updateGame, 0);
    glutMainLoop();
    return 0;
}
