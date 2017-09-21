#ifndef BLOCK_H
#define BLOCK_H

#include "../DataArray/DataArray.h"

#include <memory>
#include <string>

namespace BlockManager_namespace {

class Block {
   public:
    Block(const std::string& path_name) : path_name(path_name) {}
    Block(const std::string& path_name, int xdim, int ydim, int zdim,
          size_t dtype_size)
        : path_name(path_name),
          _xdim(xdim),
          _ydim(ydim),
          _zdim(zdim),
          _dtype_size(dtype_size) {}
    ~Block() {}

    virtual void load() = 0;
    virtual void save() = 0;

    template <typename T>
    void add(const DataArray_namespace::DataArray3D<T>& arr,
             bool overwrite = false) {
        // TODO(adb): check that this method works
        CHECK(false) << "Not implemented.";
#if 0
        DataArray_namespace::DataArray3D<T> local_arr(data, _xdim, _ydim, _zdim);

        if (overwrite) local_arr.clear();
        local_arr += arr;
#endif
    }

    template <typename T>
    void add(
        const typename DataArray_namespace::DataArray3D<T>::array_view& view,
        int x_arr_offset, int y_arr_offset, int z_arr_offset,
        bool overwrite = false) {
        if (_just_initialized) {
            _allocate();
        }
        DataArray_namespace::DataArray3D<T> local_arr(data, _xdim, _ydim,
                                                      _zdim);

        if (overwrite) local_arr.clear();
        typedef typename DataArray_namespace::DataArray3D<T>::index index;

        // Iterate over the view
        const auto num_dims = view.dimensionality;
        CHECK(num_dims == 3);

        for (size_t x = 0, local_x = x_arr_offset; x < view.shape()[0];
             x++, local_x++) {
            for (size_t y = 0, local_y = y_arr_offset; y < view.shape()[1];
                 y++, local_y++) {
                for (size_t z = 0, local_z = z_arr_offset; z < view.shape()[2];
                     z++, local_z++) {
                    local_arr(local_x, local_y, local_z) +=
                        view[index(x)][index(y)][index(z)];
                }
            }
        }

        local_arr.copy(data, _xdim, _ydim, _zdim);
        _dirty = true;
        _flush();  // flush on write for now
    }

    // Lazily load data from disk only when we need it
    bool is_loaded() const { return _data_loaded; }

    // Determine if we need to flush this block to disk before quitting
    bool is_dirty() const { return _dirty; }

    static std::string SetNeuroglancerFileName(
        int xstart, int xend, int ystart, int yend, int zstart, int zend,
        const std::array<int, 3>& voxel_offset = {{0, 0, 0}}) {
        // neuroglancer files are written into the global coordinate space,
        // whereas block coordinates use the data bounding box coordinate space,
        // so we need to add the voxel_offset to the filename.
        const auto xstart_str = std::to_string(xstart + voxel_offset[0]);
        const auto xend_str = std::to_string(xend + voxel_offset[0]);
        const auto ystart_str = std::to_string(ystart + voxel_offset[1]);
        const auto yend_str = std::to_string(yend + voxel_offset[1]);
        const auto zstart_str = std::to_string(zstart + voxel_offset[2]);
        const auto zend_str = std::to_string(zend + voxel_offset[2]);

        return xstart_str + "-" + xend_str + "_" + ystart_str + "-" + yend_str +
               "_" + zstart_str + "-" + zend_str;
    }

   protected:
    std::string path_name;
    std::unique_ptr<char[]> data;  // C order
    int _xdim;
    int _ydim;
    int _zdim;
    size_t _dtype_size;
    bool _just_initialized = true;
    bool _data_loaded = false;
    bool _dirty = false;

    void _allocate() {
        size_t arr_size = _xdim * _ydim * _zdim * _dtype_size;
        // Allocate and zero an empty data buffer
        data = std::unique_ptr<char[]>(new char[arr_size]);
        std::memset(data.get(), 0, arr_size);
        _data_loaded = true;
        _dirty = true;
        _just_initialized = false;
    }

    void _flush() {
        if (is_dirty()) {
            save();
        }
        data.reset();
        _data_loaded = false;
        _dirty = false;
    }
};

}  // namespace BlockManager_namespace

#endif  // BLOCK_H