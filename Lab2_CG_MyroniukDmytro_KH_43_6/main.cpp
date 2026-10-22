#define _USE_MATH_DEFINES
#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/glut.h>
#include <cmath>
#include <cstdlib>
#include <vector>

using namespace std;

float toRadians(float degrees)
{
    return (float)(degrees * M_PI / 180.0);
}

struct Color {
    float r, g, b;
};

const Color CUBE_COLOR = { 0.95f, 0.45f, 0.10f };
const Color SPHERE_COLOR = { 0.10f, 0.60f, 0.95f };
const Color SURFACE_FILL = { 0.25f, 0.55f, 0.30f };
const Color SURFACE_GRID = { 0.05f, 0.15f, 0.05f };
const Color BACKGROUND = { 0.93f, 0.93f, 0.95f };

const float FIGURE_OFFSET = 6.0f;
const float CUBE_SIZE = 2.5f;
const float SPHERE_RADIUS = 1.5f;

void setColor(const Color& c)
{
    glColor3f(c.r, c.g, c.b);
}

class Camera {
public:
    Camera() { reset(); }

    void reset()
    {
        yaw = 45.0f;
        pitch = 30.0f;
        distance = 18.0f;
    }

    void rotate(float dYaw, float dPitch)
    {
        yaw = wrap(yaw + dYaw);
        pitch = wrap(pitch + dPitch);
    }

    void zoom(float delta)
    {
        distance += delta;
        if (distance < 4.0f) distance = 4.0f;
        if (distance > 80.0f) distance = 80.0f;
    }

    float getDistance() const { return distance; }

    void apply() const
    {
        float p = pitch;
        if (fabs(cos(toRadians(p))) < 1e-3f) p += 0.01f;

        float yawRad = toRadians(yaw);
        float pitchRad = toRadians(p);

        float eyeX = distance * cos(pitchRad) * sin(yawRad);
        float eyeY = distance * sin(pitchRad);
        float eyeZ = distance * cos(pitchRad) * cos(yawRad);
        float upY = (cos(pitchRad) >= 0.0f) ? 1.0f : -1.0f;

        gluLookAt(eyeX, eyeY, eyeZ,
            0.0, 0.0, 0.0,
            0.0, upY, 0.0);
    }

private:
    float yaw, pitch, distance;

    static float wrap(float angle)
    {
        while (angle >= 360.0f) angle -= 360.0f;
        while (angle < 0.0f) angle += 360.0f;
        return angle;
    }
};

class Surface {
public:
    Surface(int nx, int ny, float xMin, float xMax, float yMin, float yMax)
        : nx(nx), ny(ny)
    {
        float dx = (xMax - xMin) / nx;
        float dy = (yMax - yMin) / ny;
        float yCenter = (yMin + yMax) * 0.5f;

        for (int j = 0; j <= ny; ++j) {
            for (int i = 0; i <= nx; ++i) {
                float x = xMin + i * dx;
                float y = yMin + j * dy;
                vertices.push_back(x);
                vertices.push_back(function(x, y));
                vertices.push_back(y - yCenter);
            }
        }

        for (int j = 0; j < ny; ++j) {
            for (int i = 0; i < nx; ++i) {
                GLuint a = j * (nx + 1) + i;
                GLuint b = a + 1;
                GLuint c = a + (nx + 1);
                GLuint d = c + 1;
                addTriangle(a, c, b);
                addTriangle(b, c, d);
            }
        }
    }

    void draw() const
    {
        drawFill();
        drawGrid();
    }

private:
    int nx, ny;
    vector<GLfloat> vertices;
    vector<GLuint> indices;

    static float function(float x, float y)
    {
        return sin(x) * sqrt(y);
    }

    void addTriangle(GLuint a, GLuint b, GLuint c)
    {
        indices.push_back(a);
        indices.push_back(b);
        indices.push_back(c);
    }

    const GLfloat* vertex(int i, int j) const
    {
        return &vertices[(j * (nx + 1) + i) * 3];
    }

    void drawFill() const
    {
        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(3, GL_FLOAT, 0, vertices.data());

        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(1.0f, 1.0f);
        setColor(SURFACE_FILL);
        glDrawElements(GL_TRIANGLES, (GLsizei)indices.size(), GL_UNSIGNED_INT, indices.data());
        glDisable(GL_POLYGON_OFFSET_FILL);

        glDisableClientState(GL_VERTEX_ARRAY);
    }

    void drawGrid() const
    {
        setColor(SURFACE_GRID);

        for (int j = 0; j <= ny; ++j) {
            glBegin(GL_LINE_STRIP);
            for (int i = 0; i <= nx; ++i) {
                const GLfloat* v = vertex(i, j);
                glVertex3f(v[0], v[1], v[2]);
            }
            glEnd();
        }

        for (int i = 0; i <= nx; ++i) {
            glBegin(GL_LINE_STRIP);
            for (int j = 0; j <= ny; ++j) {
                const GLfloat* v = vertex(i, j);
                glVertex3f(v[0], v[1], v[2]);
            }
            glEnd();
        }
    }
};

Camera camera;
Surface surface(40, 40, (float)-M_PI, (float)M_PI, 0.0f, 6.0f);

int windowWidth = 900;
int windowHeight = 700;
bool perspectiveMode = true;
int displayMode = 1;

void drawCube(bool solid)
{
    glPushMatrix();
    glTranslatef(-FIGURE_OFFSET, 0.0f, 0.0f);
    setColor(CUBE_COLOR);
    if (solid) {
        glEnable(GL_LIGHTING);
        glutSolidCube(CUBE_SIZE);
        glDisable(GL_LIGHTING);
    }
    else {
        glutWireCube(CUBE_SIZE);
    }
    glPopMatrix();
}

void drawSphere(bool solid)
{
    glPushMatrix();
    glTranslatef(FIGURE_OFFSET, 0.0f, 0.0f);
    setColor(SPHERE_COLOR);
    if (solid) {
        glEnable(GL_LIGHTING);
        glutSolidSphere(SPHERE_RADIUS, 32, 24);
        glDisable(GL_LIGHTING);
    }
    else {
        glutWireSphere(SPHERE_RADIUS, 24, 16);
    }
    glPopMatrix();
}

void drawAxes()
{
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    glColor3f(1.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(9.0f, 0.0f, 0.0f);
    glColor3f(0.0f, 1.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 5.0f, 0.0f);
    glColor3f(0.0f, 0.0f, 1.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, 6.0f);
    glEnd();
    glLineWidth(1.0f);
}

void setProjection()
{
    float aspect = (float)windowWidth / (float)windowHeight;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    if (perspectiveMode) {
        float nearPlane = 1.0f;
        float farPlane = 200.0f;
        float top = nearPlane * tan(toRadians(30.0f));
        float right = top * aspect;
        glFrustum(-right, right, -top, top, nearPlane, farPlane);
    }
    else {
        float halfHeight = camera.getDistance() * 0.5f;
        float halfWidth = halfHeight * aspect;
        glOrtho(-halfWidth, halfWidth, -halfHeight, halfHeight, -200.0, 200.0);
    }
}

void setView()
{
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    camera.apply();

    GLfloat lightPosition[] = { 10.0f, 20.0f, 15.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
}

void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    setProjection();
    setView();

    drawAxes();
    surface.draw();
    bool cubeSolid = (displayMode == 1 || displayMode == 4);
    bool sphereSolid = (displayMode == 2 || displayMode == 4);

    drawCube(cubeSolid);
    drawSphere(sphereSolid);

    glutSwapBuffers();
}

void reshape(int width, int height)
{
    windowWidth = width;
    windowHeight = (height == 0) ? 1 : height;
    glViewport(0, 0, windowWidth, windowHeight);
    glutPostRedisplay();
}

void keyboard(unsigned char key, int, int)
{
    const float step = 5.0f;

    switch (key) {
    case 27:
        exit(0);
    case 'p': case 'P':
        perspectiveMode = true;
        break;
    case 'o': case 'O':
        perspectiveMode = false;
        break;
    case '1':
        displayMode = 1;
        break;
    case '2':
        displayMode = 2;
        break;
    case '3':
        displayMode = 3;
        break;
    case '4':
        displayMode = 4;
        break;
    case 'a': case 'A':
        camera.rotate(-step, 0.0f);
        break;
    case 'd': case 'D':
        camera.rotate(step, 0.0f);
        break;
    case 'w': case 'W':
        camera.rotate(0.0f, step);
        break;
    case 's': case 'S':
        camera.rotate(0.0f, -step);
        break;
    case '+': case '=':
        camera.zoom(-1.0f);
        break;
    case '-': case '_':
        camera.zoom(1.0f);
        break;
    case 'r': case 'R':
        camera.reset();
        break;
    default:
        return;
    }

    glutPostRedisplay();
}

void specialKeys(int key, int, int)
{
    const float step = 5.0f;

    switch (key) {
    case GLUT_KEY_LEFT:
        camera.rotate(-step, 0.0f);
        break;
    case GLUT_KEY_RIGHT:
        camera.rotate(step, 0.0f);
        break;
    case GLUT_KEY_UP:
        camera.rotate(0.0f, step);
        break;
    case GLUT_KEY_DOWN:
        camera.rotate(0.0f, -step);
        break;
    default:
        return;
    }

    glutPostRedisplay();
}

void initGL()
{
    glClearColor(BACKGROUND.r, BACKGROUND.g, BACKGROUND.b, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glShadeModel(GL_SMOOTH);

    GLfloat diffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    GLfloat ambient[] = { 0.25f, 0.25f, 0.25f, 1.0f };
    glEnable(GL_LIGHT0);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient); 

    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_NORMALIZE);
}

int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(windowWidth, windowHeight);
    glutCreateWindow("Lab2 Myroniuk Dmytro");

    initGL();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);

    glutMainLoop();
    return 0;
}