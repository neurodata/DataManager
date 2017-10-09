#include "TiffArray.h"

using namespace DataArray_namespace;

void TiffArray32::load(const std::string& filename) {
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
                (*M)[x][row][page] = static_cast<uint32_t>(data[x * sizeof(uint32_t)]);
            }
        }

        _TIFFfree(data);
        page++;

    } while (TIFFReadDirectory(tif));
    TIFFClose(tif);
}

void TiffArray32::save(const std::string& filename) {
    CHECK(M->storage_order() == boost::c_storage_order());

    TIFF* tif = TIFFOpen(filename.c_str(), "w");
    CHECK(tif);

    CHECK(M->num_dimensions() == 3);
    const auto shape = M->shape();

    auto width = static_cast<unsigned int>(shape[0]);
    auto height = static_cast<unsigned int>(shape[1]);

    auto num_pages = static_cast<unsigned int>(shape[2]);

    unsigned int bits_per_sample = 32;

    auto scan_line_buf = std::vector<uint32_t>(width);

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
                scan_line_buf[x] = (*M)[x][y][page];
            }
            TIFFWriteScanline(tif, &scan_line_buf[0], y, 0);
        }
        TIFFWriteDirectory(tif);
    }
}