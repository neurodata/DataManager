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