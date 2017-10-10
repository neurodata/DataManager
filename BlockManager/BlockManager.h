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

#ifndef BLOCK_MANAGER_H
#define BLOCK_MANAGER_H

// #include "Blocks/Block.h"
#include "Blocks/FilesystemBlock.h"
#include "Manifest.h"

#include "../DataArray/DataArray.h"

#include <glog/logging.h>
#include <boost/filesystem.hpp>

#include <map>
#include <memory>
#include <unordered_map>
#include <vector>

namespace BlockManager_namespace {
namespace fs = boost::filesystem;

enum class BlockDataStore { FILESYSTEM };

struct BlockKey {
    uint64_t morton_index;
    int x;
    int y;
    int z;
    bool operator<(const BlockKey& other) const { return (morton_index < other.morton_index); }
};

typedef std::map<BlockKey, std::shared_ptr<Block>> BlockMortonIndexMap;

class BlockManager {
   public:
    BlockManager(const std::string& directory_path_name, std::shared_ptr<Manifest> manifestShPtr,
                 const BlockDataStore blockDataStore, const BlockSettings& settings);
    ~BlockManager();

    template <typename T>
    void Put(const DataArray_namespace::DataArray3D<T>& data, const std::array<int, 2>& xrng,
             const std::array<int, 2>& yrng, const std::array<int, 2>& zrng, const std::string& scale_key,
             bool subtractVoxelOffset = false) {
        std::array<int, 2> _xrng = xrng;
        std::array<int, 2> _yrng = yrng;
        std::array<int, 2> _zrng = zrng;

        const auto voxel_offset = getVoxelOffsetForScale(scale_key);
        if (subtractVoxelOffset) {
            _xrng[0] -= voxel_offset[0];
            _xrng[1] -= voxel_offset[0];

            _yrng[0] -= voxel_offset[1];
            _yrng[1] -= voxel_offset[1];

            _zrng[0] -= voxel_offset[2];
            _zrng[1] -= voxel_offset[2];
        }
        const auto chunk_size = getChunkSizeForScale(scale_key);
        const auto block_encoding = getEncodingForScale(scale_key);

        auto block_keys = _blocksForBoundingBox(_xrng, _yrng, _zrng, scale_key);
        auto blockMortonIndexMapItr = block_index_by_res.find(scale_key);
        CHECK(blockMortonIndexMapItr != block_index_by_res.end())
            << "Failed to find scale key " << scale_key << " in block map.";
        auto blockMortonIndexMap = blockMortonIndexMapItr->second;
        for (const auto& block_key : block_keys) {
            // Translate the block_key to voxel space coordinates
            std::array<int, 2> _x({{(block_key.x * chunk_size[0]), ((block_key.x + 1) * chunk_size[0])}});
            std::array<int, 2> _y({{(block_key.y * chunk_size[1]), ((block_key.y + 1) * chunk_size[1])}});
            std::array<int, 2> _z({{(block_key.z * chunk_size[2]), ((block_key.z + 1) * chunk_size[2])}});

            // Make sure the view matches the dimensionality of the input data
            int x_arr_offset = 0;
            int y_arr_offset = 0;
            int z_arr_offset = 0;
            if (_x[0] < _xrng[0]) {
                x_arr_offset = _xrng[0] - _x[0];
                _x[0] = _xrng[0];
            }
            if (_x[1] > _xrng[1]) {
                _x[1] = _xrng[1];
            }
            if (_y[0] < _yrng[0]) {
                y_arr_offset = _yrng[0] - _y[0];
                _y[0] = _yrng[0];
            }
            if (_y[1] > _yrng[1]) {
                _y[1] = _yrng[1];
            }
            if (_z[0] < _zrng[0]) {
                z_arr_offset = _zrng[0] - _z[0];
                _z[0] = _zrng[0];
            }
            if (_z[1] > _zrng[1]) {
                _z[1] = _zrng[1];
            }

            // Scale by the offset of the input dataset
            std::array<int, 2> _xview = _x;
            std::array<int, 2> _yview = _y;
            std::array<int, 2> _zview = _z;

            _xview[0] -= _xrng[0];
            _xview[1] -= _xrng[0];
            _yview[0] -= _yrng[0];
            _yview[1] -= _yrng[0];
            _zview[0] -= _zrng[0];
            _zview[1] -= _zrng[0];

            const auto arr_view = data.view(_xview, _yview, _zview);

            auto itr = blockMortonIndexMap->find(block_key);
            std::shared_ptr<Block> blockShPtr;
            if (itr == blockMortonIndexMap->end()) {
                // Create a new block and add it to the map
                std::string block_name = Block::SetNeuroglancerFileName(
                    static_cast<int>(block_key.x * chunk_size[0]), static_cast<int>((block_key.x + 1) * chunk_size[0]),
                    static_cast<int>(block_key.y * chunk_size[1]), static_cast<int>((block_key.y + 1) * chunk_size[1]),
                    static_cast<int>(block_key.z * chunk_size[2]), static_cast<int>((block_key.z + 1) * chunk_size[2]),
                    voxel_offset);
                const auto block_path = fs::path(directory_path_name) / fs::path(scale_key) / fs::path(block_name);
                std::string block_path_name = block_path.string();
                switch (_blockDataStore) {
                    case BlockDataStore::FILESYSTEM: {
                        blockShPtr = std::make_shared<FilesystemBlock>(block_path_name, chunk_size[0], chunk_size[1],
                                                                       chunk_size[2], sizeof(uint32_t), block_encoding,
                                                                       _blockDataType, _blockSettingsPtr);
                    } break;
                    default: { LOG(FATAL) << "Unknown backing datastore."; }
                }
                blockShPtr->zero_block();
                blockMortonIndexMap->insert(std::make_pair(block_key, blockShPtr));
            } else {
                switch (_blockDataStore) {
                    case BlockDataStore::FILESYSTEM: {
                        blockShPtr = std::dynamic_pointer_cast<FilesystemBlock>(itr->second);
                    } break;
                    default: { LOG(FATAL) << "Unkown backing datastore."; }
                }
            }
            blockShPtr->add<uint32_t>(arr_view, x_arr_offset, y_arr_offset, z_arr_offset);
        }
        return;
    }

    template <typename T>
    void Get(DataArray_namespace::DataArray3D<T> output, const std::array<int, 2>& xrng, const std::array<int, 2>& yrng,
             const std::array<int, 2>& zrng, const std::string& scale_key, bool subtractVoxelOffset = false) {
        std::array<int, 2> _xrng = xrng;
        std::array<int, 2> _yrng = yrng;
        std::array<int, 2> _zrng = zrng;

        const auto voxel_offset = getVoxelOffsetForScale(scale_key);
        if (subtractVoxelOffset) {
            _xrng[0] -= voxel_offset[0];
            _xrng[1] -= voxel_offset[0];

            _yrng[0] -= voxel_offset[1];
            _yrng[1] -= voxel_offset[1];

            _zrng[0] -= voxel_offset[2];
            _zrng[1] -= voxel_offset[2];
        }
        const auto chunk_size = getChunkSizeForScale(scale_key);

        auto block_keys = _blocksForBoundingBox(_xrng, _yrng, _zrng, scale_key);
        auto blockMortonIndexMapItr = block_index_by_res.find(scale_key);
        CHECK(blockMortonIndexMapItr != block_index_by_res.end())
            << "Failed to find scale key " << scale_key << " in block map.";
        auto blockMortonIndexMap = blockMortonIndexMapItr->second;
        for (const auto& block_key : block_keys) {
            auto itr = blockMortonIndexMap->find(block_key);
            if (itr != blockMortonIndexMap->end()) {
                // add to the output array

                std::shared_ptr<Block> blockShPtr = itr->second;

                // Translate the block_key to voxel space coordinates
                std::array<int, 2> _x({{(block_key.x * chunk_size[0]), ((block_key.x + 1) * chunk_size[0])}});
                std::array<int, 2> _y({{(block_key.y * chunk_size[1]), ((block_key.y + 1) * chunk_size[1])}});
                std::array<int, 2> _z({{(block_key.z * chunk_size[2]), ((block_key.z + 1) * chunk_size[2])}});

                // Make sure the view matches the dimensionality of the output data
                int x_arr_offset = 0;
                int y_arr_offset = 0;
                int z_arr_offset = 0;
                if (_x[0] < _xrng[0]) {
                    x_arr_offset = _xrng[0] - _x[0];
                    _x[0] = _xrng[0];
                }
                if (_x[1] > _xrng[1]) {
                    _x[1] = _xrng[1];
                }
                if (_y[0] < _yrng[0]) {
                    y_arr_offset = _yrng[0] - _y[0];
                    _y[0] = _yrng[0];
                }
                if (_y[1] > _yrng[1]) {
                    _y[1] = _yrng[1];
                }
                if (_z[0] < _zrng[0]) {
                    z_arr_offset = _zrng[0] - _z[0];
                    _z[0] = _zrng[0];
                }
                if (_z[1] > _zrng[1]) {
                    _z[1] = _zrng[1];
                }

                // Scale by the offset of the input dataset
                std::array<int, 2> _xview = _x;
                std::array<int, 2> _yview = _y;
                std::array<int, 2> _zview = _z;

                _xview[0] -= _xrng[0];
                _xview[1] -= _xrng[0];
                _yview[0] -= _yrng[0];
                _yview[1] -= _yrng[0];
                _zview[0] -= _zrng[0];
                _zview[1] -= _zrng[0];

                auto arr_view = output.view(_xview, _yview, _zview);
                blockShPtr->get<T>(arr_view, x_arr_offset, y_arr_offset, z_arr_offset);
            }
        }
        return;
    }

    std::array<int, 3> getChunkSizeForScale(const std::string& scale_key);
    std::array<int, 3> getVoxelOffsetForScale(const std::string& scale_key);
    BlockEncoding getEncodingForScale(const std::string& scale_key);

    static BlockKey GetBlockKeyForName(const std::string& filename, const std::array<int, 3> chunk_size);

   protected:
    std::vector<BlockKey> _blocksForBoundingBox(const std::array<int, 2>& xrng, const std::array<int, 2>& yrng,
                                                const std::array<int, 2>& zrng, const std::string& scale_key);
    void _init();
    // void _flush(); TODO(adb): automatically flushed on destruction, but maybe
    // we want to implement this eventually
    std::shared_ptr<BlockMortonIndexMap> _createIndexForScale(const std::string& scale);

    std::string directory_path_name;
    std::unordered_map<std::string, std::shared_ptr<BlockMortonIndexMap>> block_index_by_res;
    std::shared_ptr<Manifest> manifest;
    BlockDataStore _blockDataStore;
    BlockDataType _blockDataType;
    std::shared_ptr<BlockSettings> _blockSettingsPtr;
};

}  // namespace BlockManager_namespace

#endif  // BLOCK_MANAGER_H