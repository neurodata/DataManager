/*  Copyright 2017 NeuroData
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include "BlockManager.h"

#include "../Util/Morton.h"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <string>

using namespace BlockManager_namespace;

BlockManager::BlockManager(std::shared_ptr<Manifest> manifestShPtr, std::shared_ptr<BlockDataStore> blockDataStoreShPtr,
                           const BlockSettings& blockSettings)
    : manifest(manifestShPtr), _dataStore(blockDataStoreShPtr) {
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
        LOG(WARNING) << "This dataset has multiple chunk_size options. Undefined behavior will occur!";
    return scale.chunk_sizes[0];
}

std::array<int, 3> BlockManager::getVoxelOffsetForScale(const std::string& scale_key) {
    const auto scale = manifest->get_scale(scale_key);
    return std::array<int, 3>({{scale.voxel_offset[0], scale.voxel_offset[1], scale.voxel_offset[2]}});
}

std::array<int, 3> BlockManager::getSizeForScale(const std::string& scale_key) {
    const auto scale = manifest->get_scale(scale_key);
    return std::array<int, 3>({{scale.size[0], scale.size[1], scale.size[2]}});
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

// TODO(adb): refactor based on the more logical variable names in Put/Get
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

    // Build a map for storing blocks read in for each scale
    for (const auto& scale : manifest->_scales) {
        block_index_by_res.insert(std::make_pair(scale.key, std::make_shared<BlockMortonIndexMap>()));
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

std::array<int, 3> BlockManager::BlockStart(const BlockKey& block_key, const std::array<int, 3>& block_size) {
    return std::array<int, 3>({block_key.x * block_size[0], block_key.y * block_size[1], block_key.z * block_size[2]});
}

std::array<int, 3> BlockManager::BlockEnd(const BlockKey& block_key, const std::array<int, 3>& block_size,
                                          const std::array<int, 3>& image_size) {
    auto ret = std::array<int, 3>(
        {(block_key.x + 1) * block_size[0], (block_key.y + 1) * block_size[1], (block_key.z + 1) * block_size[2]});
    for (int i = 0; i < 3; i++)
        if (ret[i] > image_size[i]) ret[i] = image_size[i];
    return ret;
}

std::array<int, 3> BlockManager::BlockSizeFromExtents(const std::array<int, 3>& block_start,
                                                      const std::array<int, 3>& block_end) {
    return std::array<int, 3>(
        {block_end[0] - block_start[0], block_end[1] - block_start[1], block_end[2] - block_start[2]});
}

std::pair<std::array<int, 3>, std::array<int, 3>> BlockManager::GetDataView(const std::array<int, 3>& block_start,
                                                                            const std::array<int, 3>& block_end,
                                                                            const std::array<int, 3> cutout_start,
                                                                            const std::array<int, 3> cutout_end) {
    std::pair<std::array<int, 3>, std::array<int, 3>> ret = std::make_pair(cutout_start, cutout_end);

    for (int i = 0; i < 3; i++) {
        if (cutout_start[i] < block_start[i]) ret.first[i] = block_start[i];
        if (cutout_end[i] > block_end[i]) ret.second[i] = block_end[i];
    }

    return ret;
}