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

#ifndef BLOCK_TYPES_H
#define BLOCK_TYPES_H

#include <array>
#include <cstdint>

namespace BlockManager_namespace {

struct BlockKey {
    uint64_t morton_index;
    int x;
    int y;
    int z;
    bool operator<(const BlockKey& other) const { return (morton_index < other.morton_index); }
};

struct BlockInfo {
    BlockKey key;
    std::array<int, 3> block_size;
};

};  // namespace BlockManager_namespace

#endif  // BLOCK_TYPES_H