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

#include "FilesystemBlockStore.h"

#include "../Blocks/FilesystemBlock.h"

#include <glog/logging.h>
#include <boost/filesystem.hpp>

using namespace BlockManager_namespace;
namespace fs = boost::filesystem;

FilesystemBlockStore::FilesystemBlockStore(const std::string& directory_path_name)
    : _directory_path_name(directory_path_name) {
    CHECK(_directory_path_name.size() > 0 && fs::is_directory(fs::path(_directory_path_name)))
        << "Error: Directory path for filesystem datastore does not exist: " << _directory_path_name;
}

ManifestShPtr FilesystemBlockStore::GetManifest() {
    const auto manifest_path = fs::path(_directory_path_name) / fs::path("info");
    CHECK(fs::is_regular_file(manifest_path)) << "Error: Failed to find Manifest file at: " << manifest_path.string();

    std::ifstream ifs(manifest_path.string(), std::ifstream::binary);
    auto manifestShPtr = std::make_shared<Manifest>();
    manifestShPtr->Read(ifs);
    return manifestShPtr;
}

BlockShPtr FilesystemBlockStore::GetBlock(const std::string& block_name, const std::string& scale_key,
                                          unsigned int xdim, unsigned int ydim, unsigned int zdim, size_t dtype_size,
                                          BlockEncoding encoding, BlockDataType data_type,
                                          const std::shared_ptr<BlockSettings>& blockSettings) {
    auto block_path = _blockPath(block_name, scale_key);
    if (fs::is_regular_file(block_path)) {
        return std::make_shared<FilesystemBlock>(block_path, xdim, ydim, zdim, dtype_size, encoding, data_type,
                                                 blockSettings);
    } else {
        return nullptr;
    }
}

BlockShPtr FilesystemBlockStore::CreateBlock(const std::string& block_name, const std::string& scale_key,
                                             unsigned int xdim, unsigned int ydim, unsigned int zdim, size_t dtype_size,
                                             BlockEncoding encoding, BlockDataType data_type,
                                             const std::shared_ptr<BlockSettings>& blockSettings) {
    // Check and see if the block exists. If yes, return it. Otherwise create a new one.
    auto blockShPtr = GetBlock(block_name, scale_key, xdim, ydim, zdim, dtype_size, encoding, data_type, blockSettings);
    if (blockShPtr) {
        return blockShPtr;
    } else {
        auto block_path = _blockPath(block_name, scale_key);
        auto blockShPtr = std::make_shared<FilesystemBlock>(block_path, xdim, ydim, zdim, dtype_size, encoding,
                                                            data_type, blockSettings);
        // Zeroing the block tells us this is a new block with no underlying data in the datastore
        blockShPtr->zero_block();
        return blockShPtr;
    }
}

std::string FilesystemBlockStore::_blockPath(const std::string& block_name, const std::string& scale_key) {
    const auto scale_directory = fs::path(_directory_path_name) / fs::path(scale_key);
    CHECK(fs::is_directory(scale_directory))
        << "Error: No directory for scale " << scale_key << ". Expected: " << scale_directory.string();

    const auto block_path = scale_directory / fs::path(block_name);
    return block_path.string();
}