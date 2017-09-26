#ifndef ANNO_BLOCK_H
#define ANNO_BLOCK_H

#include "Block.h"

#include <glog/logging.h>

using namespace DataArray_namespace;

namespace BlockManager_namespace {

// TODO(adb): we should probably move this to Block.h... in fact we could
// probably collapse AnnoBlock and ImageBlock into a generic templated Block and
// then subclass the save / mode methods based on backend
enum class AnnoBlockFormat { RAW = 0, COMPRESSED_SEGMENTATION };

class AnnoBlock32 : public Block {
public:
  AnnoBlock32(const std::string &path_name) : Block(path_name) {}
  AnnoBlock32(const std::string &path_name, int xdim, int ydim, int zdim,
              size_t dtype_size, bool gzip, const std::string &encoding)
      : Block(path_name, xdim, ydim, zdim, dtype_size, gzip) {
    // TODO(adb): put these in some sort of types file
    if (encoding == std::string("raw")) {
      _format = AnnoBlockFormat::RAW;
    } else if (encoding == std::string("compressed_segmentation")) {
      _format = AnnoBlockFormat::COMPRESSED_SEGMENTATION;
    } else {
      LOG(FATAL) << "Unrecognized AnnoBlock encoding: " << encoding;
    }
  }
  ~AnnoBlock32() { _flush(); }

  void load();
  void save();

protected:
  AnnoBlockFormat _format = AnnoBlockFormat::RAW;

  void _loadRaw();
  void _saveRaw();
  void _loadCompressedSegmentation();
  void _saveCompressedSegmentation();
};

} // namespace BlockManager_namespace

#endif // ANNO_BLOCK_H