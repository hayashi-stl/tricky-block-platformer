#include "mesh.h"

using namespace std;

Mesh meshH = Mesh();

void Mesh::clear(uint32_t maxVerts, uint32_t maxTris)
{
    //Get rid of old data
    objects.clear();

    numImgs = 0;
    imgNames.clear();

    numTexs = 0;
    imgIDs.clear();

    numMats = 0;
    texIDs.clear();

    meshes.clear();

    numVerts = 0;
    vertex.clear();

    numTris = 0;
    element.clear();

    vertexR = unique_ptr<Vertex[]>(new Vertex[maxVerts]);
    elementR = unique_ptr<uint32_t[]>(new uint32_t[maxTris]);
    matIndexSize = unique_ptr<uint32_t[]>(new uint32_t[512]);

    //Initialize this to 0 so the number of elements is not
    //accidentally in the millions and FreeGLUT doesn't whine :)
    fill(&matIndexSize[0], &matIndexSize[0] + 512, 0);

    elemChanged = true;
    elemRemoved = true;

    gResources.vertexBuffer = makeDyBuffer(GL_ARRAY_BUFFER,
                                         vertexR.get(),
                                         maxVerts * sizeof(struct Vertex));
    gResources.elementBuffer = makeDyBuffer(GL_ELEMENT_ARRAY_BUFFER,
                                          elementR.get(),
                                          maxTris * sizeof(GLuint));
}

bool Mesh::loadMultiMesh(int num, string filename[])
{

    //Read multiple meshes
    for(int i = 0; i < num; ++i)
    {
        //If there is an error reading the file, tell it!
        string error = fromSMDLDEF(filename[i].c_str());
        if(!error.empty())
        {
            cout << (string)"Error loading " + filename[i] + ": " + error << "\n\0";
            return 0;
        }
    }

    gResources.textures = unique_ptr<GLuint[]>(new GLuint[numImgs]);

    //For loop increasing 2 variables at once
    int i = 0;
    for(auto it = imgNames.begin(); it != imgNames.end(); ++it)
    {
        gResources.textures[i] = makeTexture((*it).c_str());

        if(gResources.textures[i] == 0)
            return 0;

        ++i;
    }

    return 1;
}

void Mesh::shiftVertOffsets(uint32_t vertOffset, uint32_t amount)
{
    //First, shift the first vertex index in all meshes of all objects
    for(auto it = objects.begin(); it != objects.end(); ++it)
    {
        for(auto jt = it->begin(); jt != it->end(); ++jt)
        {
            for(auto kt = (*jt)->models.begin(); kt != (*jt)->models.end(); ++kt)
            {
                if(kt->firstVert > vertOffset)
                    kt->firstVert -= amount;
            }
        }
    }
    //Second, shift the vertex offset of all elements
    for(auto it = element.begin(); it != element.end(); ++it)
    {
        for(auto jt = it->begin(); jt != it->end(); ++jt)
        {
            if(*jt > vertOffset)
                *jt -= amount;
        }
    }
}

void Mesh::update()
{

    //Make it translate and rotate according to key presses
    /*if(keySStates[GLUT_KEY_UP])
        pos += Vector<3>(0, 1/30.0, 0);
    if(keySStates[GLUT_KEY_DOWN])
        pos += Vector<3>(0, -1/30.0, 0);
    if(keySStates[GLUT_KEY_LEFT])
        pos += Vector<3>(-1/30.0, 0, 0);
    if(keySStates[GLUT_KEY_RIGHT])
        pos += Vector<3>(1/30.0, 0, 0);
    if(keyStates['='])
        pos += Vector<3>(0, 0, 1/30.0);
    if(keyStates['-'])
        pos += Vector<3>(0, 0, -1/30.0);

    if(keyStates['a'])
        rot = Quaternion::Y(-tau / 120.0) * rot;
    if(keyStates['d'])
        rot = Quaternion::Y(tau / 120.0) * rot;
    if(keyStates['w'])
        rot = Quaternion::X(-tau / 120.0) * rot;
    if(keyStates['s'])
        rot = Quaternion::X(tau / 120.0) * rot;
    if(keyStates['z'])
        rot = Quaternion::Z(-tau / 120.0) * rot;
    if(keyStates['x'])
        rot = Quaternion::Z(tau / 120.0) * rot;

    if(keyStates[','] && fov < tau / 6.0)
        fov += tau / 1800.0;
    if(keyStates['.'] && fov > tau / 360.0)
        fov -= tau / 1800.0;*/

    //Finally, copy stuff to buffer
    uint32_t vertIndex = 0;
    for_each(vertex.begin(), vertex.end(), [this, &vertIndex](vector<Vertex> meshVerts)
    {
        copy(meshVerts.begin(), meshVerts.end(), vertexR.get() + vertIndex);
        vertIndex += meshVerts.size();
    });
    glBindBuffer(GL_ARRAY_BUFFER, gResources.vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, numVerts * sizeof(struct Vertex), vertexR.get(), GL_STREAM_DRAW);

    if(elemChanged)
    {
        if(elemRemoved)
        {
            for(auto it = element.begin(); it != element.end(); ++it)
                it->clear();
            for_each(objects.begin(), objects.end(), [&](list<shared_ptr<GeneralObj>> objList)
            {
                for_each(objList.begin(), objList.end(), [&](shared_ptr<GeneralObj> obj)
                {
                    for_each(obj->models.begin(), obj->models.end(), [&](GeneralObj::Model model)
                    {
                        uint32_t offset = model.usedMesh->matOffset;
                        for(int i = 0; i < model.usedMesh->numMats; ++i)
                        {
                            uint32_t offA = model.usedMesh->matIndexSize[2 * i + 0];
                            uint32_t offB = offA + model.usedMesh->matIndexSize[2 * i + 1];
                            for_each(model.usedMesh->element.get() + offA, model.usedMesh->element.get() + offB, [&, i, offset](uint32_t vertID)
                            {
                                element[i + offset].push_back(vertID + model.firstVert);
                            });
                        }
                    });
                });
            });
        }
        //Copy elements and keep track of material indexes and sizes
        uint32_t triCount = 0;
        uint8_t matPos = 0;
        for(auto i = element.begin(); i != element.end(); ++i)
        {
            uint32_t matIndex = triCount;
            for(auto j = i->begin(); j != i->end(); ++j)
            {
                elementR[triCount] = (*j);
                ++triCount;
            }
            matIndexSize[2 * matPos    ] = matIndex;
            matIndexSize[2 * matPos + 1] = triCount - matIndex;
            ++matPos;
        }
        glBindBuffer(GL_ARRAY_BUFFER, gResources.elementBuffer);
        glBufferData(GL_ARRAY_BUFFER, numTris * sizeof(uint32_t), elementR.get(), GL_STREAM_DRAW);
    }

    //reset bit telling if elements have been added or deleted
    elemChanged = 0;
    elemAdded = 0;
    elemRemoved = 0;

}

void Mesh::setData(uint8_t currShader)
{
    glUniform1f(gResources.uniforms[currShader].timer, gResources.timer);
    glUniform3fv(gResources.uniforms[currShader].pos, 1, gResources.pos);
    glUniform4fv(gResources.uniforms[currShader].rot, 1, gResources.rot);
    glUniform4fv(gResources.uniforms[currShader].proj, 1, gResources.proj);

    glVertexAttribPointer(gResources.attributes[currShader].position,  //attribute
                          4,                                //size
                          GL_FLOAT,                         //type
                          GL_FALSE,                         //normalized?
                          sizeof(struct Vertex),            //stride
                          (void*)offsetof(struct Vertex, position));//offset
    glVertexAttribPointer(gResources.attributes[currShader].normal,    //attribute
                          3,                                //size
                          GL_FLOAT,                         //type
                          GL_FALSE,                         //normalized?
                          sizeof(struct Vertex),            //stride
                          (void*)offsetof(struct Vertex, normal));//offset
    glVertexAttribPointer(gResources.attributes[currShader].tangent,    //attribute
                          3,                                //size
                          GL_FLOAT,                         //type
                          GL_FALSE,                         //normalized?
                          sizeof(struct Vertex),            //stride
                          (void*)offsetof(struct Vertex, tangent));//offset
    glVertexAttribPointer(gResources.attributes[currShader].texCoord,  //attribute
                          4,                                //size
                          GL_FLOAT,                         //type
                          GL_FALSE,                         //normalized?
                          sizeof(struct Vertex),            //stride
                          (void*)offsetof(struct Vertex, texCoord));//offset
    glVertexAttribPointer(gResources.attributes[currShader].color,     //attribute
                          3,                                //size
                          GL_UNSIGNED_BYTE,                 //type
                          GL_FALSE,                         //normalized?
                          sizeof(struct Vertex),            //stride
                          (void*)offsetof(struct Vertex, color));//offset
    glVertexAttribPointer(gResources.attributes[currShader].diffuse,   //attribute
                          4,                                //size
                          GL_FLOAT,                         //type
                          GL_FALSE,                         //normalized?
                          sizeof(struct Vertex),            //stride
                          (void*)offsetof(struct Vertex, diffuse));//offset
    glVertexAttribPointer(gResources.attributes[currShader].specular,  //attribute
                          4,                                //size
                          GL_FLOAT,                         //type
                          GL_FALSE,                         //normalized?
                          sizeof(struct Vertex),            //stride
                          (void*)offsetof(struct Vertex, specular));//offset

    glVertexAttribPointer(gResources.attributes[currShader].boneMat[0],//attribute
                          4,                                //size
                          GL_FLOAT,                         //type
                          GL_FALSE,                         //normalized?
                          sizeof(struct Vertex),            //stride
                          (void*)offsetof(struct Vertex, boneMat));//offset
    glVertexAttribPointer(gResources.attributes[currShader].boneMat[1],//attribute
                          4,                                //size
                          GL_FLOAT,                         //type
                          GL_FALSE,                         //normalized?
                          sizeof(struct Vertex),            //stride
                          (void*)((char*)offsetof(struct Vertex, boneMat) + 16));//offset
    glVertexAttribPointer(gResources.attributes[currShader].boneMat[2],//attribute
                          4,                                //size
                          GL_FLOAT,                         //type
                          GL_FALSE,                         //normalized?
                          sizeof(struct Vertex),            //stride
                          (void*)((char*)offsetof(struct Vertex, boneMat) + 32));//offset
    glVertexAttribPointer(gResources.attributes[currShader].mPos,      //attribute
                          3,                                //size
                          GL_FLOAT,                         //type
                          GL_FALSE,                         //normalized?
                          sizeof(struct Vertex),            //stride
                          (void*)offsetof(struct Vertex, mPos));//offset
    glVertexAttribPointer(gResources.attributes[currShader].mRot,      //attribute
                          4,                                //size
                          GL_FLOAT,                         //type
                          GL_FALSE,                         //normalized?
                          sizeof(struct Vertex),            //stride
                          (void*)offsetof(struct Vertex, mRot));//offset
    glVertexAttribPointer(gResources.attributes[currShader].mScale,    //attribute
                          3,                                //size
                          GL_FLOAT,                         //type
                          GL_FALSE,                         //normalized?
                          sizeof(struct Vertex),            //stride
                          (void*)offsetof(struct Vertex, mScale));//offset
}

void Mesh::render(const vector<uint8_t>& currShader)
{
    //Don't bother if there is nothing to render
    if(!numVerts)
        return;

    glEnable(GL_DEPTH_TEST);
    glBindBuffer(GL_ARRAY_BUFFER, gResources.vertexBuffer);

    setData(currShader[0]);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gResources.elementBuffer);

    uint8_t realCurrShader = currShader[0];

    for(int i = 0; i < numMats; ++i)
    {
        if(currShader.size() == 3 && i == currShader[2] /*When to change shaders*/)
        {
            realCurrShader++;
            glUseProgram(gResources.program[realCurrShader]);
            setData(realCurrShader);
            glDisable(GL_DEPTH_TEST);
        }

        if(matIndexSize[2*i+1] == 0)
            continue;

        //Bind the images of the 'texture' of the material
        for(int j = 0; j < 8; ++j)
        {
            if(texIDs[8 * i + j] == 0xff)
                continue;

            glActiveTexture(GL_TEXTURE0 + j);
            glBindTexture(GL_TEXTURE_2D, gResources.textures[imgIDs[texIDs[8*i+j]]]);
            glUniform1i(gResources.uniforms[realCurrShader].textures[j], j);
        }
        //Act like Sonic when drawing
        glDrawElements(GL_TRIANGLES,            //mode
                   matIndexSize[2*i+1],         //count
                   GL_UNSIGNED_INT,             //type
                   (void*)(matIndexSize[2*i] * sizeof(GLuint))); //offset
    }
}
