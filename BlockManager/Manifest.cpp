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

#include "Manifest.h"

#include <sstream>

#include <glog/logging.h>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

using namespace BlockManager_namespace;
namespace fs = boost::filesystem;

Scale::Scale(const folly::dynamic& obj) {
    // This constructor assumes that obj has already been validated to be an
    // array
    auto keyMember = obj.find("key");
    CHECK(keyMember != obj.items().end() && keyMember->second.isString())
        << "Error: Scale property 'key' is required and it must be a string.";
    key = keyMember->second.asString();

    auto sizeMember = obj.find("size");
    CHECK(sizeMember != obj.items().end() && sizeMember->second.isArray() && sizeMember->second.size() == 3)
        << "Error: Scale property 'size' is required and it must be an array "
           "with 3 elements.";
    for (int i = 0; i < 3; i++) {
        size[i] = sizeMember->second[i].asInt();
    }

    auto voxelOffsetMember = obj.find("voxel_offset");
    CHECK(voxelOffsetMember != obj.items().end() && voxelOffsetMember->second.isArray() &&
          voxelOffsetMember->second.size() == 3)
        << "Error: Scale property 'voxel_offset' is required and it must be an "
           "array with 3 elements.";
    for (int i = 0; i < 3; i++) {
        voxel_offset[i] = voxelOffsetMember->second[i].asInt();
    }

    auto resolutionMember = obj.find("resolution");
    CHECK(resolutionMember != obj.items().end() && resolutionMember->second.isArray() &&
          resolutionMember->second.size() == 3)
        << "Error: Scale property 'resolution' is required and it must be an "
           "array with 3 elements.";
    for (int i = 0; i < 3; i++) {
        resolution[i] = resolutionMember->second[i].asDouble();
    }

    auto chunkSizesMember = obj.find("chunk_sizes");
    CHECK(chunkSizesMember != obj.items().end() && chunkSizesMember->second.isArray())
        << "Error: Scale property 'chunk_sizes' is required and it must be an "
           "array.";
    for (const auto& chunkSizeObj : chunkSizesMember->second) {
        CHECK(chunkSizeObj.isArray() && chunkSizeObj.size() == 3)
            << "Error: All 'chunk_size' elements must be arrays of size 3.";
        chunk_sizes.push_back(
            std::array<int, 3>({{static_cast<int>(chunkSizeObj[0].asInt()), static_cast<int>(chunkSizeObj[1].asInt()),
                                 static_cast<int>(chunkSizeObj[2].asInt())}}));
    }

    auto encodingMember = obj.find("encoding");
    // TODO(adb): validate encoding string against list of supported encodings
    CHECK(encodingMember != obj.items().end() && encodingMember->second.isString())
        << "Error: Scale property 'encoding' is required and it must be a "
           "string.";
    encoding = encodingMember->second.asString();

    auto compressedSegmentationBlockSizeMember = obj.find("compressed_segmentation_block_size");
    if (compressedSegmentationBlockSizeMember != obj.items().end()) {
        CHECK(compressedSegmentationBlockSizeMember->second.isArray())
            << "Error: Scale property compressed_segmentation_block_size must "
               "be an array.";
        for (int i = 0; i < 3; i++) {
            compressed_segmentation_block_size[i] = compressedSegmentationBlockSizeMember->second[i].asInt();
        }
    }
}

folly::dynamic Scale::toDynamicObj() const {
    // Have to convert all the arrays first
    folly::dynamic sizeObj = folly::dynamic::array(size[0], size[1], size[2]);

    folly::dynamic voxelOffsetObj = folly::dynamic::array(voxel_offset[0], voxel_offset[1], voxel_offset[2]);

    folly::dynamic resolutionObj = folly::dynamic::array(resolution[0], resolution[1], resolution[2]);

    folly::dynamic chunkSizesObj = folly::dynamic::array();
    for (const auto& chunk : chunk_sizes) {
        chunkSizesObj.push_back(folly::dynamic::array(chunk[0], chunk[1], chunk[2]));
    }

    folly::dynamic compressedSegmentationBlockSizeObj =
        folly::dynamic::array(compressed_segmentation_block_size[0], compressed_segmentation_block_size[1],
                              compressed_segmentation_block_size[2]);

    folly::dynamic jsonObj = folly::dynamic::object("key", key)("size", sizeObj)("voxel_offset", voxelOffsetObj)(
        "resolution", resolutionObj)("chunk_sizes", chunkSizesObj)("encoding", encoding)(
        "compressed_segmentation_block_size", compressedSegmentationBlockSizeObj);

    return jsonObj;
}

void Manifest::Write(const std::string& filename) {
    folly::dynamic scaleArrObj = folly::dynamic::array();
    for (const auto& scaleObj : _scales) {
        scaleArrObj.push_back(scaleObj.toDynamicObj());
    }

    folly::dynamic jsonObj = folly::dynamic::object("type", _type)("data_type", _data_type)(
        "num_channels", _num_channels)("scales", scaleArrObj);

    if (_mesh.size() > 0) {
        jsonObj["mesh"] = _mesh;
    }

    try {
        const auto filepath = fs::path(filename);
        fs::ofstream f(filepath);
        f << folly::toJson(jsonObj);
    } catch (const fs::filesystem_error& ex) {
        LOG(FATAL) << "Error: Failed to write Neuroglancer manifest to disk. " << ex.what();
    }
}

void Manifest::ReadLocal(const std::string& filepath) {
    CHECK(fs::is_regular_file(filepath));

    std::ifstream ifs(filepath, std::ifstream::binary);
    Read(ifs);
}

void Manifest::Read(const std::ifstream& ifs) {
    std::string jsonObjStr(static_cast<std::stringstream const&>(std::stringstream() << ifs.rdbuf()).str());

    folly::dynamic parsed = folly::parseJson(jsonObjStr);
    CHECK(parsed.isObject());

    auto typeMember = parsed.find("type");
    CHECK(typeMember != parsed.items().end()) << "Error: Type is a required field in the Neuroglancer Manifest.";
    set_type(typeMember->second.asString());

    auto dataTypeMember = parsed.find("data_type");
    CHECK(dataTypeMember != parsed.items().end())
        << "Error: Data Type is a required field in the Neuroglancer Manifest.";
    set_data_type(dataTypeMember->second.asString());

    auto numChannelsMember = parsed.find("num_channels");
    CHECK(numChannelsMember != parsed.items().end()) << "Error: Num Channels is a required field in the Neuroglancer "
                                                        "Manifest.";
    set_num_channels(numChannelsMember->second.asInt());

    auto scalesMember = parsed.find("scales");
    CHECK(scalesMember != parsed.items().end() && scalesMember->second.isArray())
        << "Error: Scales is a required field in the Neuroglancer Manifest and "
           "it must be an array.";
    for (auto& scaleObj : scalesMember->second) {
        _scales.push_back(Scale(scaleObj));
    }

    auto meshMember = parsed.find("mesh");
    if (meshMember != parsed.items().end() && meshMember->second.isString()) {
        set_mesh(meshMember->second.asString());
    }
}

void Manifest::Print() const { LOG(INFO) << "TODO! :-)"; }

std::shared_ptr<Manifest> Manifest::MakeExampleManifest() {
    auto ManifestShPtr = std::make_shared<Manifest>();
    ManifestShPtr->_scales.push_back(Scale());
    return ManifestShPtr;
}