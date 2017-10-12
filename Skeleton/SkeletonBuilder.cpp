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

#include "SkeletonBuilder.h"

#include <fstream>
#include <sstream>

#include <folly/dynamic.h>
#include <folly/json.h>

using namespace Skeleton_namespace;

void SkeletonBuilder::FromJson(const std::string& filename) {
    _vertex_map_enabled = true;

    std::ifstream ifs(filename, std::ifstream::binary);

    std::string jsonObjStr(static_cast<std::stringstream const&>(std::stringstream() << ifs.rdbuf()).str());
    folly::dynamic parsed = folly::parseJson(jsonObjStr);
    CHECK(parsed.isObject());

    auto verticesMember = parsed.find("vertices");
    CHECK(verticesMember != parsed.items().end())
        << "Error: \"vertices\" is a required field in the JSON Skeleton specification";

    CHECK(verticesMember->second.isObject()) << "Error: \"vertices\" must be an object.";

    for (const auto& itr : verticesMember->second.items()) {
        auto vertexArr = itr.second;
        CHECK(vertexArr.size() == 3) << "Error: Vertex entries must contain three values";

        std::array<float, 3> vertex;
        for (int i = 0; i < 3; i++) vertex[i] = static_cast<float>(vertexArr[i].asDouble());

        addVertex(vertex, itr.first.asInt());
    }

    auto edgesMember = parsed.find("edges");
    CHECK(edgesMember != parsed.items().end())
        << "Error: \"edges\" is a required field in the JSON Skeleton specification";

    CHECK(edgesMember->second.isArray()) << "Error: \"edges\" must be an array.";

    for (const auto& itr : edgesMember->second) {
        auto edgeArr = itr;
        CHECK(edgeArr.size() == 2) << "Error: Edge entries must contain two values.";

        std::array<uint32_t, 2> edge;
        for (int i = 0; i < 2; i++) edge[i] = static_cast<uint32_t>(edgeArr[i].asInt());

        addEdge(edge);
    }
}

uint32_t SkeletonBuilder::addVertex(const std::array<float, 3>& vertex, uint32_t index) {
    if (_vertex_map_enabled) {
        if (vertex_map.find(index) != vertex_map.end()) {
            return vertex_map[index];
        } else {
            vertex_map.insert(std::make_pair(index, vertices.size()));
        }
    }
    vertices.push_back(vertex);
    return vertices.size() - 1;
}

void SkeletonBuilder::addEdge(const std::array<uint32_t, 2>& edge) {
    if (_vertex_map_enabled) {
        std::array<uint32_t, 2> _edge;
        for (int i = 0; i < 2; i++) {
            const auto edge_itr = vertex_map.find(edge[i]);
            CHECK(edge_itr != vertex_map.end());
            _edge[i] = edge_itr->second;
        }
        edges.push_back(_edge);
    } else {
        edges.push_back(edge);
    }
}