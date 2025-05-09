#include <GL/freeglut.h>
#include <cmath>
#include <vector>
#include <string>
#include <sstream>
#include <ctime>
#include <cstdlib>
#include <iomanip>

// Window dimensions
const int WIDTH = 1280;
const int HEIGHT = 720;

// Simulation state
bool isPaused = false;
float timeSpeed = 1.6f;
float cameraSpeed = 1.5f;
float houseSensitivity = 1.0f;

// Camera control
float cameraDistance = 40.0f;
float cameraAngleX = 0.0f;
float cameraAngleY = 0.0f;

// Performance metrics
float frameTime = 0.0f;
float fps = 0.0f;

// Starfield background
const int NUM_STARS = 1000;
struct Star {
    float x, y, z;
    float size;
};
std::vector<Star> stars;

// Planet data
struct Planet {
    std::string name;
    float radius;
    float distance;
    float orbitSpeed;
    float rotationSpeed;
    float red, green, blue;
    bool hasMoon;
    bool isVisible;
    float currentOrbitAngle;
    float currentRotationAngle;
    float axialTilt;
};

std::vector<Planet> planets = {
    {"Sun", 2.0f, 0.0f, 0.0f, 0.8f, 1.0f, 0.7f, 0.1f, false, true, 0.0f, 0.0f, 7.25f},
    {"Mercury", 0.38f, 5.0f, 4.74f, 0.3f, 0.7f, 0.5f, 0.3f, false, true, 0.0f, 0.0f, 0.1f},
    {"Venus", 0.95f, 7.0f, 3.5f, 0.1f, 0.9f, 0.7f, 0.2f, false, true, 0.0f, 0.0f, 177.4f},
    {"Earth", 1.0f, 10.0f, 2.98f, 0.5f, 0.2f, 0.4f, 0.9f, true, true, 0.0f, 0.0f, 23.44f},
    {"Mars", 0.53f, 13.0f, 2.41f, 0.4f, 0.8f, 0.3f, 0.1f, true, true, 0.0f, 0.0f, 25.19f},
    {"Jupiter", 1.12f, 18.0f, 1.31f, 1.2f, 0.8f, 0.6f, 0.4f, true, true, 0.0f, 0.0f, 3.13f},
    {"Saturn", 0.94f, 22.0f, 0.97f, 1.0f, 0.9f, 0.8f, 0.5f, true, true, 0.0f, 0.0f, 26.73f},
    {"Uranus", 0.4f, 26.0f, 0.68f, 0.8f, 0.4f, 0.6f, 0.8f, true, true, 0.0f, 0.0f, 97.77f},
    {"Neptune", 0.39f, 30.0f, 0.54f, 0.7f, 0.2f, 0.3f, 0.9f, true, true, 0.0f, 0.0f, 28.32f},
    {"Pluto", 0.18f, 35.0f, 0.47f, 0.3f, 0.8f, 0.7f, 0.6f, false, true, 0.0f, 0.0f, 122.53f}
};

void initStars() {
    stars.resize(NUM_STARS);
    for (int i = 0; i < NUM_STARS; ++i) {
        stars[i].x = (rand() % 2000 - 1000) / 100.0f;
        stars[i].y = (rand() % 2000 - 1000) / 100.0f;
        stars[i].z = (rand() % 2000 - 1000) / 100.0f;
        stars[i].size = rand() % 3 + 1;
    }
}

void drawOrbitRing(float radius) {
    glDisable(GL_LIGHTING);
    glColor3f(0.3f, 0.3f, 0.3f);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < 360; i += 5) {
        float degInRad = i * 3.14159f / 180.0f;
        glVertex3f(cos(degInRad) * radius, 0, sin(degInRad) * radius);
    }
    glEnd();
    glEnable(GL_LIGHTING);
}

void drawStars() {
    glDisable(GL_LIGHTING);
    glBegin(GL_POINTS);
    for (const auto& star : stars) {
        glColor3f(1.0, 1.0, 1.0);
        glVertex3f(star.x, star.y, star.z);
        glPointSize(star.size);
    }
    glEnd();
    glEnable(GL_LIGHTING);
}

void drawString(float x, float y, const std::string& str) {
    glRasterPos2f(x, y);
    for (char c : str) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, c);
    }
}

void drawPlanetName(const Planet& planet) {
    glDisable(GL_LIGHTING);
    glColor3f(1.0f, 1.0f, 1.0f);

    // Calculate screen position of the planet
    GLdouble modelview[16], projection[16];
    GLint viewport[4];
    GLdouble winX, winY, winZ;

    glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
    glGetDoublev(GL_PROJECTION_MATRIX, projection);
    glGetIntegerv(GL_VIEWPORT, viewport);

    // Get planet position
    GLdouble posX = planet.distance * sin(planet.currentOrbitAngle * 3.14159f / 180.0f);
    GLdouble posZ = planet.distance * cos(planet.currentOrbitAngle * 3.14159f / 180.0f);

    gluProject(posX, 0.0, posZ, modelview, projection, viewport, &winX, &winY, &winZ);

    // Only draw if the planet is in front of other objects
    if (winZ < 1.0) {
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        gluOrtho2D(0, viewport[2], viewport[3], 0);
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();

        // Draw the name slightly above the planet
        glRasterPos2f(winX, viewport[3] - winY - 20);
        for (char c : planet.name) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, c);
        }

        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
    }
    glEnable(GL_LIGHTING);
}

void drawUI() {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, WIDTH, HEIGHT, 0);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_LIGHTING);
    glColor3f(1.0f, 1.0f, 1.0f);

    // Time/Speed section
    drawString(20, 30, "Time / Speed");
    drawString(20, 50, "0.262");
    drawString(20, 70, "3.798");

    // Orbital/Rotation speed
    drawString(20, 100, "Orbital Speed");
    drawString(20, 120, "Rotation Speed");

    // Camera section
    drawString(20, 150, "Camera");
    drawString(20, 170, "0.950");
    drawString(20, 190, "6.888");

    // Speed/House Sens
    drawString(20, 220, "Speed");
    drawString(20, 240, "House Sens");

    // Performance metrics
    drawString(20, 270, "Performance");
    std::ostringstream frameTimeStr;
    frameTimeStr << "Frame time: " << std::fixed << std::setprecision(2) << frameTime << " ms";
    drawString(20, 290, frameTimeStr.str());
    std::ostringstream fpsStr;
    fpsStr << "FPS: " << std::fixed << std::setprecision(2) << fps;
    drawString(20, 310, fpsStr.str());

    // Play/Pause button
    drawString(20, 340, isPaused ? "Play" : "Pause");

    // Planet list
    drawString(WIDTH - 150, 30, "- Plane: Banders");
    for (size_t i = 0; i < planets.size(); ++i) {
        std::string planetEntry = "- " + planets[i].name;
        if (!planets[i].isVisible) planetEntry += " (hidden)";
        drawString(WIDTH - 150, 50 + i * 20, planetEntry);
    }

    glEnable(GL_LIGHTING);
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

void drawPlanet(const Planet& planet) {
    if (!planet.isVisible) return;

    glPushMatrix();

    if (planet.name != "Sun") {
        glRotatef(planet.currentOrbitAngle, 0.0f, 1.0f, 0.0f);
        glTranslatef(planet.distance, 0.0f, 0.0f);
        drawOrbitRing(planet.distance);
    }


    glRotatef(planet.axialTilt, 0.0f, 0.0f, 1.0f);
    glRotatef(planet.currentRotationAngle, 0.0f, 1.0f, 0.0f);

    if (planet.name == "Sun") {
        glDisable(GL_LIGHTING);
        glColor3f(1.0f, 0.9f, 0.2f);
        glutSolidSphere(planet.radius * 0.9, 40, 40);
        glColor3f(1.0f, 0.7f, 0.1f);
        glutSolidSphere(planet.radius * 1.0, 36, 36);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(1.0f, 0.5f, 0.1f, 0.5f);
        glutSolidSphere(planet.radius * 1.2, 32, 32);
        glDisable(GL_BLEND);
        glEnable(GL_LIGHTING);
    } else {
        GLfloat matShininess[] = {50.0f};
        GLfloat matSpecular[] = {0.3f, 0.3f, 0.3f, 1.0f};
        glMaterialfv(GL_FRONT, GL_SPECULAR, matSpecular);
        glMaterialfv(GL_FRONT, GL_SHININESS, matShininess);

        glColor3f(planet.red, planet.green, planet.blue);
        glutSolidSphere(planet.radius, 40, 40);

        if (planet.name == "Earth" || planet.name == "Neptune") {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glColor4f(planet.red, planet.green, planet.blue, 0.3f);
            glutSolidSphere(planet.radius * 1.05, 30, 30);
            glDisable(GL_BLEND);
        }
    }

    if (planet.name == "Saturn") {
        glPushMatrix();
        glRotatef(25.0f, 1.0f, 0.0f, 0.0f);
        glColor3f(0.8f, 0.8f, 0.6f);
        glutSolidTorus(0.2f, planet.radius * 1.8, 20, 20);
        glPopMatrix();
    }

    glPopMatrix();


    if (planet.name != "Sun") {
        drawPlanetName(planet);
    }
}

void update(int value) {
    if (!isPaused) {
        for (auto& planet : planets) {
            planet.currentOrbitAngle += planet.orbitSpeed * timeSpeed * 0.016f;
            planet.currentRotationAngle += planet.rotationSpeed * timeSpeed * 0.48f;

            if (planet.currentOrbitAngle > 360.0f) planet.currentOrbitAngle -= 360.0f;
            if (planet.currentRotationAngle > 360.0f) planet.currentRotationAngle -= 360.0f;
        }
    }

    glutPostRedisplay();
    glutTimerFunc(16, update, 0);
}

void display() {
    auto startTime = glutGet(GLUT_ELAPSED_TIME);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Draw starfield background
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluPerspective(60.0, (double)WIDTH/(double)HEIGHT, 0.1, 200.0);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    drawStars();
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    // Main solar system view
    glLoadIdentity();
    gluLookAt(cameraDistance * std::sin(cameraAngleY),
              cameraDistance * std::sin(cameraAngleX),
              cameraDistance * std::cos(cameraAngleY),
              0.0f, 0.0f, 0.0f,
              0.0f, 1.0f, 0.0f);

    // Enhanced lighting with golden color
    GLfloat sunLight[] = {1.0f, 0.8f, 0.4f, 1.0f};
    glLightfv(GL_LIGHT0, GL_DIFFUSE, sunLight);

    for (const auto& planet : planets) {
        drawPlanet(planet);
    }

    drawUI();

    glutSwapBuffers();

    auto endTime = glutGet(GLUT_ELAPSED_TIME);
    frameTime = (endTime - startTime) / 1000.0f;
    fps = 1.0f / frameTime;
}

void reshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, (float)w / (float)h, 0.1f, 100.0f);
    glMatrixMode(GL_MODELVIEW);
}

void keyboard(unsigned char key, int x, int y) {
    switch (key) {
        case ' ': isPaused = !isPaused; break;
        case 'w': cameraAngleX += 0.05f * cameraSpeed; break;
        case 's': cameraAngleX -= 0.05f * cameraSpeed; break;
        case 'a': cameraAngleY += 0.05f * cameraSpeed; break;
        case 'd': cameraAngleY -= 0.05f * cameraSpeed; break;
        case '+': cameraDistance -= 0.5f * cameraSpeed; break;
        case '-': cameraDistance += 0.5f * cameraSpeed; break;
        case '1': timeSpeed *= 0.9f; break;
        case '2': timeSpeed *= 1.1f; break;
        case 27: exit(0); break;
    }


    if (cameraDistance < 10.0f) cameraDistance = 10.0f;
    if (cameraDistance > 50.0f) cameraDistance = 50.0f;
    if (timeSpeed < 0.1f) timeSpeed = 0.1f;
    if (timeSpeed > 10.0f) timeSpeed = 10.0f;
}

void specialKeys(int key, int x, int y) {
    switch (key) {
        case GLUT_KEY_UP: houseSensitivity *= 1.1f; break;
        case GLUT_KEY_DOWN: houseSensitivity *= 0.9f; break;
    }
}

void mouse(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        if (x >= 20 && x <= 100 && y >= HEIGHT - 340 && y <= HEIGHT - 320) {
            isPaused = !isPaused;
        }

        if (x >= WIDTH - 150 && x <= WIDTH - 20) {
            int planetIndex = (y - (HEIGHT - 50)) / 20;
            if (planetIndex >= 0 && planetIndex < static_cast<int>(planets.size())) {
                planets[planetIndex].isVisible = !planets[planetIndex].isVisible;
            }
        }
    }
}

void init() {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);

    initStars();

    GLfloat lightPosition[] = {0.0f, 0.0f, 0.0f, 1.0f};
    GLfloat lightAmbient[] = {0.2f, 0.15f, 0.05f, 1.0f};
    GLfloat lightDiffuse[] = {1.0f, 0.8f, 0.4f, 1.0f};
    GLfloat lightSpecular[] = {1.0f, 0.9f, 0.6f, 1.0f};
    glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpecular);

    glEnable(GL_POINT_SMOOTH);
    glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(WIDTH, HEIGHT);
    glutCreateWindow("3D Solar System with Orbits and Labels");

    init();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    glutMouseFunc(mouse);
    glutTimerFunc(0, update, 0);

    glutMainLoop();
    return 0;
}
