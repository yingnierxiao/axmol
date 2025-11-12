/****************************************************************************
 Copyright (c) 2025 Axmol Engine contributors
 ****************************************************************************/

#include "C3FileReader.h"
#include "WDBReader.h"
#include "cocos2d.h"
#include <cstring>

namespace ax {
namespace co2effect {

using ax::FileUtils;
using ax::Data;

static constexpr char C3_MAGIC[] = "MAXFILE C3 00001";
static constexpr size_t C3_HEADER_SIZE = 16;

C3FileReader::C3FileReader() {}
C3FileReader::~C3FileReader() {}

// 辅助函数: 检查缓冲区
bool C3FileReader::checkBuffer(const uint8_t* ptr, const uint8_t* end, size_t required)
{
    return ptr + required <= end;
}

// 辅助函数: 读取字符串(前4字节是长度)
std::string C3FileReader::readString(const uint8_t*& ptr, const uint8_t* end)
{
    if (!checkBuffer(ptr, end, sizeof(uint32_t)))
        return "";

    uint32_t length = *reinterpret_cast<const uint32_t*>(ptr);
    ptr += sizeof(uint32_t);

    if (!checkBuffer(ptr, end, length))
        return "";

    std::string result(reinterpret_cast<const char*>(ptr), length);
    ptr += length;
    return result;
}

// 读取C3文件
bool C3FileReader::readC3File(const std::string& filepath, C3FileData& outData, const std::string& wdbPath)
{
    Data fileData;

    // 优先从WDB加载
    if (!wdbPath.empty())
    {
        WDBReader wdbReader;
        if (wdbReader.open(wdbPath))
        {
            AXLOG("C3FileReader: Trying to load from WDB: %s", filepath.c_str());
            fileData = wdbReader.readFile(filepath);
            if (!fileData.isNull())
            {
                AXLOG("C3FileReader: Loaded from WDB successfully");
            }
        }
    }

    // 如果WDB加载失败，尝试从文件系统加载
    if (fileData.isNull())
    {
        AXLOG("C3FileReader: Loading from file system: %s", filepath.c_str());
        fileData = FileUtils::getInstance()->getDataFromFile(filepath);
        if (fileData.isNull())
        {
            AXLOG("C3FileReader: Failed to read file: %s", filepath.c_str());
            return false;
        }
    }

    return readC3FileFromMemory(fileData.getBytes(), fileData.getSize(), outData);
}

// 从内存读取C3文件
bool C3FileReader::readC3FileFromMemory(const uint8_t* data, size_t size, C3FileData& outData)
{
    if (!data || size < C3_HEADER_SIZE)
    {
        AXLOG("C3FileReader: Invalid data or size too small");
        return false;
    }

    // 验证文件头
    if (std::memcmp(data, C3_MAGIC, C3_HEADER_SIZE) != 0)
    {
        AXLOG("C3FileReader: Invalid C3 file magic header");
        return false;
    }

    const uint8_t* ptr = data + C3_HEADER_SIZE;
    const uint8_t* end = data + size;

    // 读取所有Chunks
    while (ptr < end)
    {
        if (!checkBuffer(ptr, end, sizeof(ChunkHeader)))
            break;

        ChunkHeader header;
        std::memcpy(&header, ptr, sizeof(ChunkHeader));
        ptr += sizeof(ChunkHeader);

        if (!checkBuffer(ptr, end, header.size))
        {
            AXLOG("C3FileReader: Chunk size exceeds file boundary");
            break;
        }

        // 根据Chunk ID读取数据
        if (std::memcmp(header.id, "PHY", 3) == 0)  // PHY4, PHY3等
        {
            C3MeshData mesh;
            if (readPhyChunk(ptr, header.size, mesh))
            {
                outData.meshes.push_back(std::move(mesh));
            }
        }
        else if (std::memcmp(header.id, "MOTI", 4) == 0)
        {
            C3MotionData motion;
            if (readMotionChunk(ptr, header.size, motion))
            {
                outData.motions.push_back(std::move(motion));
            }
        }
        else if (std::memcmp(header.id, "PTCL", 4) == 0)
        {
            C3ParticleData particle;
            if (readParticleChunk(ptr, header.size, particle))
            {
                outData.particles.push_back(std::move(particle));
            }
        }
        else if (std::memcmp(header.id, "SHAP", 4) == 0)
        {
            C3ShapeData shape;
            if (readShapeChunk(ptr, header.size, shape))
            {
                outData.shapes.push_back(std::move(shape));
            }
        }
        else
        {
            // 未知Chunk,跳过
            AXLOG("C3FileReader: Unknown chunk type: %.4s", header.id);
        }

        ptr += header.size;
    }

    AXLOG("C3FileReader: Loaded %d meshes, %d motions, %d particles, %d shapes",
          (int)outData.meshes.size(), (int)outData.motions.size(),
          (int)outData.particles.size(), (int)outData.shapes.size());

    return true;
}

// 读取PHY4 Chunk
bool C3FileReader::readPhyChunk(const uint8_t* data, size_t size, C3MeshData& outMesh)
{
    const uint8_t* ptr = data;
    const uint8_t* end = data + size;

    // 读取名称
    outMesh.name = readString(ptr, end);

    // 读取BlendCount (每个顶点受多少骨骼影响)
    if (!checkBuffer(ptr, end, sizeof(uint32_t))) return false;
    uint32_t blendCount = *reinterpret_cast<const uint32_t*>(ptr);
    ptr += sizeof(uint32_t);

    // 读取顶点数量 (普通 + 透明)
    if (!checkBuffer(ptr, end, sizeof(uint32_t) * 2)) return false;
    uint32_t normalVertCount = *reinterpret_cast<const uint32_t*>(ptr);
    ptr += sizeof(uint32_t);
    uint32_t alphaVertCount = *reinterpret_cast<const uint32_t*>(ptr);
    ptr += sizeof(uint32_t);
    uint32_t totalVertCount = normalVertCount + alphaVertCount;

    // 读取顶点数据
    outMesh.vertices.resize(totalVertCount);
    for (uint32_t i = 0; i < totalVertCount; ++i)
    {
        if (!checkBuffer(ptr, end, sizeof(float) * 3 + sizeof(float) * 2 + sizeof(uint32_t)))
            return false;

        auto& vert = outMesh.vertices[i];

        // 位置 (Vec3)
        std::memcpy(&vert.position, ptr, sizeof(float) * 3);
        ptr += sizeof(float) * 3;

        // UV坐标
        std::memcpy(&vert.texcoord, ptr, sizeof(float) * 2);
        ptr += sizeof(float) * 2;

        // 颜色 (DWORD ARGB)
        uint32_t colorDWORD = *reinterpret_cast<const uint32_t*>(ptr);
        ptr += sizeof(uint32_t);

        // 转换ARGB到RGBA float
        vert.color.w = ((colorDWORD >> 24) & 0xFF) / 255.0f; // A
        vert.color.x = ((colorDWORD >> 16) & 0xFF) / 255.0f; // R
        vert.color.y = ((colorDWORD >> 8) & 0xFF) / 255.0f;  // G
        vert.color.z = (colorDWORD & 0xFF) / 255.0f;         // B

        // 骨骼索引和权重
        if (blendCount > 0)
        {
            if (!checkBuffer(ptr, end, sizeof(uint32_t) * 2 + sizeof(float) * 2))
                return false;

            vert.boneIndices[0] = *reinterpret_cast<const uint32_t*>(ptr);
            ptr += sizeof(uint32_t);
            vert.boneIndices[1] = *reinterpret_cast<const uint32_t*>(ptr);
            ptr += sizeof(uint32_t);

            vert.boneWeights[0] = *reinterpret_cast<const float*>(ptr);
            ptr += sizeof(float);
            vert.boneWeights[1] = *reinterpret_cast<const float*>(ptr);
            ptr += sizeof(float);
        }
        else
        {
            vert.boneIndices[0] = vert.boneIndices[1] = 0;
            vert.boneWeights[0] = 1.0f;
            vert.boneWeights[1] = 0.0f;
        }
    }

    // 读取三角形数量
    if (!checkBuffer(ptr, end, sizeof(uint32_t) * 2)) return false;
    outMesh.normalTriCount = *reinterpret_cast<const uint32_t*>(ptr);
    ptr += sizeof(uint32_t);
    outMesh.alphaTriCount = *reinterpret_cast<const uint32_t*>(ptr);
    ptr += sizeof(uint32_t);
    uint32_t totalTriCount = outMesh.normalTriCount + outMesh.alphaTriCount;

    // 读取索引数据
    uint32_t indexCount = totalTriCount * 3;
    if (!checkBuffer(ptr, end, sizeof(uint16_t) * indexCount)) return false;

    outMesh.indices.resize(indexCount);
    std::memcpy(outMesh.indices.data(), ptr, sizeof(uint16_t) * indexCount);
    ptr += sizeof(uint16_t) * indexCount;

    // 读取纹理名称
    outMesh.textureName = readString(ptr, end);

    // 读取包围盒
    if (checkBuffer(ptr, end, sizeof(float) * 6))
    {
        std::memcpy(&outMesh.bboxMin, ptr, sizeof(float) * 3);
        ptr += sizeof(float) * 3;
        std::memcpy(&outMesh.bboxMax, ptr, sizeof(float) * 3);
        ptr += sizeof(float) * 3;
    }

    AXLOG("C3FileReader: Loaded mesh '%s', %d vertices, %d triangles, texture '%s'",
          outMesh.name.c_str(), totalVertCount, totalTriCount, outMesh.textureName.c_str());

    return true;
}

// 读取MOTI Chunk
bool C3FileReader::readMotionChunk(const uint8_t* data, size_t size, C3MotionData& outMotion)
{
    const uint8_t* ptr = data;
    const uint8_t* end = data + size;

    // 读取骨骼数量
    if (!checkBuffer(ptr, end, sizeof(uint32_t))) return false;
    outMotion.boneCount = *reinterpret_cast<const uint32_t*>(ptr);
    ptr += sizeof(uint32_t);

    // 读取帧数
    if (!checkBuffer(ptr, end, sizeof(uint32_t))) return false;
    outMotion.frameCount = *reinterpret_cast<const uint32_t*>(ptr);
    ptr += sizeof(uint32_t);

    // 检查关键帧标识
    if (!checkBuffer(ptr, end, 4)) return false;
    char keyType[4];
    std::memcpy(keyType, ptr, 4);
    ptr += 4;

    if (std::memcmp(keyType, "XKEY", 4) == 0)
    {
        // 压缩格式(XKEY): WORD帧索引 + 压缩的3x4矩阵
        if (!checkBuffer(ptr, end, sizeof(uint32_t))) return false;
        uint32_t keyFrameCount = *reinterpret_cast<const uint32_t*>(ptr);
        ptr += sizeof(uint32_t);

        outMotion.keyFrames.resize(keyFrameCount);
        for (uint32_t i = 0; i < keyFrameCount; ++i)
        {
            if (!checkBuffer(ptr, end, sizeof(uint16_t))) return false;
            uint16_t frameIdx = *reinterpret_cast<const uint16_t*>(ptr);
            ptr += sizeof(uint16_t);

            outMotion.keyFrames[i].frameIndex = frameIdx;
            outMotion.keyFrames[i].boneMatrices.resize(outMotion.boneCount);

            // 读取压缩矩阵 (3x4 = 12个float)
            for (uint32_t b = 0; b < outMotion.boneCount; ++b)
            {
                if (!checkBuffer(ptr, end, sizeof(float) * 12)) return false;

                float m[12];
                std::memcpy(m, ptr, sizeof(float) * 12);
                ptr += sizeof(float) * 12;

                // 构建4x4矩阵
                auto& mat = outMotion.keyFrames[i].boneMatrices[b];
                mat.m[0] = m[0];  mat.m[1] = m[1];  mat.m[2] = m[2];  mat.m[3] = 0.0f;
                mat.m[4] = m[3];  mat.m[5] = m[4];  mat.m[6] = m[5];  mat.m[7] = 0.0f;
                mat.m[8] = m[6];  mat.m[9] = m[7];  mat.m[10] = m[8]; mat.m[11] = 0.0f;
                mat.m[12] = m[9]; mat.m[13] = m[10]; mat.m[14] = m[11]; mat.m[15] = 1.0f;
            }
        }
    }
    else if (std::memcmp(keyType, "KKEY", 4) == 0)
    {
        // 标准格式(KKEY): DWORD帧索引 + 完整4x4矩阵
        if (!checkBuffer(ptr, end, sizeof(uint32_t))) return false;
        uint32_t keyFrameCount = *reinterpret_cast<const uint32_t*>(ptr);
        ptr += sizeof(uint32_t);

        outMotion.keyFrames.resize(keyFrameCount);
        for (uint32_t i = 0; i < keyFrameCount; ++i)
        {
            if (!checkBuffer(ptr, end, sizeof(uint32_t))) return false;
            outMotion.keyFrames[i].frameIndex = *reinterpret_cast<const uint32_t*>(ptr);
            ptr += sizeof(uint32_t);

            outMotion.keyFrames[i].boneMatrices.resize(outMotion.boneCount);
            if (!checkBuffer(ptr, end, sizeof(Mat4) * outMotion.boneCount)) return false;

            std::memcpy(outMotion.keyFrames[i].boneMatrices.data(), ptr,
                       sizeof(Mat4) * outMotion.boneCount);
            ptr += sizeof(Mat4) * outMotion.boneCount;
        }
    }
    else
    {
        // 回退: 无关键帧标识,所有帧都是关键帧
        ptr -= 4;
        outMotion.keyFrames.resize(outMotion.frameCount);

        for (uint32_t i = 0; i < outMotion.frameCount; ++i)
        {
            outMotion.keyFrames[i].frameIndex = i;
            outMotion.keyFrames[i].boneMatrices.resize(outMotion.boneCount);

            if (!checkBuffer(ptr, end, sizeof(Mat4) * outMotion.boneCount)) return false;
            std::memcpy(outMotion.keyFrames[i].boneMatrices.data(), ptr,
                       sizeof(Mat4) * outMotion.boneCount);
            ptr += sizeof(Mat4) * outMotion.boneCount;
        }
    }

    // 读取Morph数据
    if (checkBuffer(ptr, end, sizeof(uint32_t)))
    {
        outMotion.morphCount = *reinterpret_cast<const uint32_t*>(ptr);
        ptr += sizeof(uint32_t);

        if (outMotion.morphCount > 0 && checkBuffer(ptr, end, sizeof(float) * outMotion.morphCount * outMotion.frameCount))
        {
            outMotion.morphWeights.resize(outMotion.morphCount * outMotion.frameCount);
            std::memcpy(outMotion.morphWeights.data(), ptr,
                       sizeof(float) * outMotion.morphWeights.size());
        }
    }

    AXLOG("C3FileReader: Loaded motion, %d bones, %d frames, %d keyframes",
          outMotion.boneCount, outMotion.frameCount, (int)outMotion.keyFrames.size());

    return true;
}

// 读取PTCL Chunk
bool C3FileReader::readParticleChunk(const uint8_t* data, size_t size, C3ParticleData& outParticle)
{
    const uint8_t* ptr = data;
    const uint8_t* end = data + size;

    // 读取名称
    outParticle.name = readString(ptr, end);

    // 读取纹理名称
    outParticle.textureName = readString(ptr, end);

    // 读取纹理行数
    if (!checkBuffer(ptr, end, sizeof(uint32_t))) return false;
    outParticle.textureRows = *reinterpret_cast<const uint32_t*>(ptr);
    ptr += sizeof(uint32_t);

    // 读取最大粒子数量
    if (!checkBuffer(ptr, end, sizeof(uint32_t))) return false;
    outParticle.maxParticles = *reinterpret_cast<const uint32_t*>(ptr);
    ptr += sizeof(uint32_t);

    // 读取帧数
    if (!checkBuffer(ptr, end, sizeof(uint32_t))) return false;
    uint32_t frameCount = *reinterpret_cast<const uint32_t*>(ptr);
    ptr += sizeof(uint32_t);

    outParticle.frames.resize(frameCount);

    // 读取每帧数据
    for (uint32_t i = 0; i < frameCount; ++i)
    {
        auto& frame = outParticle.frames[i];

        // 读取该帧粒子数量
        if (!checkBuffer(ptr, end, sizeof(uint32_t))) return false;
        frame.particleCount = *reinterpret_cast<const uint32_t*>(ptr);
        ptr += sizeof(uint32_t);

        if (frame.particleCount > 0)
        {
            // 读取位置
            if (!checkBuffer(ptr, end, sizeof(Vec3) * frame.particleCount)) return false;
            frame.positions.resize(frame.particleCount);
            std::memcpy(frame.positions.data(), ptr, sizeof(Vec3) * frame.particleCount);
            ptr += sizeof(Vec3) * frame.particleCount;

            // 读取年龄
            if (!checkBuffer(ptr, end, sizeof(float) * frame.particleCount)) return false;
            frame.ages.resize(frame.particleCount);
            std::memcpy(frame.ages.data(), ptr, sizeof(float) * frame.particleCount);
            ptr += sizeof(float) * frame.particleCount;

            // 读取大小
            if (!checkBuffer(ptr, end, sizeof(float) * frame.particleCount)) return false;
            frame.sizes.resize(frame.particleCount);
            std::memcpy(frame.sizes.data(), ptr, sizeof(float) * frame.particleCount);
            ptr += sizeof(float) * frame.particleCount;

            // 读取变换矩阵
            if (!checkBuffer(ptr, end, sizeof(Mat4))) return false;
            std::memcpy(&frame.transform, ptr, sizeof(Mat4));
            ptr += sizeof(Mat4);
        }
    }

    AXLOG("C3FileReader: Loaded particle '%s', %d max particles, %d frames, texture '%s'",
          outParticle.name.c_str(), outParticle.maxParticles, frameCount, outParticle.textureName.c_str());

    return true;
}

// 读取SHAP Chunk
bool C3FileReader::readShapeChunk(const uint8_t* data, size_t size, C3ShapeData& outShape)
{
    const uint8_t* ptr = data;
    const uint8_t* end = data + size;

    // 读取名称
    outShape.name = readString(ptr, end);

    // 读取线条数量
    if (!checkBuffer(ptr, end, sizeof(uint32_t))) return false;
    uint32_t lineCount = *reinterpret_cast<const uint32_t*>(ptr);
    ptr += sizeof(uint32_t);

    outShape.lines.resize(lineCount);

    // 读取每条线的顶点
    for (uint32_t i = 0; i < lineCount; ++i)
    {
        if (!checkBuffer(ptr, end, sizeof(uint32_t))) return false;
        uint32_t vertCount = *reinterpret_cast<const uint32_t*>(ptr);
        ptr += sizeof(uint32_t);

        if (!checkBuffer(ptr, end, sizeof(Vec3) * vertCount)) return false;
        outShape.lines[i].resize(vertCount);
        std::memcpy(outShape.lines[i].data(), ptr, sizeof(Vec3) * vertCount);
        ptr += sizeof(Vec3) * vertCount;
    }

    // 读取纹理名称
    outShape.textureName = readString(ptr, end);

    // 读取分段数量
    if (checkBuffer(ptr, end, sizeof(uint32_t)))
    {
        outShape.segmentCount = *reinterpret_cast<const uint32_t*>(ptr);
    }

    AXLOG("C3FileReader: Loaded shape '%s', %d lines, texture '%s'",
          outShape.name.c_str(), lineCount, outShape.textureName.c_str());

    return true;
}

} // namespace co2effect
} // namespace ax
