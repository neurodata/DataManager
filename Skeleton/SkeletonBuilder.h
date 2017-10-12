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

#ifndef SKELETON_BUILDER_H
#define SKELETON_BUILDER_H

#include "Skeleton.h"

#include <unordered_map>

namespace Skeleton_namespace {

class SkeletonBuilder : public Skeleton {
   public:
    SkeletonBuilder() {}
    ~SkeletonBuilder() {}

    void FromJson(const std::string& filename);

    uint32_t addVertex(const std::array<float, 3>& vertex, uint32_t index = 0);
    void addEdge(const std::array<uint32_t, 2>& edge);

   protected:
    std::unordered_map<uint32_t, uint32_t> vertex_map;  // prior index --> new index
    bool _vertex_map_enabled = false;
};
}

#endif