/****************************************************************************
 Copyright (c) 2025 Axmol Engine contributors
 ****************************************************************************/

#ifndef __CO2EFFECT_WDB_READER_H__
#define __CO2EFFECT_WDB_READER_H__

#include "cocos2d.h"
#include <string>
#include <unordered_map>

namespace ax {
namespace co2effect {

/**
 * WDB文件读取器 (BDMG格式)
 * WDB = Windsoul Data Bundle (魂之数据包)
 *
 * 文件结构:
 * [Header]
 *   Magic: "BDMG" (4 bytes)
 *   TotalSize: uint32
 *   DataOffset: uint32
 *   FileCount: uint32
 * [Index Table]
 *   Entry[]: {hash: uint32, offset: uint32, size: uint32} * FileCount
 * [Filename Table]
 *   Null-terminated strings
 * [Data Section]
 *   File data
 */
class WDBReader
{
public:
    WDBReader();
    ~WDBReader();

    /**
     * 打开WDB文件
     */
    bool open(const std::string& wdbPath);

    /**
     * 关闭WDB
     */
    void close();

    /**
     * 从WDB中读取文件
     * @param filePath 文件路径（例如："ini/+3DEffect.ini"）
     * @return 文件数据，失败返回空Data
     */
    ax::Data readFile(const std::string& filePath);

    /**
     * 检查文件是否存在
     */
    bool hasFile(const std::string& filePath) const;

    /**
     * 获取所有文件列表
     */
    std::vector<std::string> getFileList() const;

    /**
     * 是否已打开
     */
    bool isOpen() const { return !_wdbData.isNull(); }

private:
    struct FileEntry
    {
        uint32_t offset;
        uint32_t size;
    };

    ax::Data _wdbData;
    std::unordered_map<std::string, FileEntry> _fileIndex;
};

} // namespace co2effect
} // namespace ax

#endif // __CO2EFFECT_WDB_READER_H__
