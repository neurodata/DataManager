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

#ifndef TIFF_ARRAY_H
#define TIFF_ARRAY_H

#include "DataArray.h"

#include <glog/logging.h>
#include <tiffio.h>

namespace DataArray_namespace {

class TiffArray32 : public DataArray3D<uint32_t> {
   public:
    TiffArray32(unsigned int xdim, unsigned int ydim, unsigned int zdim) : DataArray3D<uint32_t>(xdim, ydim, zdim) {}
    ~TiffArray32() {}
    void load(const std::string& filename) final;
    void save(const std::string& filename) final;
};

}  // namespace DataArray_namespace

#endif  // TIFF_ARRAY_H