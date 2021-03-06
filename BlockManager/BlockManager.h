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

#include "Blocks/Block.h"
#include "Blocks/Types.h"
#include "Datastore/BlockDataStore.h"
#include "Manifest.h"

#include "../DataArray/DataArray.h"

#include <glog/logging.h>

#include <map>
#include <memory>
#include <unordered_map>
#include <vector>

namespace BlockManager_namespace {

typedef std::map<BlockKey, BlockShPtr> BlockMortonIndexMap;

class BlockManager {
   public:
    BlockManager(std::shared_ptr<Manifest> manifestShPtr, std::shared_ptr<BlockDataStore> blockDataStoreShPtr,
                 const BlockSettings& settings);
    ~BlockManager();

    /**
     * Put takes as input a 3D matrix containing input data and the cutout region corresponding to the data's location
     * in the dataspace. The cutout region is specified as a bounding box in 3D space, and can either be in 0-indexed
     * coordinates or the coordinates of the data space. If subtractVoxelOffset is true, the cutout is in the
     * coordinates of the dataspace and we need to subtract the voxel offset from the cutout. Note that while block keys
     * typically use voxel coordinates, block data is stored using image coordinates after being read from the
     * datasource.
     */
    template <typename T>
    void Put(const DataArray_namespace::DataArray<T>& data, const std::array<int, 2>& xrng,
             const std::array<int, 2>& yrng, const std::array<int, 2>& zrng, const std::string& scale_key,
             bool subtractVoxelOffset = false) {
        auto cutout_start = std::array<int, 3>({xrng[0], yrng[0], zrng[0]});
        auto cutout_end = std::array<int, 3>({xrng[1], yrng[1], zrng[1]});

        const auto voxel_offset = getVoxelOffsetForScale(scale_key);
        auto cutout_start_abs = cutout_start;
        auto cutout_end_abs = cutout_end;
        if (subtractVoxelOffset) {
            for (int i = 0; i < 3; i++) {
                cutout_start_abs[i] -= voxel_offset[i];
                cutout_end_abs[i] -= voxel_offset[i];
            }
        }
        const auto image_size = getSizeForScale(scale_key);
        const auto chunk_size = getChunkSizeForScale(scale_key);
        const auto block_encoding = getEncodingForScale(scale_key);

        auto block_keys =
            _blocksForBoundingBox(std::array<int, 2>({cutout_start_abs[0], cutout_end_abs[0]}),
                                  std::array<int, 2>({cutout_start_abs[1], cutout_end_abs[1]}),
                                  std::array<int, 2>({cutout_start_abs[2], cutout_end_abs[2]}), scale_key);

        auto blockMortonIndexMapItr = block_index_by_res.find(scale_key);
        CHECK(blockMortonIndexMapItr != block_index_by_res.end())
            << "Failed to find scale key " << scale_key << " in block map.";
        auto blockMortonIndexMap = blockMortonIndexMapItr->second;
        for (const auto& block_key : block_keys) {
            // Note that the block key is expected to be 0-indexed (in image space)
            auto block_start = BlockManager::BlockStart(block_key, chunk_size);
            auto block_end = BlockManager::BlockEnd(block_key, chunk_size, image_size);
            auto block_size = BlockManager::BlockSizeFromExtents(block_start, block_end);

            // Get the portion of the cutout that lives within this block
            const auto block_restricted_cutout =
                BlockManager::GetDataView(block_start, block_end, cutout_start_abs, cutout_end_abs);

            // Subtract off the cutout starting coordinates (in image space) for the input data view
            auto xview = std::array<int, 2>({block_restricted_cutout.first[0] - cutout_start_abs[0],
                                             block_restricted_cutout.second[0] - cutout_start_abs[0]});
            auto yview = std::array<int, 2>({block_restricted_cutout.first[1] - cutout_start_abs[1],
                                             block_restricted_cutout.second[1] - cutout_start_abs[1]});
            auto zview = std::array<int, 2>({block_restricted_cutout.first[2] - cutout_start_abs[2],
                                             block_restricted_cutout.second[2] - cutout_start_abs[2]});

            const auto input_data_view = data.view(xview, yview, zview);

            auto itr = blockMortonIndexMap->find(block_key);
            BlockShPtr blockShPtr;
            if (itr == blockMortonIndexMap->end()) {
                // Create a new block and add it to the map
                const auto block_name = _dataStore->BlockName(block_start[0], block_end[0], block_start[1],
                                                              block_end[1], block_start[2], block_end[2], voxel_offset);

                blockShPtr = _dataStore->CreateBlock(block_name, scale_key, block_size[0], block_size[1], block_size[2],
                                                     sizeof(T), block_encoding, _blockDataType, _blockSettingsPtr);
                blockMortonIndexMap->insert(std::make_pair(block_key, blockShPtr));
            } else {
                blockShPtr = itr->second;
            }

            // Offset if the cutout starts somewhere in the middle of the block
            int x_block_offset = block_restricted_cutout.first[0] - block_start[0];
            int y_block_offset = block_restricted_cutout.first[1] - block_start[1];
            int z_block_offset = block_restricted_cutout.first[2] - block_start[2];

            blockShPtr->add<T>(input_data_view, x_block_offset, y_block_offset, z_block_offset);
        }
        return;
    }

    template <typename T>
    void Get(DataArray_namespace::DataArray<T> output, const std::array<int, 2>& xrng, const std::array<int, 2>& yrng,
             const std::array<int, 2>& zrng, const std::string& scale_key, bool subtractVoxelOffset = false) {
        auto cutout_start = std::array<int, 3>({xrng[0], yrng[0], zrng[0]});
        auto cutout_end = std::array<int, 3>({xrng[1], yrng[1], zrng[1]});

        const auto voxel_offset = getVoxelOffsetForScale(scale_key);
        auto cutout_start_abs = cutout_start;
        auto cutout_end_abs = cutout_end;
        if (subtractVoxelOffset) {
            for (int i = 0; i < 3; i++) {
                cutout_start_abs[i] -= voxel_offset[i];
                cutout_end_abs[i] -= voxel_offset[i];
            }
        }
        const auto image_size = getSizeForScale(scale_key);
        const auto chunk_size = getChunkSizeForScale(scale_key);
        const auto block_encoding = getEncodingForScale(scale_key);

        auto block_keys =
            _blocksForBoundingBox(std::array<int, 2>({cutout_start_abs[0], cutout_end_abs[0]}),
                                  std::array<int, 2>({cutout_start_abs[1], cutout_end_abs[1]}),
                                  std::array<int, 2>({cutout_start_abs[2], cutout_end_abs[2]}), scale_key);

        auto blockMortonIndexMapItr = block_index_by_res.find(scale_key);
        CHECK(blockMortonIndexMapItr != block_index_by_res.end())
            << "Failed to find scale key " << scale_key << " in block map.";
        auto blockMortonIndexMap = blockMortonIndexMapItr->second;
        for (const auto& block_key : block_keys) {
            auto itr = blockMortonIndexMap->find(block_key);

            auto block_start = BlockManager::BlockStart(block_key, chunk_size);
            auto block_end = BlockManager::BlockEnd(block_key, chunk_size, image_size);

            BlockShPtr blockShPtr;
            if (itr == blockMortonIndexMap->end()) {
                // If the block isn't in our map, we need to query the datastore
                const auto block_name = _dataStore->BlockName(block_start[0], block_end[0], block_start[1],
                                                              block_end[1], block_start[2], block_end[2], voxel_offset);
                auto block_size = BlockManager::BlockSizeFromExtents(block_start, block_end);

                blockShPtr = _dataStore->GetBlock(block_name, scale_key, block_size[0], block_size[1], block_size[2],
                                                  sizeof(T), block_encoding, _blockDataType, _blockSettingsPtr);
                if (!blockShPtr) continue;
            } else {
                // add to the output array
                blockShPtr = itr->second;
            }

            // Get the portion of the cutout that lives within this block
            const auto block_restricted_cutout =
                BlockManager::GetDataView(block_start, block_end, cutout_start_abs, cutout_end_abs);

            auto xview = std::array<int, 2>({block_restricted_cutout.first[0] - cutout_start_abs[0],
                                             block_restricted_cutout.second[0] - cutout_start_abs[0]});
            auto yview = std::array<int, 2>({block_restricted_cutout.first[1] - cutout_start_abs[1],
                                             block_restricted_cutout.second[1] - cutout_start_abs[1]});
            auto zview = std::array<int, 2>({block_restricted_cutout.first[2] - cutout_start_abs[2],
                                             block_restricted_cutout.second[2] - cutout_start_abs[2]});

            auto output_data_view = output.view(xview, yview, zview);

            // Offset if the cutout starts somewhere in the middle of the block
            int x_block_offset = block_restricted_cutout.first[0] - block_start[0];
            int y_block_offset = block_restricted_cutout.first[1] - block_start[1];
            int z_block_offset = block_restricted_cutout.first[2] - block_start[2];

            blockShPtr->get<T>(output_data_view, x_block_offset, y_block_offset, z_block_offset);
        }
        return;
    }

    std::array<int, 3> getChunkSizeForScale(const std::string& scale_key);
    std::array<int, 3> getVoxelOffsetForScale(const std::string& scale_key);
    std::array<int, 3> getSizeForScale(const std::string& scale_key);
    BlockEncoding getEncodingForScale(const std::string& scale_key);

    static std::array<int, 3> BlockStart(const BlockKey& block_key, const std::array<int, 3>& block_size);
    static std::array<int, 3> BlockEnd(const BlockKey& block_key, const std::array<int, 3>& block_size,
                                       const std::array<int, 3>& image_size);
    static std::array<int, 3> BlockSizeFromExtents(const std::array<int, 3>& block_start,
                                                   const std::array<int, 3>& block_end);
    static std::pair<std::array<int, 3>, std::array<int, 3>> GetDataView(const std::array<int, 3>& block_start,
                                                                         const std::array<int, 3>& block_end,
                                                                         const std::array<int, 3> cutout_start,
                                                                         const std::array<int, 3> cutout_end);

   protected:
    std::vector<BlockKey> _blocksForBoundingBox(const std::array<int, 2>& xrng, const std::array<int, 2>& yrng,
                                                const std::array<int, 2>& zrng, const std::string& scale_key);
    void _init();
    // void _flush(); TODO(adb): automatically flushed on destruction, but maybe
    // we want to implement this eventually

    std::shared_ptr<Manifest> manifest;
    std::shared_ptr<BlockDataStore> _dataStore;
    std::shared_ptr<BlockSettings> _blockSettingsPtr;

    std::unordered_map<std::string, std::shared_ptr<BlockMortonIndexMap>> block_index_by_res;
    BlockDataType _blockDataType;
};

}  // namespace BlockManager_namespace

#endif  // BLOCK_MANAGER_H