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

#ifndef BLOCK_H
#define BLOCK_H

#include <DataArray/DataArray.h>

#include <glog/logging.h>

#include <memory>
#include <string>

namespace BlockManager_namespace {

// Block settings stores NDM specific block information that is not included in
// the Neuroglancer format. We might want to support saving block settings in a
// dot file in the block storage directory for consistency across ingest /
// cutout operations.
struct BlockSettings {
    bool gzip;
};

struct SerializedBlockOutput {
    std::unique_ptr<char[]> data;
    size_t size;
};

enum class BlockEncoding { RAW = 0, COMPRESSED_SEGMENTATION, JPEG };

enum class BlockDataType { UINT8, UINT16, UINT32, UINT64 };

class Block {
   public:
    Block(const std::string &path_name, int xdim, int ydim, int zdim, size_t dtype_size, BlockEncoding encoding,
          BlockDataType data_type, const std::shared_ptr<BlockSettings> &blockSettingsPtr);
    ~Block();

    template <typename T>
    void add(const typename DataArray_namespace::DataArray<T>::array_view &view, int x_arr_offset, int y_arr_offset,
             int z_arr_offset, bool overwrite = false) {
        if (!_data_loaded) {
            load();
        }
        DataArray_namespace::DataArray<T> local_arr(_data, _xdim, _ydim, _zdim);
        if (overwrite) local_arr.clear();
        typedef typename DataArray_namespace::DataArray<T>::index index;

        // Iterate over the view
        const auto num_dims = view.dimensionality;
        CHECK(num_dims == 3);

        for (size_t x = 0, local_x = x_arr_offset; x < view.shape()[0]; x++, local_x++) {
            for (size_t y = 0, local_y = y_arr_offset; y < view.shape()[1]; y++, local_y++) {
                for (size_t z = 0, local_z = z_arr_offset; z < view.shape()[2]; z++, local_z++) {
                    local_arr(local_x, local_y, local_z) += view[index(x)][index(y)][index(z)];
                }
            }
        }

        local_arr.copy(_data, _xdim, _ydim, _zdim);
        _dirty = true;
        _flush();  // flush on write for now
    }

    template <typename T>
    void get(typename DataArray_namespace::DataArray<T>::array_view &view, int x_arr_offset, int y_arr_offset,
             int z_arr_offset) {
        if (!_data_loaded) {
            load();
        }
        DataArray_namespace::DataArray<T> local_arr(_data, _xdim, _ydim, _zdim);

        typedef typename DataArray_namespace::DataArray<T>::index index;
        const auto num_dims = view.dimensionality;
        CHECK(num_dims == 3);

        for (size_t x = 0, local_x = x_arr_offset; x < view.shape()[0]; x++, local_x++) {
            for (size_t y = 0, local_y = y_arr_offset; y < view.shape()[1]; y++, local_y++) {
                for (size_t z = 0, local_z = z_arr_offset; z < view.shape()[2]; z++, local_z++) {
                    view[index(x)][index(y)][index(z)] += local_arr(local_x, local_y, local_z);
                }
            }
        }
    }

    // Zero block memory and set _data_loaded and _dirty to true
    void zero_block();

    // Lazily load data from disk only when we need it
    bool is_loaded() const { return _data_loaded; }

    // Determine if we need to flush this block to disk before quitting
    bool is_dirty() const { return _dirty; }

    std::array<int, 3> shape() const { return std::array<int, 3>({_xdim, _ydim, _zdim}); }

    static std::string SetNeuroglancerFileName(int xstart, int xend, int ystart, int yend, int zstart, int zend,
                                               const std::array<int, 3> &voxel_offset = {{0, 0, 0}});

   protected:
    std::string _path_name;
    std::unique_ptr<char[]> _data;  // C order
    const std::shared_ptr<BlockSettings> _blockSettingsPtr;
    int _xdim;
    int _ydim;
    int _zdim;
    size_t _dtype_size;
    bool _data_loaded = false;
    bool _dirty = false;
    BlockEncoding _encoding;
    BlockDataType _data_type;

    virtual void load() = 0;
    virtual void save() = 0;

    SerializedBlockOutput _serializeByEncoding();
    void _loadSerializedDataByEncoding(std::unique_ptr<char[]> buf);

    void _allocate();
    void _flush();

    SerializedBlockOutput _toCompressedSegmentation();
    void _fromCompressedSegmentation(std::unique_ptr<char[]> input);

    SerializedBlockOutput _toRaw();
    void _fromRaw(std::unique_ptr<char[]> input);

    SerializedBlockOutput _toJpeg();
    void _fromJpeg(std::unique_ptr<char[]> input);
};

}  // namespace BlockManager_namespace

#endif  // BLOCK_H