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

#include "ExtractNeuroglancerMesh.h"
#include "MeshExtractor.h"
#include "TriangleMeshIO.h"

#include <stdio.h>
#include <memory>

using namespace DataArray_namespace;
using namespace MeshExtractor_namespace;

void ConvertArray64to32Bit(void* input, void* output, int length) {
    auto inputPtr = reinterpret_cast<uint64_t*>(input);
    auto outputPtr = reinterpret_cast<uint32_t*>(output);
    for (int i = 0; i < length; i++) {
        outputPtr[i] = static_cast<uint32_t>(inputPtr[i]);
    }
}

void ExtractNeuroglancerMeshFromChunk(void* input, char* file_path_prefix, int xdim, int ydim, int zdim, int xoffset,
                                      int yoffset, int zoffset, float xres, float yres, float zres, int chunkID,
                                      int* num_meshes) {
    auto propsShPtr = std::make_shared<MarchingCubesProperties>();

    uint32_t* _input_ptr = reinterpret_cast<uint32_t*>(input);
    uint32_t* _data_ptr = new uint32_t[xdim * ydim * zdim];
    for (int x = 0; x < xdim; x++) {
        for (int y = 0; y < ydim; y++) {
            for (int z = 0; z < zdim; z++) {
                _data_ptr[z + (zdim * y) + (zdim * ydim * x)] = _input_ptr[(z * xdim * ydim) + (y * xdim) + x];
            }
        }
    }

    auto dataArrayShPtr = std::make_shared<DataArray<uint32_t>>(
        static_cast<unsigned int>(xdim), static_cast<unsigned int>(ydim), static_cast<unsigned int>(zdim));

    dataArrayShPtr->set(_data_ptr, static_cast<unsigned int>(xdim), static_cast<unsigned int>(ydim),
                        static_cast<unsigned int>(zdim));
    delete[] _data_ptr;

    auto extractor =
        NeuroglancerMeshExtractor(dataArrayShPtr, static_cast<unsigned int>(xdim), static_cast<unsigned int>(ydim),
                                  static_cast<unsigned int>(zdim), propsShPtr);

    extractor.setOffsets(xoffset, yoffset, zoffset);
    extractor.setVoxelResolution(xres, yres, zres);

    std::vector<std::shared_ptr<MeshExtractor_namespace::TriangleMesh<float>>> meshes;
    extractor.extract(meshes);

    *num_meshes = meshes.size();
    for (const auto& triangleMeshPtr : meshes) {
        std::string filename = std::string(file_path_prefix) + std::string(".") + std::to_string(triangleMeshPtr->id);
        TriangleMeshIO<float>::WriteNeuroglancerBin(filename, *triangleMeshPtr);
        meshExtractedCallbackGo(const_cast<char*>(filename.c_str()), triangleMeshPtr->id, chunkID);
    }
}
