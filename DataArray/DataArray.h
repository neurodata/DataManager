#ifndef DATA_ARRAY_H
#define DATA_ARRAY_H

#include <glog/logging.h>
#include "boost/multi_array.hpp"

#include <array>
#include <cstring>
#include <memory>

namespace DataArray_namespace {

class DataArray {};

template <typename T>
class DataArray3D : public DataArray {
   public:
    typedef typename boost::multi_array<T, 3> array_type;
    typedef typename array_type::index index;
    typedef typename array_type::index_range range;
    typedef typename array_type::template array_view<3>::type array_view;

    DataArray3D(unsigned int xdim, unsigned int ydim, unsigned int zdim,
                const boost::general_storage_order<3>& so = boost::c_storage_order()) {
        M = std::shared_ptr<array_type>(new array_type(boost::extents[xdim][ydim][zdim], so));
    }
    DataArray3D(std::unique_ptr<char[]>& data, unsigned int xdim, unsigned int ydim, unsigned int zdim,
                const boost::general_storage_order<3>& so = boost::c_storage_order())
        : DataArray3D(xdim, ydim, zdim, so) {
        std::memcpy(M->origin(), data.get(), xdim * ydim * zdim * sizeof(T));
    }
    ~DataArray3D() { M.reset(); }

    array_view view(const std::array<int, 2>& xrng, const std::array<int, 2>& yrng,
                    const std::array<int, 2>& zrng) const {
        return (*M)[boost::indices[range(xrng[0], xrng[1])][range(yrng[0], yrng[1])][range(zrng[0], zrng[1])]];
    }

    size_t size() const { return M->size(); }

    T operator()(unsigned int x, unsigned int y, unsigned int z) const {
        index _x = static_cast<index>(x);
        index _y = static_cast<index>(y);
        index _z = static_cast<index>(z);

        return (*M)[_x][_y][_z];
    }

    T& operator()(unsigned int x, unsigned int y, unsigned int z) {
        index _x = static_cast<index>(x);
        index _y = static_cast<index>(y);
        index _z = static_cast<index>(z);
        return (*M)[_x][_y][_z];
    }

    void clear() { std::memset(M->origin(), 0, size()); }

    void copy(std::unique_ptr<char[]>& data, unsigned int xdim, unsigned int ydim, unsigned int zdim) {
        std::memcpy(data.get(), M->origin(), xdim * ydim * zdim * sizeof(T));
    }

    virtual void load(const std::string& filename) {}

   protected:
    std::shared_ptr<array_type> M;
};

}  // namespace DataArray_namespace

#endif  // DATA_ARRAY_H
