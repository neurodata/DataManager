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

        unsigned char* data =
            (unsigned char*)_TIFFmalloc((tsize_t)(TIFFScanlineSize(tif)));

        for (auto row = 0; row < height; row++) {
            TIFFReadScanline(tif, data, row);
            for (int x = 0; x < width; x++) {
                (*M)[x][row][page] =
                    static_cast<uint32_t>(data[x * sizeof(uint32_t)]);
            }
        }

        _TIFFfree(data);
        page++;

    } while (TIFFReadDirectory(tif));
    TIFFClose(tif);
}