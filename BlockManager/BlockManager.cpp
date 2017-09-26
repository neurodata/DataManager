#include "BlockManager.h"

#include "../Util/Morton.h"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <string>

using namespace BlockManager_namespace;
namespace fs = boost::filesystem;

BlockManager::BlockManager(const std::string& directory_path_name, std::shared_ptr<Manifest> manifestShPtr,
                           const BlockDataStore blockDataStore, const BlockSettings& blockSettings)
    : directory_path_name(directory_path_name), manifest(manifestShPtr), _blockDataStore(blockDataStore) {
    _blockSettingsPtr = std::make_shared<BlockSettings>(blockSettings);
    _init();
}

BlockManager::~BlockManager() {
    // _flush(); // TODO(adb): this happens automatically in each block, so I
    // think we can remove here
}

std::array<int, 3> BlockManager::getChunkSizeForScale(const std::string& scale_key) {
    const auto scale = manifest->get_scale(scale_key);

    CHECK(scale.chunk_sizes.size() > 0);
    if (scale.chunk_sizes.size() > 1)
        LOG(WARNING) << "This dataset has multiple chunk_size options. "
                        "Undefined behavior may occur!";
    return scale.chunk_sizes[0];
}

std::array<int, 3> BlockManager::getVoxelOffsetForScale(const std::string& scale_key) {
    const auto scale = manifest->get_scale(scale_key);
    return std::array<int, 3>({{scale.voxel_offset[0], scale.voxel_offset[1], scale.voxel_offset[2]}});
}

BlockEncoding BlockManager::getEncodingForScale(const std::string& scale_key) {
    const auto scale = manifest->get_scale(scale_key);
    if (scale.encoding == std::string("raw")) {
        return BlockEncoding::RAW;
    } else if (scale.encoding == std::string("compressed_segmentation")) {
        return BlockEncoding::COMPRESSED_SEGMENTATION;
    } else if (scale.encoding == std::string("jpeg")) {
        return BlockEncoding::JPEG;
    } else {
        LOG(FATAL) << "Unable to parse block encoding string " << scale.encoding;
        return BlockEncoding::RAW;
    }
}

std::vector<BlockKey> BlockManager::_blocksForBoundingBox(const std::array<int, 2>& xrng,
                                                          const std::array<int, 2>& yrng,
                                                          const std::array<int, 2>& zrng,
                                                          const std::string& scale_key) {
    const auto scale = manifest->get_scale(scale_key);
    const auto chunk_size = getChunkSizeForScale(scale_key);

    std::array<int, 2> _xrng;
    std::array<int, 2> _yrng;
    std::array<int, 2> _zrng;

    _xrng[0] = floor(static_cast<double>(xrng[0]) / static_cast<double>(chunk_size[0]));
    _xrng[1] = ceil(static_cast<double>(xrng[1]) / static_cast<double>(chunk_size[0]));

    _yrng[0] = floor(static_cast<double>(yrng[0]) / static_cast<double>(chunk_size[1]));
    _yrng[1] = ceil(static_cast<double>(yrng[1]) / static_cast<double>(chunk_size[1]));

    _zrng[0] = floor(static_cast<double>(zrng[0]) / static_cast<double>(chunk_size[2]));
    _zrng[1] = ceil(static_cast<double>(zrng[1]) / static_cast<double>(chunk_size[2]));

    std::vector<BlockKey> ret;
    ret.reserve((_xrng[1] - _xrng[0] + 1) * (_yrng[1] - _yrng[0] + 1) * (_zrng[1] - _zrng[0] + 1));
    for (int x = _xrng[0]; x < _xrng[1]; x++) {
        for (int y = _yrng[0]; y < _yrng[1]; y++) {
            for (int z = _zrng[0]; z < _zrng[1]; z++) {
                uint64_t morton_idx = Morton64::XYZMorton(std::array<int, 3>({{x, y, z}}));
                ret.push_back(BlockKey({morton_idx, x, y, z}));
            }
        }
    }

    std::sort(ret.begin(), ret.end());
    return ret;
}

void BlockManager::_init() {
    // Parse the block datatype from the root manifest file
    const auto data_type_str = manifest->data_type();
    if (data_type_str == std::string("uint8")) {
        _blockDataType = BlockDataType::UINT8;
    } else if (data_type_str == std::string("uint16")) {
        _blockDataType = BlockDataType::UINT16;
    } else if (data_type_str == std::string("uint32")) {
        _blockDataType = BlockDataType::UINT32;
    } else if (data_type_str == std::string("uint64")) {
        _blockDataType = BlockDataType::UINT64;
    } else {
        LOG(FATAL) << "Unable to parse data type string: " << data_type_str;
    }

    // Scan the directory containing the manifest and index all the files for
    // each resolution
    for (const auto& scale : manifest->_scales) {
        std::shared_ptr<BlockMortonIndexMap> blockIdxMap = _createIndexForScale(scale.key);
        LOG(INFO) << "Read " << blockIdxMap->size() << " blocks for scale " << scale.key;
        block_index_by_res.insert(std::make_pair(scale.key, blockIdxMap));
    }
}

#if 0
void BlockManager::_flush() {
    for(const auto& scale_itr : block_index_by_res) {
        for(const auto& block_itr : scale_itr->second) {
            block_itr->second->save();
        }
    }
}
#endif

BlockKey BlockManager::GetBlockKeyForName(const std::string& filename, const std::array<int, 3> chunk_size) {
    std::stringstream ss(filename);
    std::string item;
    std::array<int, 3> coords;
    int counter = 0;
    std::string delim("_");
    while (std::getline(ss, item, *delim.c_str())) {
        CHECK(counter < 3);
        auto pos = item.find(std::string("-"));
        CHECK(pos != std::string::npos);
        coords[counter] = std::stoi(item.substr(0, pos));
        counter++;
    }
    for (int i = 0; i < 3; i++) {
        coords[i] = floor(coords[i] / (double)chunk_size[i]);
    }
    const auto morton_index = Morton64::XYZMorton(coords);
    return {morton_index, coords[0], coords[1], coords[2]};
}

std::shared_ptr<BlockMortonIndexMap> BlockManager::_createIndexForScale(const std::string& scale_key) {
    std::shared_ptr<BlockMortonIndexMap> ret = std::make_shared<BlockMortonIndexMap>();

    LOG(INFO) << "Creating index for scale " << scale_key;

    const auto chunk_size = getChunkSizeForScale(scale_key);
    const auto block_encoding = getEncodingForScale(scale_key);

    // For each scale, iterate through the files in that directory
    const auto path = fs::path(directory_path_name) / fs::path(scale_key);
    CHECK(fs::is_directory(path)) << "Error: Directory path for scale " << scale_key << " is not a directory!";

    for (auto& file : fs::directory_iterator(path)) {
        // Parse the block key
        const auto blockKey = GetBlockKeyForName(file.path().filename().string(), chunk_size);

        // Create block
        auto blockShPtr =
            std::make_shared<FilesystemBlock>(file.path().string(), chunk_size[0], chunk_size[1], chunk_size[2],
                                              sizeof(uint32_t), block_encoding, _blockDataType, _blockSettingsPtr);

        // Insert block
        ret->insert(std::make_pair(blockKey, blockShPtr));
    }
    return ret;
}