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

#include "Skeleton.h"

#include <fstream>
#include <iterator>

#include <glog/logging.h>
#include <boost/filesystem.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_stream.hpp>

namespace fs = boost::filesystem;
namespace io = boost::iostreams;

using namespace Skeleton_namespace;

/**
 * Neuroglancer File Format:
 * num_vertices: uint32
 * 0: uint32
 * num_edges: uint32
 * 0: uint32
 * <<< vertex list (x: float32, y: float32, z: float32) >>>
 * <<< edge list (a: uint32, b: uint32) >>>
 */
void Skeleton::writeNeuroglancerFile(const std::string& filename, bool gzip) {
    try {
        const auto filepath = fs::path(filename);
        io::filtering_ostream out;
        if (gzip) {
            out.push(io::gzip_compressor(io::gzip_params(io::gzip::default_compression)));
        }
        out.push(io::file_sink(filepath.string()));

        uint32_t zero_padding = 0;
        uint32_t num_vertices = static_cast<uint32_t>(vertices.size());
        out.write(reinterpret_cast<const char*>(&num_vertices), sizeof(uint32_t));
        out.write(reinterpret_cast<const char*>(&zero_padding), sizeof(uint32_t));
        uint32_t num_edges = static_cast<uint32_t>(edges.size());
        out.write(reinterpret_cast<const char*>(&num_edges), sizeof(uint32_t));
        out.write(reinterpret_cast<const char*>(&zero_padding), sizeof(uint32_t));

        for (auto& vertex : vertices) {
            out.write(reinterpret_cast<const char*>(&vertex[0]), 3 * sizeof(float));
        }
        for (const auto& edge : edges) {
            out.write(reinterpret_cast<const char*>(&edge[0]), 2 * sizeof(uint32_t));
        }
    } catch (const fs::filesystem_error& ex) {
        LOG(FATAL) << "Error: Failed to write raw block to disk. " << ex.what();
    }
}
