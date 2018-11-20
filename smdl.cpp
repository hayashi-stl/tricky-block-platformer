#include "smdl.h"
//#define O(n^2) evil_for_vertices

bool compareFaces(const FaceSMDL& a, const FaceSMDL& b)
{
    return a.matID < b.matID;
}

Vector<3> normalF(VertSMDL* v, FaceSMDL* f, int index)
{
    int i = index / 3 * 3;
    Vector<3> dPos[2] = {(Vector<3>)v[f[i+1].vertID].pos - (Vector<3>)v[f[i].vertID].pos,
                         (Vector<3>)v[f[i+2].vertID].pos - (Vector<3>)v[f[i].vertID].pos};
    Vector<3> cross = dPos[0].cross(dPos[1]);
    cross /= Vector<3>::mag(cross);
    return cross;
}

Vector<3> tangentF(VertSMDL* v, FaceSMDL* f, int index)
{
    //floor division
    int i = index / 3 * 3;
    Vector<3> dPos[2] = {(Vector<3>)v[f[i+1].vertID].pos - (Vector<3>)v[f[i].vertID].pos,
                         (Vector<3>)v[f[i+2].vertID].pos - (Vector<3>)v[f[i].vertID].pos};
    Vector<2> dUV[2] = {(Vector<2>)v[f[i+1].vertID].texCoord - (Vector<2>)v[f[i].vertID].texCoord,
                        (Vector<2>)v[f[i+2].vertID].texCoord - (Vector<2>)v[f[i].vertID].texCoord};

    Vector<3> tangent = (dPos[0]*dUV[1][1] - dPos[1]*dUV[0][1]) / (dUV[0][0]*dUV[1][1] - dUV[0][1]*dUV[1][0]);
    tangent /= Vector<3>::mag(tangent);
    return tangent;
}

string fromSMDLA(MeshObj* m, const char* filename, const char* name, int type)
{
    vector<int> typeTrans;
    vector<int> indexTrans;
    if(type == BONE)
    {
        typeTrans = vector<int>({0, 0, 0, 1, 1, 1, 1, 2, 2, 2});
        indexTrans= vector<int>({0, 1, 2, 0, 1, 2, 3, 0, 1, 2});
    }
    else if(type == MATERIAL)
    {
        typeTrans = vector<int>({0, 0, 0, 1, 2, 2, 2, 3, 4});
        indexTrans= vector<int>({0, 1, 2, 0, 0, 1, 2, 0, 0});
    }

    //Open file and return a goose egg if it doesn't work
    ifstream file;
    file.open(filename, ios::in | ios::binary);
    if(!file.is_open())
        return "File could not open\0";

    //Make sure it is a proper SMDL file
    if(readInt(file) != 0x534d444c /*SMDL*/)
    {
        file.close();
        return "This is not a proper SMDL-type file because look at the first 4 bytes!\0";
    }
    if(readByte(file) != 0x41 /*A*/)
    {
        file.close();
        return "This is not a proper SMDL animation file because look at the 5th byte!\0";
    }

    char intToCheck;
    if(type == BONE)
        intToCheck = 'B';
    else if(type == MATERIAL)
        intToCheck = 'M';

    if(readByte(file) != intToCheck)
    {
        file.close();
        return "This is not a proper SMDLA" + string(1, intToCheck) + " file because look at the 6th byte!\0";
    }

    //Initialize the animation for storage
    Animation* anim;
    m->animations[type].insert({name, Animation()});
    anim = &m->animations[type][name];
    anim->animLen = uint16_t(roundf(readFloat(file) * 30.0 /*fps*/));

    uint32_t fileSize = readInt(file);
    file.ignore(2); //padding

    while(file.tellg() != fileSize + 16)
    {
        anim->anims.push_back(AnimCurve());
        AnimCurve* c = &anim->anims.back();

        //Things for each curve
        c->ID = readByte(file);
        uint8_t propAndIndex = readByte(file);
        c->prop = typeTrans[propAndIndex];
        c->index = indexTrans[propAndIndex];

        uint16_t numKeys = readShort(file);
        c->keys = vector<AnimCurve::Keyframe>(numKeys);

        //Things for each keyframe
        for(int i = 0; i < numKeys; ++i)
        {
            AnimCurve::Keyframe* key = &c->keys[i];
            key->pos = uint16_t(roundf(readFloat(file) * 30.0 /*fps*/));
            key->interp = readByte(file);
            key->values.push_back(readFloat(file));

            //easing
            if(key->interp != LINEAR && key->interp != CONSTANT)
                key->values.push_back(readByte(file));
        }

        //Correct keyframes if they don't span the whole length of the animation
        if(c->keys[0].pos > 0)
        {
            c->keys.insert(c->keys.begin(), AnimCurve::Keyframe());
            c->keys[0].pos = 0;
            c->keys[0].interp = CONSTANT;
            c->keys[0].values.push_back(c->keys[1].values[0]);
        }
        if(c->keys.back().pos < anim->animLen)
        {
            c->keys.push_back(AnimCurve::Keyframe());
            c->keys.back().pos = anim->animLen;
            c->keys.back().interp = CONSTANT;
            c->keys.back().values.push_back(c->keys[c->keys.size() - 2].values[0]);
        }
    }

    return string();
}

string fromSMDL(const char* filename, const char* name, vector<string> animFilenames[], vector<string> animNames[])
{
    //Open file and return a goose egg if it doesn't work
    ifstream file;
    file.open(filename, ios::in | ios::binary);
    if(!file.is_open())
        return "File could not open\0";

    //Make sure it is a proper SMDL file
    if(readInt(file) != 0x534d444c)
    {
        file.close();
        return "This is not a proper SMDL file because look at the first 4 bytes!\0";
    }

    file.ignore(2);
    bool isRigged = readByte(file) != 0;
    file.ignore(9);

    //Rigging info
    unique_ptr<Bone[]> bones;
    uint8_t numBones = 0;
    if(isRigged)
    {
        //Make sure it is a proper SMDL file (test 1.5)
        if(readInt(file) != 0x424f4e45)
        {
            file.close();
            return "This is not a proper SMDL file because it is missing a BONE section and is rigged\0";
        }
        file.ignore(5);
        numBones = readByte(file);
        file.ignore(6);

        bones = unique_ptr<Bone[]>(new Bone[numBones + 1]); //Add 1 for the root bone
        for(int i = 0; i <= numBones; ++i)
            bones[i] = Bone();

        for(int i = 0; i < numBones; ++i)
        {
            if(readByte(file))
            {
                bones[i].parent = readByte(file);
                bones[bones[i].parent].children.push_back(i);
            }
            else
            {
                file.ignore(1); //would have been the parent index
                bones[i].parent = numBones; //root node
                bones[numBones].children.push_back(i);
            }
            for(int j = 0; j < 3; ++j)
                bones[i].head[j] = readFloat(file);
        }
        ignorePad(file, 16);
    }

    //Make sure it is a proper SMDL file (test 2)
    if(readInt(file) != 0x56455254)
    {
        file.close();
        return "This is not a proper SMDL file because it is missing a VERT section\0";
    }
    file.ignore(4);

    uint16_t numVerts = readShort(file);
    //initialize temporary array with attributes
    //coordx, coordy, coordz, normalx, normaly, normalz, texcoordu, texcoordv
    //numFace3s is defined here so it doesn't have to search the face section for it
    uint16_t numFace3s = readShort(file);
    //must be dynamic or declaring forward_list<uint16_t>[numFace3s] will crash program
    unique_ptr<VertSMDL[]> vertArray(new VertSMDL[numFace3s]);
    file.ignore(4);

    //Read vertices
    for(uint16_t i = 0; i < numVerts; ++i)
    {
        //Store vertex positions
        vertArray[i].pos[0] = readFloat(file);
        vertArray[i].pos[1] = readFloat(file);
        vertArray[i].pos[2] = readFloat(file);

        //Store vertex normals
        vertArray[i].normal[0] = readFloat(file);
        vertArray[i].normal[1] = readFloat(file);
        vertArray[i].normal[2] = readFloat(file);

        if(isRigged)
        {
            vertArray[i].groups = vector<BoneGroup>(readByte(file));
            for(auto it = vertArray[i].groups.begin(); it != vertArray[i].groups.end(); ++it)
            {
                it->index = readByte(file);
                it->weight = readFloat(file);
            }
        }
    }
    ignorePad(file, 16);

    //Make sure it is a proper SMDL file (test 3)
    if(readInt(file) != 0x494d4753)
    {
        file.close();
        return "This is not a proper SMDL file because it is missing a IMGS section\0";
    }
    file.ignore(5);
    uint8_t numImgs = readByte(file);
    file.ignore(6);

    //Store image names
    string imgName[numImgs];
    for(int i = 0; i < numImgs; ++i)
    {
        uint8_t nameSize = readByte(file);
        char tempImgName[nameSize + 5];
        tempImgName[nameSize    ] = '.';
        tempImgName[nameSize + 1] = 't';
        tempImgName[nameSize + 2] = 'g';
        tempImgName[nameSize + 3] = 'a';
        tempImgName[nameSize + 4] = 0;
        file.read(tempImgName, nameSize);
        imgName[i] = (string)tempImgName;
    }

    ignorePad(file, 16);

    //Make sure it is a proper SMDL file (test 4)
    if(readInt(file) != 0x54455853)
    {
        file.close();
        return "This is not a proper SMDL file because it is missing a TEXS section\0";
    }
    file.ignore(5);
    uint8_t numTexs = readByte(file);
    file.ignore(6);

    //Store image IDs of each texture
    uint8_t imgIDs[numTexs];
    for(int i = 0; i < numTexs; ++i)
    {
        //offset value by the number of images already there to
        //avoid ID collisions
        imgIDs[i] = readByte(file) + meshH.numImgs;
        file.ignore(1);
    }

    ignorePad(file, 16);

    //Make sure it is a proper SMDL file (test 5)
    if(readInt(file) != 0x4d415453)
    {
        file.close();
        return "This is not a proper SMDL file because it is missing a MATS section\0";
    }
    file.ignore(5);
    uint8_t numMats = readByte(file);
    file.ignore(6);

    //Read the materials
    unique_ptr<MatSMDL[]> matArray (new MatSMDL[numMats]);
    uint8_t texIDs[8 * numMats];
    for(uint8_t i = 0; i < numMats; ++i)
    {
        for(uint8_t j = 0; j < 8; ++j)
            texIDs[8 * i + j] = 0xff;

        //Diffuse
        for(uint8_t j = 0; j < 3; ++j)
            matArray[i].diffuse[j] = readByte(file) / 255.0f;
        matArray[i].diffuse[3] = readFloat(file);

        //Specular
        for(uint8_t j = 0; j < 3; ++j)
            matArray[i].specular[j] = readByte(file) / 255.0f;
        matArray[i].specular[3] = readFloat(file);

        matArray[i].shininess = readFloat(file);

        uint8_t useAlpha = readByte(file);
        if(useAlpha)
            file.ignore(7);

        uint8_t numTexSlots = readByte(file);
        for(uint8_t j = 0; j < numTexSlots; ++j)
        {
            uint8_t texID = readByte(file);
            uint8_t influence = readByte(file);
            //Combine influence in last slot
            matArray[i].influence |= influence;

            uint8_t numInfs = 0;
            for(uint8_t k = 0; k < 8; ++k)
            {
                if(influence >> k & 1)
                    //offset value by the number of textures already there to
                    //avoid ID collisions
                    texIDs[8 * i + 7 - k] = texID + meshH.numTexs;
                numInfs += influence >> k & 1;
            }

            file.ignore(4 * numInfs);
        }
    }
    ignorePad(file, 16);

    //Make sure it is a proper SMDL file (test 6)
    if(readInt(file) != 0x46414345)
    {
        file.close();
        return "This is not a proper SMDL file because it is missing a FACE section\0";
    }
    file.ignore(4);
    uint16_t numFaces = readShort(file);
    file.ignore(6);

    if(3 * numFaces != numFace3s)
    {
        file.close();
        return "The number of faces doesn't match the one before\0";
    }

    //6 texture coordinates per face
    unique_ptr<FaceSMDL[]> faceArray(new FaceSMDL[numFace3s]);

    for(uint16_t i = 0; i < numFaces; ++i)
    {
        //Store vertex indices
        faceArray[3 * i + 0].vertID = readShort(file);
        faceArray[3 * i + 1].vertID = readShort(file);
        faceArray[3 * i + 2].vertID = readShort(file);

        //Store texture coordinates
        faceArray[3 * i    ].texCoord[0] = readFloat(file);
        faceArray[3 * i    ].texCoord[1] = readFloat(file);
        faceArray[3 * i + 1].texCoord[0] = readFloat(file);
        faceArray[3 * i + 1].texCoord[1] = readFloat(file);
        faceArray[3 * i + 2].texCoord[0] = readFloat(file);
        faceArray[3 * i + 2].texCoord[1] = readFloat(file);

        for(uint8_t j = 0; j < 3; ++j)
        {
            for(uint8_t k = 0; k < 3; ++k)
                faceArray[3 * i + j].color[k] = readByte(file);
        }


        uint8_t matIndex = readByte(file);
        for(uint8_t j = 0; j < 3; ++j)
            faceArray[3 * i + j].matID = matIndex;
    }

    //Sort the faces by material and therefore by texture
    //Use stable sort to preserve vertex connectivity

    file.close();

    //Iterate through faces and store which faces use each vertex in linear time
    for(uint16_t i = 0; i < numFace3s; ++i)
    {
        vertArray[faceArray[i].vertID].vFaces.push_front(i);
    }

    //Deal with the indexing of OpenGL
    uint16_t oldNumVerts = numVerts;
    for(uint16_t i = 0; i < oldNumVerts; ++i)
    {
        //Skip stray vertices
        if(vertArray[i].vFaces.empty())
            continue;

        //Stores groups of face thirds with equal texcoords for each vertex
        //Store the first face third using the vertex in the first slot
        vector<list<uint16_t>> faceGroups;
        faceGroups.push_back(list<uint16_t>());
        faceGroups[0].push_back(vertArray[i].vFaces.front());

        //Also store texcoord groups and the first texcoords now
        list<FaceSMDL> face3Groups;
        face3Groups.push_back(faceArray[vertArray[i].vFaces.front()]);

        //Add 1 because the first face third index already got stored
        for(auto j = vertArray[i].vFaces.begin(); j != vertArray[i].vFaces.end(); ++j)
        {
            if(j == vertArray[i].vFaces.begin())
                continue;

            //Check if the face third's texcoords equal any previous texcoords
            bool equalsFace3 = 0;
            int index = 0;
            for(auto k = face3Groups.begin(); k != face3Groups.end(); ++k)
            {
                if(faceArray[*j] == (*k))
                {
                    equalsFace3 = 1;
                    faceGroups[index].push_back(*j);
                }
                ++index;
                //If a match is found, quit searching
                if(equalsFace3)
                    break;
            }
            //If no match is found, add a new group with the data
            if(!equalsFace3)
            {
                face3Groups.push_back(faceArray[*j]);

                faceGroups.push_back(list<uint16_t>());
                (*(faceGroups.end() - 1)).push_back(*j);
            }
        }

        //Finally assign texture coordinates to vertices
        vertArray[i].texCoord[0] = face3Groups.front().texCoord[0];
        vertArray[i].texCoord[1] = face3Groups.front().texCoord[1];
        for(uint8_t j = 0; j < 3; ++j)
            vertArray[i].color[j] = face3Groups.front().color[j];
        for(uint8_t j = 0; j < 4; ++j)
        {
            vertArray[i].diffuse[j] = matArray[face3Groups.front().matID].diffuse[j];
            vertArray[i].specular[j] = matArray[face3Groups.front().matID].specular[j];
        }
        vertArray[i].shininess = matArray[face3Groups.front().matID].shininess;
        vertArray[i].influence = matArray[face3Groups.front().matID].influence;

        if(faceGroups.size() != 1)
        {
            vertArray[i].vFaces = forward_list<uint16_t>(faceGroups[0].begin(), faceGroups[0].end());
            int index = 1;
            for(auto j = face3Groups.begin(); j != face3Groups.end(); ++j)
            {
                if(j == face3Groups.begin())
                    continue;

                //Add new vertices to accommodate for unequal texcoords
                for(int k = 0; k < 3; ++k)
                {
                    vertArray[numVerts].pos[k] = vertArray[i].pos[k];
                    vertArray[numVerts].normal[k] = vertArray[i].normal[k];
                }
                vertArray[numVerts].groups = vertArray[i].groups;

                //Assign different texcoords to each new vertex
                vertArray[numVerts].texCoord[0] = (*j).texCoord[0];
                vertArray[numVerts].texCoord[1] = (*j).texCoord[1];
                for(uint8_t k = 0; k < 3; ++k)
                    vertArray[numVerts].color[k] = (*j).color[k];
                for(int k = 0; k < 4; ++k)
                {
                    vertArray[numVerts].diffuse[k] = matArray[(*j).matID].diffuse[k];
                    vertArray[numVerts].specular[k] = matArray[(*j).matID].specular[k];
                }
                vertArray[numVerts].shininess = matArray[(*j).matID].shininess;
                vertArray[numVerts].influence = matArray[(*j).matID].influence;
                vertArray[numVerts].vFaces = forward_list<uint16_t>(faceGroups[index].begin(), faceGroups[index].end());

                for(auto k = faceGroups[index].begin(); k != faceGroups[index].end(); ++k)
                    faceArray[*k].vertID = numVerts;
                numVerts += 1;
                index += 1;
            }
        }
    }

    vector<FaceSMDL> faceArrayToSort;
    faceArrayToSort.assign(faceArray.get(), faceArray.get() + numFace3s);

    //Sort by material
    //Stable sort to protect connectivity
    stable_sort(faceArrayToSort.begin(), faceArrayToSort.end(), compareFaces);

    for(int i = 0; i < numFace3s; ++i)
    {
        faceArray[i].vertID = faceArrayToSort[i].vertID;
        faceArray[i].matID = faceArrayToSort[i].matID;
    }

    //store how many times each material is used and the index to the first
    //face 3rd of each material
    uint16_t matIndexSize[2 * numMats];
    for(int i = 0; i < numMats; ++i)
    {
        matIndexSize[2 * i    ] = 0xffff;
        matIndexSize[2 * i + 1] = 0;
    }
    for(int i = 0; i < numFace3s; ++i)
    {
        if(matIndexSize[2 * faceArray[i].matID] == 0xffff)
            matIndexSize[2 * faceArray[i].matID] = i;
        matIndexSize[2 * faceArray[i].matID + 1] += 1;
    }


    //Iterate through faces and store which faces use each vertex in linear time
    //again; the sorting messed things up
    for(uint16_t i = 0; i < numVerts; ++i)
        vertArray[i].vFaces.clear();
    for(uint16_t i = 0; i < numFace3s; ++i)
        vertArray[faceArray[i].vertID].vFaces.push_front(i);

    //Finally compute tangents if necessary
    //Using a tangent without bump mapping is undefined behavior.
    for(uint16_t i = 0; i < numVerts; ++i)
    {
        if(vertArray[i].influence & 1)
        {
            //Prepare the vectors for calculating the tangent
            Vector<3> normal = normalF(vertArray.get(), faceArray.get(), vertArray[i].vFaces.front());
            Vector<3> tangent = tangentF(vertArray.get(), faceArray.get(), vertArray[i].vFaces.front());
            if(isnan(tangent[0]) | isnan(tangent[1]) | isnan(tangent[2]))
            {
                Quaternion dNorm2 = rotateTo(Vector<3>(1, 0, 0), normal);
                Vector<4> newTan2 = dNorm2 * Quaternion(tangent, false) * dNorm2.conj();
                for(int i = 0; i < 3; ++i)
                    tangent.v[i] = newTan2[i + 1];
            }

            //rotate tangent to fit vertex normal
            Quaternion dNorm = rotateTo(normal, (Vector<3>)vertArray[i].normal);
            Vector<4> newTan = dNorm * Quaternion(tangent, 0) * dNorm.conj();
            for(int j = 0; j < 3; ++j)
                vertArray[i].tangent[j] = newTan[j + 1];
        }
    }

    meshH.meshes.insert({string(name), MeshObj()});
    MeshObj* m = &(meshH.meshes[name]);

    //Data copying time!
    m->numVerts = numVerts;
    m->numTris = numFace3s;
    m->imgOffset = meshH.numImgs;
    m->texOffset = meshH.numTexs;
    m->matOffset = meshH.numMats;
    m->numMats = numMats;

    m->vertex = unique_ptr<Vertex[]>(new Vertex[numVerts]);
    m->element = unique_ptr<uint16_t[]>(new uint16_t[numFace3s]);
    m->matIndexSize = unique_ptr<uint16_t[]>(new uint16_t[numMats * 2]);

    m->vFaces = unique_ptr<forward_list<uint16_t>[]>(new forward_list<uint16_t>[numVerts]);
    m->groups = unique_ptr<vector<BoneGroup>[]>(new vector<BoneGroup>[numVerts]);

    //For each material, list vertices that use it; will be useful for diffuse and specular animation
    m->matVerts = unique_ptr<vector<uint16_t>[]>(new vector<uint16_t>[numMats]);

    m->isRigged = isRigged;
    if(isRigged)
    {
        m->bones = vector<Bone>(numBones + 1); //Add 1 for the root bone
        for(uint8_t i = 0; i <= numBones; ++i)
            m->bones[i] = bones[i];
    }

    for(uint16_t i = 0; i < numVerts; ++i)
    {
        m->vertex[i].position[0] = vertArray[i].pos[0];
        m->vertex[i].position[1] = vertArray[i].pos[1];
        m->vertex[i].position[2] = vertArray[i].pos[2];
        m->vertex[i].position[3] = 1.0;

        m->vertex[i].normal[0] = vertArray[i].normal[0];
        m->vertex[i].normal[1] = vertArray[i].normal[1];
        m->vertex[i].normal[2] = vertArray[i].normal[2];

        m->vertex[i].tangent[0] = vertArray[i].tangent[0];
        m->vertex[i].tangent[1] = vertArray[i].tangent[1];
        m->vertex[i].tangent[2] = vertArray[i].tangent[2];

        m->vertex[i].texCoord[0] = vertArray[i].texCoord[0];
        m->vertex[i].texCoord[1] = vertArray[i].texCoord[1];

        m->vertex[i].color[0] = vertArray[i].color[0];
        m->vertex[i].color[1] = vertArray[i].color[1];
        m->vertex[i].color[2] = vertArray[i].color[2];

        //Shininess
        m->vertex[i].texCoord[2] = vertArray[i].shininess;

        m->vertex[i].diffuse[0] = vertArray[i].diffuse[0];
        m->vertex[i].diffuse[1] = vertArray[i].diffuse[1];
        m->vertex[i].diffuse[2] = vertArray[i].diffuse[2];
        m->vertex[i].diffuse[3] = vertArray[i].diffuse[3];
        m->vertex[i].specular[0] = vertArray[i].specular[0];
        m->vertex[i].specular[1] = vertArray[i].specular[1];
        m->vertex[i].specular[2] = vertArray[i].specular[2];
        m->vertex[i].specular[3] = vertArray[i].specular[3];

        m->vertex[i].texCoord[3] = (float)vertArray[i].influence;

        //create 3/4 of an identity matrix
        for(int j = 0; j < 12; ++j)
            m->vertex[i].boneMat[j] = ((j >> 2 == (j & 3)) ? 1 : 0);

        //set regular model transformation
        for(int j = 0; j < 3; ++j)
        {
            m->vertex[i].mPos[j] = 0.0;
            m->vertex[i].mScale[j] = 1.0;
            m->vertex[i].mRot[j+1] = 0.0;
        }
        m->vertex[i].mRot[0] = 1.0;

        m->vFaces[i].insert_after(m->vFaces[i].before_begin(), vertArray[i].vFaces.begin(), vertArray[i].vFaces.end());
        if(isRigged)
            m->groups[i] = vertArray[i].groups;

        if(!vertArray[i].vFaces.empty())
            m->matVerts[faceArray[vertArray[i].vFaces.front()].matID].push_back(i);
    }

    for(uint8_t i = 0; i < numImgs; ++i)
    {
        meshH.imgNames.push_back(imgName[i]);
    }

    for(uint8_t i = 0; i < numTexs; ++i)
    {
        meshH.imgIDs.push_back(imgIDs[i]);
    }

    for(uint8_t i = 0; i < numMats; ++i)
    {
        m->matIndexSize[2 * i    ] = matIndexSize[2 * i    ];
        m->matIndexSize[2 * i + 1] = matIndexSize[2 * i + 1];
        for(uint8_t j = 0; j < 8; ++j)
        {
            meshH.texIDs.push_back(texIDs[8 * i + j]);
        }

        //Also don't forget to add a material slot in the mesh handler
        meshH.element.push_back(list<uint32_t>());
    }

    for(uint16_t i = 0; i < numFace3s; ++i)
        m->element[i] = faceArray[i].vertID;

    //For keeping track of totals
    meshH.numImgs += numImgs;
    meshH.numTexs += numTexs;
    meshH.numMats += numMats;

    //Insert default animations (which do nothing)
    for(int i = 0; i < numAnimFiles; ++i)
    {
        m->animations[i].insert({"", Animation()});
        m->animations[i][""].animLen = 1;
    }

    //Read the different types of animations
    for(int i = 0; i < numAnimFiles; ++i)
    {
        for(uint16_t j = 0; j < animFilenames[i].size(); ++j)
        {
            string error = fromSMDLA(m, animFilenames[i][j].c_str(), animNames[i][j].c_str(), i);
            if(!error.empty())
            {
                return (string)"Error loading " + animFilenames[i][j] + ": " + error + "\n\0";
            }
        }
    }

    return string();
}


