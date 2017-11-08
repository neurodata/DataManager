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

#ifndef BLOCK_DATA_STORE_H
#define BLOCK_DATA_STORE_H

#include "../Blocks/Block.h"
#include "../Blocks/Types.h"
#include "../Manifest.h"

#include <memory>

namespace BlockManager_namespace {

class BlockDataStore {
   public:
    /**
     * Retrieve the manifest file
     */
    virtual ManifestShPtr GetManifest() = 0;

    /**
     * Returns a Block object if one exists in the datastore. Otherwise, return a nullptr.
     */
    virtual BlockShPtr GetBlock(const std::string& block_name, const std::string& scale_key, unsigned int xdim,
                                unsigned int ydim, unsigned int zdim, size_t dtype_size, BlockEncoding encoding,
                                BlockDataType data_type, const std::shared_ptr<BlockSettings>& blockSettings) = 0;

    /**
     * Create a new datastore block. Does not necessarily save the block or create an object in the datastore.
     */
    virtual BlockShPtr CreateBlock(const std::string& block_name, const std::string& scale_key, unsigned int xdim,
                                   unsigned int ydim, unsigned int zdim, size_t dtype_size, BlockEncoding format,
                                   BlockDataType data_type, const std::shared_ptr<BlockSettings>& blockSettings) = 0;

    /**
     * Since we expect most datastores to use the neuroglancer block file format, we provide an implementation of
     * BlockName for neuroglancer precomputed chunk files here. The BlockName method can be overriden for datastores
     * with specialized block keys.
     */
    virtual std::string BlockName(int xstart, int xend, int ystart, int yend, int zstart, int zend,
                                  const std::array<int, 3>& voxel_offset = {{0, 0, 0}}) {
        // neuroglancer files are written into the global coordinate space,
        // whereas block coordinates use the data bounding box coordinate space,
        // so we need to add the voxel_offset to the filename.
        const auto xstart_str = std::to_string(xstart + voxel_offset[0]);
        const auto xend_str = std::to_string(xend + voxel_offset[0]);
        const auto ystart_str = std::to_string(ystart + voxel_offset[1]);
        const auto yend_str = std::to_string(yend + voxel_offset[1]);
        const auto zstart_str = std::to_string(zstart + voxel_offset[2]);
        const auto zend_str = std::to_string(zend + voxel_offset[2]);

        return xstart_str + "-" + xend_str + "_" + ystart_str + "-" + yend_str + "_" + zstart_str + "-" + zend_str;
    }
};

};  // namespace BlockManager_namespace

#endif  // BLOCK_DATA_STORE_H