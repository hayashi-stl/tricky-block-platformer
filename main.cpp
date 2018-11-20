#define GLEW_STATIC

#include <stdlib.h>
#include <GL/glew.h>
#ifdef __APPLE__
#  include <GLUT/glut.h>
#else
#  include <GL/glut.h>
#endif
#include <math.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <thread>
#include "mesh.h"
#include "Resources.h"
#include "Math/Matrix.h"
#include <list>
#include "Mesh/GeneralObj.h"
#include "GameManager.h"
#include "Globals.h"

int timer = 0;
int64_t lastTime;

bool meshRemoved = false;
bool meshRemoved2 = false;
int frame = 0, time2, timebase = 0;

static void showInfoLog(GLuint object,
                          PFNGLGETSHADERIVPROC glGet__iv,
                          PFNGLGETSHADERINFOLOGPROC glGet__InfoLog)
{
    GLint logLength;
    char* log;

    glGet__iv(object, GL_INFO_LOG_LENGTH, &logLength);
    log = (char*)malloc(logLength);
    glGet__InfoLog(object, logLength, NULL, log);
    fprintf(stderr, "%s", log);
    free(log);
}

static GLuint makeProgram(GLuint vertexShader, GLuint fragmentShader)
{
    GLint programOK;
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &programOK);
    if (!programOK) {
        fprintf(stderr, "Failed to link shader program:\n");
        showInfoLog(program, glGetProgramiv, glGetProgramInfoLog);
        glDeleteProgram(program);
        return 0;
    }
    return program;
}

static GLuint makeShader(GLenum type, const char* filename)
{
    GLint length;
    GLchar* source = (GLchar*)file_contents(filename, &length);
    GLuint shader;
    GLint shaderOK;

    if(!source)
        return 0;

    shader = glCreateShader(type);
    glShaderSource(shader, 1, (const GLchar**)&source, &length);
    free(source);
    glCompileShader(shader);

    glGetShaderiv(shader, GL_COMPILE_STATUS, &shaderOK);
    if(!shaderOK)
    {
        fprintf(stderr, "Failed to compile %s:\n", filename);
        showInfoLog(shader, glGetShaderiv, glGetShaderInfoLog);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

static int makeResources(const char* vertexShaderFile)
{
    meshH.clear(65535, 65535);
    gm.changeState(GameState::MENU, 0xff, true);

    gResources.vertexShader[Resources::REGULAR_V] = makeShader(GL_VERTEX_SHADER, vertexShaderFile);
    gResources.fragmentShader[Resources::REGULAR_F] = makeShader(GL_FRAGMENT_SHADER, "regular.f.glsl");
    gResources.vertexShader[Resources::FONT_V] = makeShader(GL_VERTEX_SHADER, "Fonts/font.v.glsl");
    gResources.fragmentShader[Resources::FONT_F] = makeShader(GL_FRAGMENT_SHADER, "Fonts/font.f.glsl");

    for(int i = 0; i < Resources::numShaders; ++i)
    {
        if(gResources.vertexShader[i] == 0)
            return 0;
        if(gResources.fragmentShader[i] == 0)
            return 0;
    }

    for(int i = 0; i < Resources::numShaders; ++i)
    {
        gResources.program[i] = makeProgram(gResources.vertexShader[i], gResources.fragmentShader[i]);
        if(gResources.program[i] == 0)
            return 0;
    }

    for(int i = 0; i < Resources::numShaders; ++i)
    {
        gResources.uniforms[i].timer = glGetUniformLocation(gResources.program[i], "timer");

        gResources.uniforms[i].textures[TEX_DIFF]     = glGetUniformLocation(gResources.program[i], "texDiff");
        gResources.uniforms[i].textures[TEX_DIFF_COL] = glGetUniformLocation(gResources.program[i], "texDiffCol");
        gResources.uniforms[i].textures[TEX_ALPHA]    = glGetUniformLocation(gResources.program[i], "texAlpha");
        gResources.uniforms[i].textures[TEX_SPEC]     = glGetUniformLocation(gResources.program[i], "texSpec");
        gResources.uniforms[i].textures[TEX_SPEC_COL] = glGetUniformLocation(gResources.program[i], "texSpecCol");
        gResources.uniforms[i].textures[TEX_HARDNESS] = glGetUniformLocation(gResources.program[i], "texHard");
        gResources.uniforms[i].textures[TEX_EMIT]     = glGetUniformLocation(gResources.program[i], "texEmit");
        gResources.uniforms[i].textures[TEX_NORMAL]   = glGetUniformLocation(gResources.program[i], "texNormal");

        gResources.uniforms[i].pos  = glGetUniformLocation(gResources.program[i], "vPos");
        gResources.uniforms[i].rot  = glGetUniformLocation(gResources.program[i], "vRot");
        gResources.uniforms[i].proj = glGetUniformLocation(gResources.program[i], "proj");

        gResources.attributes[i].position   = glGetAttribLocation(gResources.program[i], "position");
        gResources.attributes[i].normal     = glGetAttribLocation(gResources.program[i], "normal");
        gResources.attributes[i].tangent    = glGetAttribLocation(gResources.program[i], "tangent");
        gResources.attributes[i].texCoord   = glGetAttribLocation(gResources.program[i], "texCoord");
        gResources.attributes[i].color      = glGetAttribLocation(gResources.program[i], "color");
        gResources.attributes[i].diffuse    = glGetAttribLocation(gResources.program[i], "diffuse");
        gResources.attributes[i].specular   = glGetAttribLocation(gResources.program[i], "specular");
        gResources.attributes[i].boneMat[0] = glGetAttribLocation(gResources.program[i], "boneMat0");
        gResources.attributes[i].boneMat[1] = glGetAttribLocation(gResources.program[i], "boneMat1");
        gResources.attributes[i].boneMat[2] = glGetAttribLocation(gResources.program[i], "boneMat2");
        gResources.attributes[i].mPos       = glGetAttribLocation(gResources.program[i], "mPos");
        gResources.attributes[i].mRot       = glGetAttribLocation(gResources.program[i], "mRot");
        gResources.attributes[i].mScale     = glGetAttribLocation(gResources.program[i], "mScale");
    }


    return 1;
}

static void drag(int x, int y)
{
    mousePos[0] = (2.0 * x / gResources.windowSize[0] - 1) * tanf(meshH.fov);
    mousePos[1] = (-2.0 * y / gResources.windowSize[1] + 1) * tanf(meshH.fov) / gResources.windowSize[0] * gResources.windowSize[1];
}
static void mouseMove(int x, int y)
{
    mousePos[0] = (2.0 * x / gResources.windowSize[0] - 1) * tanf(meshH.fov);
    mousePos[1] = (-2.0 * y / gResources.windowSize[1] + 1) * tanf(meshH.fov) / gResources.windowSize[0] * gResources.windowSize[1];
}
static void mouse(int button, int state, int x, int y)
{
    mousePos[0] = (2.0 * x / gResources.windowSize[0] - 1) * tanf(meshH.fov);
    mousePos[1] = (-2.0 * y / gResources.windowSize[1] + 1) * tanf(meshH.fov) / gResources.windowSize[0] * gResources.windowSize[1];
    if(state == GLUT_DOWN)
        mouseStates[button] = 1;
    else
        mouseStates[button] = 0;
}

static void keyPressed(unsigned char key, int x, int y)
{
    keyStates[key] = 1;
}
static void keySPressed(int key, int x, int y)
{
    keySStates[key] = 1;
    keyMods = glutGetModifiers();
}

static void keyReleased(unsigned char key, int x, int y)
{
    keyStates[key] = 0;
}
static void keySReleased(int key, int x, int y)
{
    keySStates[key] = 0;
    keyMods = glutGetModifiers();
}

static void update()
{
    gResources.timer = (float)timer / 30.0f;

    //update key state buffer
    copy(keyStates, keyStates + 256, bufKeyStates);
    copy(keySStates, keySStates + 256, bufKeySStates);
    copy(mouseStates, mouseStates + 3, bufMouseStates);
    copy(mousePos, mousePos + 2, bufMousePos);
    bufKeyMods = keyMods;

    gm.update();
    meshH.update();

    float tanFOV = 1 / tanf(meshH.fov);
    Vector<4> proj (tanFOV, (float)gResources.windowSize[0]/(float)gResources.windowSize[1] * tanFOV, 0.5, 1000.0);
    Vector<3> vPos = meshH.pos;
    Quaternion vRot = meshH.rot;

    vPos.copyV(gResources.pos);
    vRot.copyQ(gResources.rot);
    proj.copyV(gResources.proj);

    ++frame;
	time2=glutGet(GLUT_ELAPSED_TIME);

	if (time2 - timebase > 1000) {
		//std::cout << frame*1000.0/(time2-timebase) << "\n";
	 	timebase = time2;
		frame = 0;
	}

    glutPostRedisplay();
}

static void updateTimer(int t)
{
    timer += 1;
    glutTimerFunc(33, updateTimer, 0);
}

static void reshape(int w, int h)
{
    gResources.windowSize[0] = w;
    gResources.windowSize[1] = h;
    glViewport(0, 0, w, h);
}

static void render()
{
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    vector<uint8_t> currShader;

    if(dynamic_cast<MenuState*>(gm.getState()) ||
       dynamic_cast<LevelSelect*>(gm.getState()))
    {
        currShader.push_back(Resources::FONT_V);
    }
    else
    {
        currShader.push_back(Resources::REGULAR_V);
        currShader.push_back(Resources::FONT_V);
        if(dynamic_cast<LevelState*>(gm.getState()))
            currShader.push_back(4);
        else
            currShader.push_back(5);
    }

    for(uint8_t i = 0; i < currShader.size(); ++i)
    {
        glEnableVertexAttribArray(gResources.attributes[currShader[i]].position);
        glEnableVertexAttribArray(gResources.attributes[currShader[i]].normal);
        glEnableVertexAttribArray(gResources.attributes[currShader[i]].tangent);
        glEnableVertexAttribArray(gResources.attributes[currShader[i]].texCoord);
        glEnableVertexAttribArray(gResources.attributes[currShader[i]].color);
        glEnableVertexAttribArray(gResources.attributes[currShader[i]].diffuse);
        glEnableVertexAttribArray(gResources.attributes[currShader[i]].specular);
        glEnableVertexAttribArray(gResources.attributes[currShader[i]].boneMat[0]);
        glEnableVertexAttribArray(gResources.attributes[currShader[i]].boneMat[1]);
        glEnableVertexAttribArray(gResources.attributes[currShader[i]].boneMat[2]);
        glEnableVertexAttribArray(gResources.attributes[currShader[i]].mPos);
        glEnableVertexAttribArray(gResources.attributes[currShader[i]].mRot);
        glEnableVertexAttribArray(gResources.attributes[currShader[i]].mScale);
    }

    glUseProgram(gResources.program[currShader[0]]);
    meshH.render(currShader);

    for(uint8_t i = 0; i < currShader.size(); ++i)
    {
        glDisableVertexAttribArray(gResources.attributes[currShader[i]].position);
        glDisableVertexAttribArray(gResources.attributes[currShader[i]].normal);
        glDisableVertexAttribArray(gResources.attributes[currShader[i]].tangent);
        glDisableVertexAttribArray(gResources.attributes[currShader[i]].texCoord);
        glDisableVertexAttribArray(gResources.attributes[currShader[i]].color);
        glDisableVertexAttribArray(gResources.attributes[currShader[i]].diffuse);
        glDisableVertexAttribArray(gResources.attributes[currShader[i]].specular);
        glDisableVertexAttribArray(gResources.attributes[currShader[i]].boneMat[0]);
        glDisableVertexAttribArray(gResources.attributes[currShader[i]].boneMat[1]);
        glDisableVertexAttribArray(gResources.attributes[currShader[i]].boneMat[2]);
        glDisableVertexAttribArray(gResources.attributes[currShader[i]].mPos);
        glDisableVertexAttribArray(gResources.attributes[currShader[i]].mRot);
        glDisableVertexAttribArray(gResources.attributes[currShader[i]].mScale);
    }
    glutSwapBuffers();

    //calculate time taken to render last frame (and assume the next will be similar)
    int64_t thisTime = glutGet(GLUT_ELAPSED_TIME); //the higher resolution this is the better
    int64_t deltaTime = thisTime - lastTime;

    //limit framerate by sleeping. a sleep call is never really that accurate
    int64_t sleepTime = 33 - deltaTime; //add difference to desired deltaTime
    sleepTime = sleepTime >= 0 ? sleepTime : 0; //negative sleeping won't make it go faster :(
    this_thread::sleep_for(chrono::milliseconds(sleepTime));
    lastTime = glutGet(GLUT_ELAPSED_TIME); //the higher resolution this is the better
}

int main(int argc, char** argv)
{

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
    gResources.windowSize[0] = 800;
    gResources.windowSize[1] = 600;
    glutInitWindowSize(800, 600);
    glutCreateWindow("Tricky Block Puzzle Game");
    glutReshapeFunc(&reshape);
    glutDisplayFunc(&render);
    glutIdleFunc(&update);

    glutMotionFunc(&drag);
    glutPassiveMotionFunc(&mouseMove);
    glutMouseFunc(&mouse);

    glutKeyboardFunc(&keyPressed);
    glutKeyboardUpFunc(&keyReleased);

    glutSpecialFunc(&keySPressed);
    glutSpecialUpFunc(&keySReleased);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    glewInit();
    if(!GLEW_VERSION_2_0)
    {
        fprintf(stderr, ((string)"OpenGL 2.0 is not available" + (const char*)glGetString(GL_VERSION)).c_str());
        return 1;
    }
    if(!makeResources(argc >= 2 ? argv[1] : "regular.v.glsl"))
    {
        fprintf(stderr, "Resources cannot be made");
        return 1;
    }

    lastTime = glutGet(GLUT_ELAPSED_TIME);
    glutMainLoop();
}
