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

#ifndef SKELETON_H
#define SKELETON_H

#include <array>
#include <vector>

namespace Skeleton_namespace {

class Skeleton {
   public:
    Skeleton() {}
    ~Skeleton() {}

    void writeNeuroglancerFile(const std::string& filename, bool gzip=false);

   protected:
    uint32_t id;
    std::vector<std::array<float, 3> > vertices;
    std::vector<std::array<uint32_t, 2> > edges;
};
}

#endif