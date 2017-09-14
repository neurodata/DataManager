#ifndef TIFF_ARRAY_H
#define TIFF_ARRAY_H

#include "DataArray.h"

#include <tiffio.h>
#include <glog/logging.h>

namespace DataArray_namespace {

class TiffArray32 : public DataArray3D<uint32_t> {
public:
    TiffArray32(unsigned int xdim, unsigned int ydim, unsigned int zdim): DataArray3D<uint32_t>(xdim, ydim, zdim) {}
    ~TiffArray32() {}
    void load(const std::string& filename) final;
};

}

#endif // TIFF_ARRAY_H 