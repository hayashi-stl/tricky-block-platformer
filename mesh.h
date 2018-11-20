#ifndef MESH_H_INCLUDED
#define MESH_H_INCLUDED

#define GLEW_STATIC

#include <stdlib.h>
#include <GL/glew.h>
#ifdef __APPLE__
#  include <GLUT/glut.h>
#else
#  include <GL/glut.h>
#  include <GL/freeglut.h>
#endif
#include <stddef.h>
#include <math.h>
#include <stdio.h>
#include <array>
#include <vector>
#include <deque>
#include <unordered_map>
#include <list>
#include <string>
#include <iterator>
#include <algorithm>
#include <memory>
#include <iostream>
#include "Mesh/meshobj.h"
#include "Resources.h"
#include "Math/Vector.h"
#include "Math/Quaternion.h"
#include "util.h"
#include "Mesh/GeneralObj.h"
#include "Loaders/smdldef.h"
#include "Globals.h"

#define TEX_DIFF 0
#define TEX_DIFF_COL 1
#define TEX_ALPHA 2
#define TEX_SPEC 3
#define TEX_SPEC_COL 4
#define TEX_HARDNESS 5
#define TEX_EMIT 6
#define TEX_NORMAL 7

class GeneralObj;

using namespace std;

static GLuint makeStBuffer(GLenum target, const void* bufferData, GLsizei bufferSize)
{
    GLuint buffer;
    glGenBuffers(1, &buffer);
    glBindBuffer(target, buffer);
    glBufferData(target, bufferSize, bufferData, GL_STATIC_DRAW);
    return buffer;
}
static GLuint makeDyBuffer(GLenum target, const void* bufferData, GLsizei bufferSize)
{
    GLuint buffer;
    glGenBuffers(1, &buffer);
    glBindBuffer(target, buffer);
    glBufferData(target, bufferSize, bufferData, GL_STREAM_DRAW);
    return buffer;
}

static GLuint makeTexture(const char* filename)
{
    GLuint texture;
    int width, height;
    void* pixels = read_tga(filename, &width, &height);

    if(!pixels)
        return 0;

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_REPEAT);

    glTexImage2D(GL_TEXTURE_2D, 0,          //target, level of detail
                 GL_RGB8,                   //internal format
                 width, height, 0,          //width, height, border
                 GL_BGR, GL_UNSIGNED_BYTE,  //external format, type
                 pixels);                   //pixels
    free(pixels);
    return texture;
}

class Mesh
{

public:
    Vector<3> pos = Vector<3>(0, 0, 0);
    Quaternion rot = Quaternion(1, 0, 0, 0);
    float fov = tau / 8;

    unordered_map<string, MeshObj> meshes;
    deque<list<shared_ptr<GeneralObj>>> objects;

    //Did elements get deleted or added?
    bool elemChanged = 0;
    bool elemAdded = 0;
    bool elemRemoved = 0;
    forward_list<uint32_t> removals;

    uint32_t numVerts = 0;
    list<vector<Vertex>> vertex;
    unique_ptr<Vertex[]> vertexR;

    uint8_t numImgs = 0;
    deque<string> imgNames;

    uint8_t numTexs = 0;
    deque<uint8_t> imgIDs;

    uint8_t numMats = 0;
    deque<uint8_t> texIDs;
    unique_ptr<uint32_t[]> matIndexSize;

    //Used to store elements at every addition and deletion
    deque<list<uint32_t>> element;
    unique_ptr<uint32_t[]> elementR;
    uint32_t numTris = 0; //number of face _thirds_

    Mesh(){}
    void clear(uint32_t, uint32_t);
    void shiftVertOffsets(uint32_t, uint32_t);
    bool loadMultiMesh(int, string[]);
    void update();
    void setData(uint8_t);
    void render(const vector<uint8_t>&);
};

extern Mesh meshH;

#endif // MESH_H_INCLUDED
