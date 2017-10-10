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

#include "FilesystemBlock.h"

#include <fstream>
#include <iterator>

#include <boost/filesystem.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_stream.hpp>

using namespace BlockManager_namespace;
namespace fs = boost::filesystem;
namespace io = boost::iostreams;

void FilesystemBlock::load() {
    try {
        const auto filepath = fs::path(_path_name);
        io::filtering_istream in;
        if (_blockSettingsPtr->gzip) {
            in.push(io::gzip_decompressor());
        }
        in.push(io::file_source(filepath.string(), std::ios::in | std::ios::binary));

        std::vector<char> buf;
        io::copy(in, io::back_inserter(buf));
        LOG(INFO) << "Read in " << buf.size() << " bytes";
        auto input_buf = std::unique_ptr<char[]>(new char[buf.size()]);
        std::memcpy(input_buf.get(), &buf[0], buf.size());

        _loadSerializedDataByEncoding(std::move(input_buf));

    } catch (const fs::filesystem_error &ex) {
        LOG(FATAL) << "Error: Failed to write raw block to disk. " << ex.what();
    }
}

void FilesystemBlock::save() {
    auto serialized_data = _serializeByEncoding();

    try {
        const auto filepath = fs::path(_path_name);
        io::filtering_ostream out;
        if (_blockSettingsPtr->gzip) {
            out.push(io::gzip_compressor(io::gzip_params(io::gzip::default_compression)));
        }
        out.push(io::file_sink(filepath.string()));
        out.write(serialized_data.data.get(), serialized_data.size);
    } catch (const fs::filesystem_error &ex) {
        LOG(FATAL) << "Error: Failed to write raw block to disk. " << ex.what();
    }
}