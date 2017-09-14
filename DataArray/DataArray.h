#ifndef DATA_ARRAY_H
#define DATA_ARRAY_H

#include "boost/multi_array.hpp"
#include <glog/logging.h>

#include <array>
#include <memory>

namespace DataArray_namespace {

class DataArray {};

template< typename T >
class DataArray3D: public DataArray {
public:
    typedef typename boost::multi_array<T, 3> array_type;
    typedef typename array_type::index index;
    typedef typename array_type::index_range range;
    typedef typename array_type::template array_view<3>::type array_view;

    DataArray3D(unsigned int xdim, unsigned int ydim, unsigned int zdim, const boost::general_storage_order<3>& so = boost::c_storage_order()) {
        M = std::shared_ptr<array_type>(new array_type(boost::extents[xdim][ydim][zdim], so));
    }
    DataArray3D(std::unique_ptr<char[]>& data, unsigned int xdim, unsigned int ydim, unsigned int zdim): DataArray3D(xdim, ydim, zdim) {
        std::memcpy(M->origin(), data.get(), xdim*ydim*zdim*sizeof(T));
    }
    ~DataArray3D() {
        M.reset();
    }

    array_view view(const std::array<int, 2>& xrng, const std::array<int, 2>& yrng, const std::array<int, 2>& zrng) const {
        return (*M)[ boost::indices[range(xrng[0],xrng[1])][range(yrng[0],yrng[1])][range(zrng[0],zrng[1])] ];
    }

    size_t size() const { return M->size(); }

    T operator()(unsigned int x, unsigned int y, unsigned int z) const {
        index _x = static_cast<index>(x);
        index _y = static_cast<index>(y);
        index _z = static_cast<index>(z);

        return (*M)[_x][_y][_z];
    }
    
    T &operator()(unsigned int x, unsigned int y, unsigned int z) {
        index _x = static_cast<index>(x);
        index _y = static_cast<index>(y);
        index _z = static_cast<index>(z);
        return (*M)[_x][_y][_z];
    }

#if 0
    // TODO(adb): verify that this works
    T operator[](int i) {
        return (*M)[static_cast<index>(i)];
    }
#endif

#if 0
    DataArray3D<T>& operator+=(const DataArray3D<T>& right) {
        for(int i=0; i<M->size(); i++) {
            (*M)[i] += right[i];
        }
        return *this;
    }
    DataArray3D<T>& operator+=(const DataArray3D<T>::array_view& right) {
        auto local_itr = M->begin();
        for(auto right_itr = right.begin(); right_itr != right.end() || local_itr != M->end(); right_itr++, local_itr++) {
            local_itr = local_itr + right_itr;
        }
        return *this;
    }
    DataArray3D<T>& operator=(const DataArray3D<T>& other) {
        // copy constructor
        M = std::make_shared<array_type>(*other.M);
        return *this;
    }
    DataArray3D<T>& operator+=(const DataArray3D<T>& right) {
        for(int i=0; i<M->size(); i++) {
            (*M)[i] += right[i];
        }
        return *this;
    }
    DataArray3D<T>& operator+=(const DataArray3D<T>::array_view& right) {
        for(auto index = 0; index < right.size(); index++) {
            (*M)[index] += right[index];
        }
        return *this;
    }
#endif 

    void clear() {
        std::memset(M->origin(), 0, size());    
    }

    void copy(std::unique_ptr<char[]>& data, unsigned int xdim, unsigned int ydim, unsigned int zdim) {
        std::memcpy(data.get(), M->origin(), xdim*ydim*zdim*sizeof(T));
    }

    virtual void load(const std::string& filename) {}
    
protected:   
    std::shared_ptr<array_type> M;
};

}

#endif // DATA_ARRAY_H

