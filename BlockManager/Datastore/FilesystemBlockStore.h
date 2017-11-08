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

#ifndef FILESYSTEM_BLOCK_STORE_H
#define FILESYSTEM_BLOCK_STORE_H

#include "BlockDataStore.h"

namespace BlockManager_namespace {

class FilesystemBlockStore : public BlockDataStore {
   public:
    FilesystemBlockStore(const std::string& directory_path_name);

    ManifestShPtr GetManifest();

    BlockShPtr GetBlock(const std::string& block_name, const std::string& scale_key, unsigned int xdim,
                        unsigned int ydim, unsigned int zdim, size_t dtype_size, BlockEncoding encoding,
                        BlockDataType data_type, const std::shared_ptr<BlockSettings>& blockSettings);

    BlockShPtr CreateBlock(const std::string& block_name, const std::string& scale_key, unsigned int xdim,
                           unsigned int ydim, unsigned int zdim, size_t dtype_size, BlockEncoding encoding,
                           BlockDataType data_type, const std::shared_ptr<BlockSettings>& blockSettings);

   protected:
    std::string _directory_path_name;

    std::string _blockPath(const std::string& block_name, const std::string& scale_key);
};

};  // namespace BlockManager_namespace

#endif  // FILESYSTEM_BLOCK_STORE_H