#ifndef RESOURCES_H_INCLUDED
#define RESOURCES_H_INCLUDED
#include <memory>

using namespace std;
class Resources
{
public:
    static const int numShaders = 2;

    uint32_t windowSize[2];

    uint32_t vertexBuffer, elementBuffer;
    unique_ptr<uint32_t[]> textures;
    uint32_t vertexShader[numShaders], fragmentShader[numShaders], program[numShaders];

    enum vertexShadres {REGULAR_V, FONT_V};
    enum fragmentShaders {REGULAR_F, FONT_F};

    struct{
        int32_t timer;
        int32_t textures[8];
        int32_t rot;
        int32_t pos;
        int32_t proj;
    } uniforms[numShaders];

    struct{
        int32_t position;
        int32_t normal;
        int32_t tangent;
        int32_t texCoord;
        int32_t color;
        int32_t diffuse;
        int32_t specular;
        int32_t boneMat[3];
        int32_t mPos;
        int32_t mRot;
        int32_t mScale;
    } attributes[numShaders];

    float pos[3];
    float rot[4];
    float proj[4];
    float timer;
};

extern Resources gResources;

#endif // RESOURCES_H_INCLUDED
