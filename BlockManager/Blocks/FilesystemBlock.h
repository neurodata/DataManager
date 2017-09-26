#ifndef FS_BLOCK_H
#define FS_BLOCK_H

#include "Block.h"

namespace BlockManager_namespace {

class FilesystemBlock : public Block {
   public:
    FilesystemBlock(const std::string& path_name, int xdim, int ydim, int zdim, size_t dtype_size, BlockEncoding format,
                    BlockDataType data_type, const std::shared_ptr<BlockSettings>& blockSettings)
        : Block(path_name, xdim, ydim, zdim, dtype_size, format, data_type, blockSettings) {}
    ~FilesystemBlock() {}

    void load();
    void save();
};
}

#endif  // FS_BLOCK_H