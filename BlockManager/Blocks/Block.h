#ifndef BLOCK_H
#define BLOCK_H

#include "../DataArray/DataArray.h"

#include <memory>
#include <string> 

namespace BlockManager_namespace {

class Block {
public:
    Block(const std::string& path_name): path_name(path_name) {}
    Block(const std::string& path_name, int xdim, int ydim, int zdim, size_t dtype_size): path_name(path_name), _xdim(xdim), _ydim(ydim), _zdim(zdim) {
        size_t arr_size = _xdim * _ydim * _zdim * dtype_size;
        // Allocate and zero an empty data buffer
        data = std::unique_ptr<char[]>(new char[arr_size]);
        std::memset(data.get(), 0, arr_size);
        _data_loaded = true;
    }
    ~Block() {}

    virtual void load() = 0;
    virtual void save() = 0;

    template< typename T >
    void add(const DataArray_namespace::DataArray3D<T>& arr, bool overwrite=false) {
        // TODO(adb): check that this method works
        DataArray_namespace::DataArray3D<T> local_arr(data, _xdim, _ydim, _zdim);

        if (overwrite) local_arr.clear();
        local_arr += arr;
    }
    
    template< typename T >
    void add(const typename DataArray_namespace::DataArray3D<T>::array_view& view, bool overwrite=false) {
        DataArray_namespace::DataArray3D<T> local_arr(data, _xdim, _ydim, _zdim);

        if (overwrite) local_arr.clear();
        typedef typename DataArray_namespace::DataArray3D<T>::index index;
        for(int x = 0; x < _xdim; x++) {
            for(int y = 0; y < _ydim; y++) {
                for(int z = 0; z < _zdim; z++) {
                    local_arr(x,y,z) += view[index(x)][index(y)][index(z)];
                }
            }
        }
        local_arr.copy(data, _xdim, _ydim, _zdim);
        _dirty = true;
    }

    // Lazily load data from disk only when we need it
    bool is_loaded() const { return _data_loaded; }

    // Determine if we need to flush this block to disk before quitting
    bool is_dirty() const { return _dirty; }

    static std::string SetNeuroglancerFileName(int xstart, int xend, int ystart, int yend, int zstart, int zend) {

        const auto xstart_str = std::to_string(xstart);
        const auto xend_str = std::to_string(xend);
        const auto ystart_str = std::to_string(ystart);
        const auto yend_str = std::to_string(yend);        
        const auto zstart_str = std::to_string(zstart);
        const auto zend_str = std::to_string(zend);
        
        return xstart_str + "-" + xend_str + "_" + ystart_str + "-" + yend_str + "_" + zstart_str + "-" + zend_str;
    }

protected:
    std::string path_name;
    std::unique_ptr<char[]> data; // C order
    int _xdim;
    int _ydim;
    int _zdim;
    bool _data_loaded = false;
    bool _dirty = false;

    void _flush() {
        if (is_dirty()) {
            save();
        }
        data.reset(); 
    }
    
};

}

#endif // BLOCK_H