#ifndef ANNO_BLOCK_H
#define ANNO_BLOCK_H 

#include "Block.h"

#include <glog/logging.h>

using namespace DataArray_namespace;

namespace BlockManager_namespace {

enum class AnnoBlockFormat {
    RAW = 0
};

class AnnoBlock32: public Block {
public:
    AnnoBlock32(const std::string& path_name): Block(path_name) {}
    AnnoBlock32(const std::string& path_name, int xdim, int ydim, int zdim, size_t dtype_size): Block(path_name, xdim, ydim, zdim, dtype_size) {}
    ~AnnoBlock32() { _flush(); }

    void load();
    void save();

protected:
    AnnoBlockFormat _format = AnnoBlockFormat::RAW;
    
    void _saveRaw();
};

}

#endif // ANNO_BLOCK_H