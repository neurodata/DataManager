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

#include "BloscArray.h"

#include <glog/logging.h>

#include <fstream>
#include <memory>

using namespace DataArray_namespace;

template <class T>
void BloscArray<T>::load(const std::string& filename) {
    // Initialize the blosc environment
    blosc_init();

    std::ifstream ifile;

    ifile.open(filename, std::ios::in | std::ios::binary);
    std::vector<char> compressed((std::istreambuf_iterator<char>(ifile)), (std::istreambuf_iterator<char>()));
    ifile.close();

    const auto shape = this->M->shape();
    // TODO(adb): We could copy directly into the boost multidimensional array.
    // However, blocks from The Boss are coming in uint64_t format.
    // For now, we convert from uint64_t to uint32_t.
    size_t num_elements = shape[0] * shape[1] * shape[2];
    size_t uncompressed_size = num_elements * sizeof(uint64_t);

    auto uncompressed = std::vector<uint64_t>(num_elements);

    int dsize = static_cast<int>(uncompressed_size);
    dsize = blosc_decompress(&compressed[0], &uncompressed[0], dsize);
    CHECK_GE(dsize, 0) << "Decompression error. Error code: " << dsize;
    CHECK_EQ(dsize, static_cast<int>(uncompressed_size));

    // Destroy blosc environment
    blosc_destroy();

    // TODO(adb): remove this once we can copy directly into the multidimensional array objects memory
    for (size_t x = 0; x < shape[0]; x++) {
        for (size_t y = 0; y < shape[1]; y++) {
            for (size_t z = 0; z < shape[2]; z++) {
                size_t i = x + y * shape[0] + z * shape[0] * shape[1];
                (*this->M)[static_cast<typename DataArray<T>::index>(x)][static_cast<typename DataArray<T>::index>(y)]
                          [static_cast<typename DataArray<T>::index>(z)] = static_cast<uint32_t>(uncompressed[i]);
            }
        }
    }
}

template <class T>
void BloscArray<T>::save(const std::string& filename) {
    LOG(FATAL) << "Error: Saving DataArray in Blosc format is not supported.";
}

namespace DataArray_namespace {

#define DO_INSTANTIATE(T)         \
    template class BloscArray<T>; \
    /**/

DO_INSTANTIATE(uint8_t)
DO_INSTANTIATE(uint16_t)
DO_INSTANTIATE(uint32_t)
DO_INSTANTIATE(uint64_t)
DO_INSTANTIATE(float)

#undef DO_INSTANTIATE

}  // namespace DataArray_namespace