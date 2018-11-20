#ifndef SMDL_H_INCLUDED
#define SMDL_H_INCLUDED
#include <iostream>
#include <fstream>
#include <cstring>
#include <cmath>
#include <string>
#include <vector>
#include <forward_list>
#include <list>
#include <array>
#include <iterator>
#include "Mesh/meshobj.h"
#include "mesh.h"
#include <memory>
#include <iomanip>
#include <algorithm>
#include "Math/Vector.h"
#include "Globals.h"
#include "Loaders/Loader.h"

class Mesh;

struct MatSMDL
{
    float diffuse[4];
    float specular[4];
    float shininess;
    uint8_t influence = 0;
};

struct VertSMDL
{
    float pos[3];
    float normal[3];
    float tangent[3]; // for bump maps
    float texCoord[2];
    uint8_t color[3];
    uint8_t influence;
    float diffuse[4];
    float specular[4];
    float shininess;
    forward_list<uint16_t> vFaces;
    vector<BoneGroup> groups;
    bool operator==(const MatSMDL& a)
    {
        //Floats are compared for strict equality because they were copied
        //and copied floats equal
        for(int i = 0; i < 4; ++i)
        {
            if(diffuse[i]  != a.diffuse[i] ||
               specular[i] != a.specular[i])
                return false;
        }
        return true;
    }
    bool operator==(const VertSMDL& a)
    {
        //Floats are compared for strict equality because they were copied
        //and copied floats equal
        for(int i = 0; i < 3; ++i)
        {
            if(pos[i]    != a.pos[i] ||
               normal[i] != a.normal[i] ||
               color[i]  != a.color[i])
                return false;
        }
        for(int i = 0; i < 2; ++i)
        {
            if(texCoord[i] != a.texCoord[i])
                return false;
        }
        if(influence != a.influence)
            return false;
        //Floats are compared for strict equality because they were copied
        //and copied floats equal
        for(int i = 0; i < 4; ++i)
        {
            if(diffuse[i]  != a.diffuse[i] ||
               specular[i] != a.specular[i])
                return false;
        }
        return true;
    }
};

//Actually, face thirds
struct FaceSMDL
{
    //Floats are compared for strict equality because they were copied
    //and copied floats equal
    float texCoord[2];
    uint8_t color[3];
    uint16_t vertID;
    uint8_t matID;
    bool operator==(const FaceSMDL& a)
    {
        return texCoord[0] == a.texCoord[0] &&
               texCoord[1] == a.texCoord[1] &&
               color[0]    == a.color[0] &&
               color[1]    == a.color[1] &&
               color[2]    == a.color[2] &&
               matID       == a.matID;
    }
};

Vector<3> normalF(VertSMDL* v, FaceSMDL* f, int index);
Vector<3> tangentF(VertSMDL* v, FaceSMDL* f, int index);

bool compareFaces(const FaceSMDL&, const FaceSMDL&);

using namespace std;


string fromSMDLA(MeshObj*, const char*, const char*, int);
string fromSMDL(const char*, const char*, vector<string>[], vector<string>[]);

#endif // SMDL_H_INCLUDED
