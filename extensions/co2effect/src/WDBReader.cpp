/****************************************************************************
 Copyright (c) 2025 Axmol Engine contributors
 ****************************************************************************/

#include "WDBReader.h"
#include "cocos2d.h"
#include <cstring>

namespace ax {
namespace co2effect {

using ax::FileUtils;
using ax::Data;

WDBReader::WDBReader() {}

WDBReader::~WDBReader()
{
    close();
}

bool WDBReader::open(const std::string& wdbPath)
{
    close();

    // 读取整个WDB文件到内存
    _wdbData = FileUtils::getInstance()->getDataFromFile(wdbPath);
    if (_wdbData.isNull() || _wdbData.getSize() < 16)
    {
        AXLOG("WDBReader: Failed to read WDB file: %s", wdbPath.c_str());
        return false;
    }

    const uint8_t* data = _wdbData.getBytes();
    size_t dataSize = _wdbData.getSize();

    // 检查Magic标识 "BDMG"
    if (memcmp(data, "BDMG", 4) != 0)
    {
        AXLOG("WDBReader: Invalid WDB magic, expected 'BDMG'");
        close();
        return false;
    }

    // 读取BDMG头部
    uint32_t totalSize = *reinterpret_cast<const uint32_t*>(data + 4);
    uint32_t dataOffset = *reinterpret_cast<const uint32_t*>(data + 8);
    uint32_t fileCount = *reinterpret_cast<const uint32_t*>(data + 12);

    AXLOG("WDBReader: BDMG Header - FileCount=%u, DataOffset=%u, TotalSize=%u",
          fileCount, dataOffset, totalSize);

    // 检查是否有RSDB段
    if (memcmp(data + 16, "RSDB", 4) != 0)
    {
        AXLOG("WDBReader: No RSDB segment found");
        close();
        return false;
    }

    // 读取RSDB头部
    uint32_t rsdbAmount = *reinterpret_cast<const uint32_t*>(data + 20);
    AXLOG("WDBReader: RSDB segment - Amount=%u", rsdbAmount);

    // RSDB Entry表从偏移24开始，每个条目12字节：UniqId(8) + FilenameOffset(4)
    size_t rsdbEntryOffset = 24;
    std::vector<std::pair<uint64_t, uint32_t>> rsdbEntries;  // <UniqId, FilenameOffset>

    for (uint32_t i = 0; i < rsdbAmount; ++i)
    {
        size_t entryPos = rsdbEntryOffset + i * 12;
        if (entryPos + 12 > dataSize)
        {
            AXLOG("WDBReader: RSDB entry overflow at index %u", i);
            break;
        }

        uint64_t uniqId = *reinterpret_cast<const uint64_t*>(data + entryPos);
        uint32_t fnameOffset = *reinterpret_cast<const uint32_t*>(data + entryPos + 8);
        rsdbEntries.push_back({uniqId, fnameOffset});
    }

    // 通过FilenameOffset读取文件名
    std::vector<std::string> fileNames;
    for (const auto& entry : rsdbEntries)
    {
        uint32_t fnameOffset = entry.second;
        if (fnameOffset >= dataSize)
        {
            fileNames.push_back("");
            continue;
        }

        const char* namePtr = reinterpret_cast<const char*>(data + fnameOffset);
        size_t maxLen = dataSize - fnameOffset;
        size_t len = strnlen(namePtr, maxLen);

        std::string fileName(namePtr, len);
        fileNames.push_back(fileName);
    }

    AXLOG("WDBReader: Parsed %zu filenames from RSDB", fileNames.size());

    // 现在需要读取文件数据索引表（在RSDB Entry表之后）
    // 数据索引格式：hash(4) + offset(4) + size(4) = 12字节，共fileCount个
    size_t dataIndexOffset = rsdbEntryOffset + rsdbAmount * 12;

    for (uint32_t i = 0; i < fileCount; ++i)
    {
        size_t entryPos = dataIndexOffset + i * 12;
        if (entryPos + 12 > dataSize)
        {
            AXLOG("WDBReader: Data index overflow at index %u", i);
            break;
        }

        uint32_t hash = *reinterpret_cast<const uint32_t*>(data + entryPos);
        uint32_t offset = *reinterpret_cast<const uint32_t*>(data + entryPos + 4);
        uint32_t size = *reinterpret_cast<const uint32_t*>(data + entryPos + 8);

        // 使用文件名作为key（如果有的话）
        if (i < fileNames.size() && !fileNames[i].empty())
        {
            _fileIndex[fileNames[i]] = {offset, size};
        }
    }

    AXLOG("WDBReader: Loaded %zu files", _fileIndex.size());

    // 调试：输出前5个文件名
    int debugCount = 0;
    for (const auto& pair : _fileIndex)
    {
        AXLOG("WDBReader:   [%d] %s (offset=%u, size=%u)",
              debugCount, pair.first.c_str(), pair.second.offset, pair.second.size);
        if (++debugCount >= 5) break;
    }

    return true;
}

void WDBReader::close()
{
    _wdbData.clear();
    _fileIndex.clear();
}

Data WDBReader::readFile(const std::string& filePath)
{
    if (_wdbData.isNull())
    {
        AXLOG("WDBReader: WDB not opened");
        return Data::Null;
    }

    auto it = _fileIndex.find(filePath);
    if (it == _fileIndex.end())
    {
        AXLOG("WDBReader: File not found in WDB: %s", filePath.c_str());
        return Data::Null;
    }

    const FileEntry& entry = it->second;
    if (entry.offset + entry.size > _wdbData.getSize())
    {
        AXLOG("WDBReader: Invalid file entry: offset=%u, size=%u", entry.offset, entry.size);
        return Data::Null;
    }

    // 复制文件数据
    uint8_t* fileData = (uint8_t*)malloc(entry.size);
    if (!fileData)
    {
        AXLOG("WDBReader: Failed to allocate memory for file: %s", filePath.c_str());
        return Data::Null;
    }

    memcpy(fileData, _wdbData.getBytes() + entry.offset, entry.size);

    Data result;
    result.fastSet(fileData, entry.size);
    return result;
}

bool WDBReader::hasFile(const std::string& filePath) const
{
    return _fileIndex.find(filePath) != _fileIndex.end();
}

std::vector<std::string> WDBReader::getFileList() const
{
    std::vector<std::string> result;
    result.reserve(_fileIndex.size());

    for (const auto& pair : _fileIndex)
    {
        result.push_back(pair.first);
    }

    return result;
}

} // namespace co2effect
} // namespace ax
