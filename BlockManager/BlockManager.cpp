#include "BlockManager.h"

#include "../Util/Morton.h"

#include <cmath>
#include <algorithm>

using namespace BlockManager_namespace;
namespace fs = boost::filesystem;

BlockManager::BlockManager(const std::string& directory_path_name, std::shared_ptr<Manifest> manifestShPtr): directory_path_name(directory_path_name), manifest(manifestShPtr) {
    _init();
}

BlockManager::~BlockManager() {
    // _flush(); // TODO(adb): this happens automatically in each block
}

std::array<int, 3> BlockManager::getChunkSizeForScale(const std::string& scale_key) {
    const auto scale = manifest->get_scale(scale_key);
 
    CHECK(scale.chunk_sizes.size() > 0);
    if (scale.chunk_sizes.size() > 1) LOG(WARNING) << "This dataset has multiple chunk_size options. Undefined behavior may occur!";
    return scale.chunk_sizes[0];
}

std::vector< BlockKey > BlockManager::_blocksForBoundingBox(const std::array<int, 2>& xrng, const std::array<int, 2>& yrng, const std::array<int, 2>& zrng, const std::string& scale_key) {
    const auto scale = manifest->get_scale(scale_key);
    // Construct the bounding box by translating the xyz range by -voxel_offset and divivding by the chunk_size (assuming just {x,y,z} chunks here)
    const auto voxel_offset = scale.voxel_offset;
    std::array<int, 2> _xrng = {{xrng[0] - voxel_offset[0], xrng[1] - voxel_offset[1]}};
    std::array<int, 2> _yrng = {{yrng[0] - voxel_offset[0], yrng[1] - voxel_offset[1]}};
    std::array<int, 2> _zrng = {{zrng[0] - voxel_offset[0], zrng[1] - voxel_offset[1]}};
    
    const auto chunk_size = getChunkSizeForScale(scale_key);

    _xrng[0] = floor(_xrng[0] / chunk_size[0]);
    _xrng[1] = ceil(_xrng[1] / chunk_size[0]);

    _yrng[0] = floor(_yrng[0] / chunk_size[1]);
    _yrng[1] = ceil(_yrng[1] / chunk_size[1]);

    _zrng[0] = floor(_zrng[0] / chunk_size[2]);
    _zrng[1] = ceil(_zrng[1] / chunk_size[2]);
    
    std::vector< BlockKey > ret;
    ret.reserve((_xrng[1] - _xrng[0] + 1) * (_yrng[1] - _yrng[0] + 1) * (_zrng[1] - _zrng[0] + 1));
    for(int x = _xrng[0]; x < _xrng[1]; x++ ) {
        for(int y = _yrng[0]; y < _yrng[1]; y++) {
            for(int z = _zrng[0]; z < _zrng[1]; z++) {
                uint64_t morton_idx = Morton64::XYZMorton(std::array<int, 3>({{x, y, z}}));
                ret.push_back(BlockKey({morton_idx, x, y, z}));
            }
        }
    }

    std::sort(ret.begin(), ret.end());
    return ret;
}

void BlockManager::_init() {
    // Scan the directory containing the manifest and index all the files for each resolution
    for(const auto& scale : manifest->_scales) {
        std::shared_ptr<BlockMortonIndexMap> blockIdxMap = _createIndexForScale(scale.key);
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

std::shared_ptr<BlockMortonIndexMap> BlockManager::_createIndexForScale(const std::string& scale) {
#if 1
    std::shared_ptr<BlockMortonIndexMap> ret = std::make_shared<BlockMortonIndexMap>();

    // For each scale, iterate through the files in that directory
    const auto path = fs::path(directory_path_name);
    CHECK(fs::is_directory(path / fs::path(scale))) << "Error: Directory path for scale " << scale << " is not a directory!";

    for(auto& file : fs::directory_iterator(path)) {
        LOG(INFO) << file.path();
        // TODO(adb): create block for the file
        // insert block into the map 
    }
    return ret;
#endif
}