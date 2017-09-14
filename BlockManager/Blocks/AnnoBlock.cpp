#include "AnnoBlock.h"

#include <fstream>
#include <iterator>

#include <boost/filesystem.hpp>

using namespace DataArray_namespace;
using namespace BlockManager_namespace;
namespace fs = boost::filesystem;

void AnnoBlock32::load() {
    // TODO(adb)
}

void AnnoBlock32::save() {
    switch(_format) {
        case AnnoBlockFormat::RAW: {
            _saveRaw();
        } break;
        default: {
            CHECK(false) << "Error: Unknown anno block format. Aborting save!";
        }
    }
}

void AnnoBlock32::_saveRaw() {
    // Convert data to Fortran order
    DataArray3D<uint32_t> local_arr(data, _xdim, _ydim, _zdim);
   
    DataArray3D<uint32_t> fortran_arr(_xdim, _ydim, _zdim, boost::fortran_storage_order());

    for(int x=0; x<_xdim; x++) {
        for(int y=0; y<_ydim; y++) {
            for(int z=0; z<_zdim; z++) {
                fortran_arr(x,y,z) = local_arr(x,y,z);
            }
        }
    }

    size_t arr_size = _xdim * _ydim * _zdim * sizeof(uint32_t);
    std::unique_ptr<char[]> _tmp_data(new char[arr_size]);
    fortran_arr.copy(_tmp_data, _xdim, _ydim, _zdim);

    try {
        const auto filepath = fs::path(path_name);
        fs::ofstream f(filepath, std::ios_base::binary );
        std::copy( 
            _tmp_data.get(), 
            _tmp_data.get() + (_xdim * _ydim * _zdim * sizeof(uint32_t)),
            std::ostreambuf_iterator<char>(f));
    }   
    catch (const fs::filesystem_error& ex) {
        LOG(FATAL) << "Error: Failed to write raw block to disk. " << ex.what();
    }
}