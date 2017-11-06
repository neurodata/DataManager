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

#include "TiffArray.h"

#include <glog/logging.h>
#include <tiffio.h>

using namespace DataArray_namespace;

template <class T>
void TiffArray<T>::load(const std::string& filename) {
    TIFF* tif = TIFFOpen(filename.c_str(), "r");
    CHECK(tif);
    int page = 0;

    do {
        unsigned int width, height;
        TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width);
        TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height);

        unsigned char* data = (unsigned char*)_TIFFmalloc((tsize_t)(TIFFScanlineSize(tif)));

        for (size_t row = 0; row < height; row++) {
            TIFFReadScanline(tif, data, row);
            for (size_t x = 0; x < width; x++) {
                (*this->M)[x][row][page] = static_cast<T>(data[x * sizeof(T)]);
            }
        }

        _TIFFfree(data);
        page++;

    } while (TIFFReadDirectory(tif));
    TIFFClose(tif);
}

template <class T>
void TiffArray<T>::save(const std::string& filename) {
    CHECK(this->M->storage_order() == boost::c_storage_order());

    TIFF* tif = TIFFOpen(filename.c_str(), "w");
    CHECK(tif);

    CHECK(this->M->num_dimensions() == 3);
    const auto shape = this->M->shape();

    auto width = static_cast<unsigned int>(shape[0]);
    auto height = static_cast<unsigned int>(shape[1]);

    auto num_pages = static_cast<unsigned int>(shape[2]);

    unsigned int bits_per_sample = sizeof(T) * 8;

    auto scan_line_buf = std::vector<T>(width);

    for (unsigned int page = 0; page < num_pages; page++) {
        TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, width);
        TIFFSetField(tif, TIFFTAG_IMAGELENGTH, height);
        TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, bits_per_sample);
        TIFFSetField(tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
        TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, width);
        TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
        TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_DEFLATE);

        for (unsigned int y = 0; y < height; y++) {
            for (size_t x = 0; x < width; x++) {
                scan_line_buf[x] = (*this->M)[x][y][page];
            }
            TIFFWriteScanline(tif, &scan_line_buf[0], y, 0);
        }
        TIFFWriteDirectory(tif);
    }
}

#define DO_INSTANTIATE(T)        \
    template class TiffArray<T>; \
    /**/

DO_INSTANTIATE(uint8_t)
DO_INSTANTIATE(uint16_t)
DO_INSTANTIATE(uint32_t)
DO_INSTANTIATE(uint64_t)
DO_INSTANTIATE(float)

#undef DO_INSTANTIATE
