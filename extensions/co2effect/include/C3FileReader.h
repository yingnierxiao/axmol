/****************************************************************************
 Copyright (c) 2025 Axmol Engine contributors

 https://axmol.dev/

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 ****************************************************************************/

#ifndef __CO2EFFECT_C3_FILE_READER_H__
#define __CO2EFFECT_C3_FILE_READER_H__

#include "cocos2d.h"
#include <string>
#include <vector>

namespace ax {
namespace co2effect {

using ax::Vec2;
using ax::Vec3;
using ax::Vec4;
using ax::Mat4;

/**
 * C3文件格式读取器
 *
 * C3格式来自Conquer Online 2的3D特效系统
 * 文件结构: "MAXFILE C3 00001" + Chunks
 */

// 顶点数据结构
struct C3Vertex
{
    Vec3 position;
    Vec4 color;
    Vec2 texcoord;
    uint32_t boneIndices[2];
    float boneWeights[2];
};

// 网格数据(PHY4 Chunk)
struct C3MeshData
{
    std::string name;
    std::string textureName;
    std::vector<C3Vertex> vertices;
    std::vector<uint16_t> indices;
    Vec3 bboxMin;
    Vec3 bboxMax;
    uint32_t normalTriCount;
    uint32_t alphaTriCount;
};

// 骨骼动画关键帧
struct C3KeyFrame
{
    uint32_t frameIndex;
    std::vector<Mat4> boneMatrices;
};

// 骨骼动画数据(MOTI Chunk)
struct C3MotionData
{
    uint32_t boneCount;
    uint32_t frameCount;
    std::vector<C3KeyFrame> keyFrames;
    std::vector<float> morphWeights;
    uint32_t morphCount;
};

// 粒子帧数据
struct C3ParticleFrame
{
    uint32_t particleCount;
    std::vector<Vec3> positions;
    std::vector<float> ages;
    std::vector<float> sizes;
    Mat4 transform;
};

// 粒子系统数据(PTCL Chunk)
struct C3ParticleData
{
    std::string name;
    std::string textureName;
    uint32_t maxParticles;
    uint32_t textureRows;
    std::vector<C3ParticleFrame> frames;
};

// 形状/线条数据(SHAP Chunk)
struct C3ShapeData
{
    std::string name;
    std::string textureName;
    std::vector<std::vector<Vec3>> lines;
    uint32_t segmentCount;
};

// C3文件完整数据
struct C3FileData
{
    std::vector<C3MeshData> meshes;
    std::vector<C3MotionData> motions;
    std::vector<C3ParticleData> particles;
    std::vector<C3ShapeData> shapes;
};

/**
 * C3文件读取器
 */
class C3FileReader
{
public:
    C3FileReader();
    ~C3FileReader();

    static bool readC3File(const std::string& filepath, C3FileData& outData, const std::string& wdbPath = "");
    static bool readC3FileFromMemory(const uint8_t* data, size_t size, C3FileData& outData);

private:
    #pragma pack(push, 1)
    struct ChunkHeader
    {
        char id[4];
        uint32_t size;
    };
    #pragma pack(pop)

    static bool readPhyChunk(const uint8_t* data, size_t size, C3MeshData& outMesh);
    static bool readMotionChunk(const uint8_t* data, size_t size, C3MotionData& outMotion);
    static bool readParticleChunk(const uint8_t* data, size_t size, C3ParticleData& outParticle);
    static bool readShapeChunk(const uint8_t* data, size_t size, C3ShapeData& outShape);
    static std::string readString(const uint8_t*& ptr, const uint8_t* end);
    static bool checkBuffer(const uint8_t* ptr, const uint8_t* end, size_t required);
};

} // namespace co2effect
} // namespace ax

#endif // __CO2EFFECT_C3_FILE_READER_H__
