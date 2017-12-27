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

/**
 * Wrapper for compressing and decompressing image data to the JPEG file format. Heavily influenced and inspired by
 * http://www.christian-etter.de/?cat=48.
 */

#include <jerror.h>
#include <jpeglib.h>

#include <memory>
#include <vector>

#ifndef JPEG_H
#define JPEG_H

#define JPEG_MEM_DST_MGR_BUFFER_SIZE (8 * (1 << 10))

namespace JPEG {

class jpeg_compress_struct_wrapper {
   public:
    jpeg_compress_struct_wrapper() {
        this->cinfo.err = jpeg_std_error(&this->jerr);
        jpeg_create_compress(&this->cinfo);
    }

    ~jpeg_compress_struct_wrapper() { jpeg_destroy_compress(&this->cinfo); }

    operator jpeg_compress_struct*() { return &this->cinfo; }

   private:
    jpeg_error_mgr jerr;
    jpeg_compress_struct cinfo;
};

typedef struct _jpeg_destination_mem_mgr {
    jpeg_destination_mgr mgr;
    std::vector<unsigned char> data;
} jpeg_destination_mem_mgr;

static void jpeg_mem_init_destination(j_compress_ptr cinfo) {
    auto dest = reinterpret_cast<jpeg_destination_mem_mgr*>(cinfo->dest);
    dest->data.resize(JPEG_MEM_DST_MGR_BUFFER_SIZE);
    cinfo->dest->next_output_byte = dest->data.data();
    cinfo->dest->free_in_buffer = dest->data.size();
}

static void jpeg_mem_term_destination(j_compress_ptr cinfo) {
    auto dest = reinterpret_cast<jpeg_destination_mem_mgr*>(cinfo->dest);
    dest->data.resize(dest->data.size() - cinfo->dest->free_in_buffer);
}

static boolean jpeg_mem_empty_output_buffer(j_compress_ptr cinfo) {
    jpeg_destination_mem_mgr* dest = reinterpret_cast<jpeg_destination_mem_mgr*>(cinfo->dest);
    size_t old_sz = dest->data.size();
    dest->data.resize(old_sz + JPEG_MEM_DST_MGR_BUFFER_SIZE);
    cinfo->dest->next_output_byte = dest->data.data() + old_sz;
    cinfo->dest->free_in_buffer = JPEG_MEM_DST_MGR_BUFFER_SIZE;
    return (boolean) true;
}

static void jpeg_mem_dest(j_compress_ptr cinfo, jpeg_destination_mem_mgr* dest) {
    cinfo->dest = reinterpret_cast<jpeg_destination_mgr*>(dest);
    cinfo->dest->init_destination = jpeg_mem_init_destination;
    cinfo->dest->term_destination = jpeg_mem_term_destination;
    cinfo->dest->empty_output_buffer = jpeg_mem_empty_output_buffer;
}

std::pair<size_t, std::unique_ptr<char[]>> toJPEG(int width, int height, size_t quality,
                                                  std::unique_ptr<char[]>& input_data) {
    jpeg_destination_mem_mgr dest_mem;
    jpeg_compress_struct_wrapper cinfo;

    j_compress_ptr pcinfo = cinfo;
    jpeg_mem_dest(pcinfo, &dest_mem);

    const int NUM_IMAGE_COMPONENTS = 1;  // Only support grayscale
    // Setup output file parameters
    pcinfo->image_width = width;
    pcinfo->image_height = height;
    pcinfo->input_components = NUM_IMAGE_COMPONENTS;
    pcinfo->in_color_space = JCS_GRAYSCALE;

    jpeg_set_defaults(pcinfo);
    jpeg_set_quality(pcinfo, quality, TRUE /* limit to baseline-JPEG values */);
    jpeg_start_compress(pcinfo, TRUE);

    int row_stride = width * NUM_IMAGE_COMPONENTS;

    auto inputPtr = reinterpret_cast<char*>(input_data.get());
    JSAMPROW row_pointer[1]; /* pointer to JSAMPLE row[s] */
    while (pcinfo->next_scanline < pcinfo->image_height) {
        /* jpeg_write_scanlines expects an array of pointers to scanlines.
         * Here the array is only one element long, but you could pass
         * more than one scanline at a time if that's more convenient.
         */
        row_pointer[0] = reinterpret_cast<JSAMPROW>(&inputPtr[pcinfo->next_scanline * row_stride]);
        jpeg_write_scanlines(pcinfo, row_pointer, /*num_lines=*/1);
    }
    jpeg_finish_compress(pcinfo);

    auto outputBuf = std::unique_ptr<char[]>(new char[dest_mem.data.size()]);
    std::memcpy(outputBuf.get(), &dest_mem.data[0], dest_mem.data.size());

    return std::make_pair(static_cast<size_t>(dest_mem.data.size()), std::move(outputBuf));
}

};  // namespace JPEG

#endif  // JPEG_H