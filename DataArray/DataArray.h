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

#ifndef DATA_ARRAY_H
#define DATA_ARRAY_H

#include "boost/multi_array.hpp"

#include <array>
#include <cstring>
#include <memory>

namespace DataArray_namespace {

template <class T>
class DataArray {
   public:
    typedef typename boost::multi_array<T, 3> array_type;
    typedef typename array_type::index index;
    typedef typename array_type::index_range range;
    typedef typename array_type::template array_view<3>::type array_view;

    DataArray(unsigned int xdim, unsigned int ydim, unsigned int zdim,
              const boost::general_storage_order<3>& so = boost::c_storage_order()) {
        M = std::shared_ptr<array_type>(new array_type(boost::extents[xdim][ydim][zdim], so));
    }
    DataArray(std::unique_ptr<char[]>& data, unsigned int xdim, unsigned int ydim, unsigned int zdim,
              const boost::general_storage_order<3>& so = boost::c_storage_order())
        : DataArray(xdim, ydim, zdim, so) {
        std::memcpy(M->origin(), data.get(), xdim * ydim * zdim * sizeof(T));
    }
    ~DataArray() { M.reset(); }

    array_view view(const std::array<int, 2>& xrng, const std::array<int, 2>& yrng,
                    const std::array<int, 2>& zrng) const {
        return (*M)[boost::indices[range(xrng[0], xrng[1])][range(yrng[0], yrng[1])][range(zrng[0], zrng[1])]];
    }

    size_t num_elements() const { return M->shape()[0] * M->shape()[1] * M->shape()[2]; }
    size_t num_bytes() const { return num_elements() * sizeof(T); }

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

    T operator[](unsigned int i) const {
        const auto arr_ptr = M->origin();
        return arr_ptr[i];
    }

    T& operator[](unsigned int i) {
        const auto arr_ptr = M->origin();
        return arr_ptr[i];
    }

    void clear() { std::memset(M->origin(), 0, num_bytes()); }

    void copy(std::unique_ptr<char[]>& data, unsigned int xdim, unsigned int ydim, unsigned int zdim) const {
        std::memcpy(data.get(), M->origin(), xdim * ydim * zdim * sizeof(T));
    }

    virtual void load(const std::string& filename) {}
    virtual void save(const std::string& filename) {}

   protected:
    std::shared_ptr<array_type> M;
};

}  // namespace DataArray_namespace

#endif  // DATA_ARRAY_H
