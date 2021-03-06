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

#include "NeuroDataManager.h"

#include "BlockManager/BlockManager.h"
#include "BlockManager/Datastore/FilesystemBlockStore.h"
#include "BlockManager/Manifest.h"
#include "DataArray/TiffArray.h"
#ifdef HAVE_BLOSC
#include "DataArray/BloscArray.h"
#endif

#include <iostream>
#include <memory>
#include <set>

#include <gflags/gflags.h>
#include <glog/logging.h>
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

static bool ValidateInputFileFormat(const char* flagname, const std::string& value) {
    if (value == std::string("tif")) {
        return true;
    }
#ifdef HAVE_BLOSC
    else if (value == std::string("blosc")) {
        return true;
    }
#endif
    return false;
}

static bool DeprecateDataDirFlag(const char* filename, const std::string& value) {
    if (value.size() > 0) {
        std::cerr << "The `-datadir` flag has been deprecated. Use `-datastore` instead.\n";
        return false;
    }
    return true;
}

const std::set<std::string> SUPPORTED_DATATYPES = {"uint8", "uint16", "uint32", "uint64", "float"};
static bool ValidateInputDataType(const char* flagname, const std::string& value) {
    if (SUPPORTED_DATATYPES.find(value) != SUPPORTED_DATATYPES.end()) {
        return true;
    } else {
        return false;
    }
}

DEFINE_string(datadir, "", "Deprecated: Use `-datastore` instead.");
DEFINE_validator(datadir, &DeprecateDataDirFlag);
DEFINE_string(datastore, "",
              "Path to the data directory containing a Neuroglancer JSON "
              "Manifest.");
DEFINE_string(input, "", "Path to the input file (for ingest).");
DEFINE_string(output, "", "Path to the output file (for cutout).");
DEFINE_string(
    format, "tif",
#ifdef HAVE_BLOSC
    "Input/output file format. 'tif' is supported for both input/output. 'blosc' is supported for input only.");
#else
    "Input/output file format. Currently only 'tif' is supported.");
#endif
DEFINE_validator(format, &ValidateInputFileFormat);
DEFINE_string(datatype, "uint32", "Datatype of the input file. Should match the datatype in the Datastore Manifest.");
DEFINE_validator(datatype, &ValidateInputDataType);
DEFINE_int64(x, 0, "The x-dim of the input/output file.");
DEFINE_int64(y, 0, "The y-dim of the input/output file.");
DEFINE_int64(z, 0, "The z-dim of the input/output file.");
DEFINE_int64(xoffset, 0, "The x-offset of the input/output file.");
DEFINE_int64(yoffset, 0, "The y-offset of the input/output file.");
DEFINE_int64(zoffset, 0, "The z-offset of the input/output file.");
DEFINE_string(scale, "0", "String key for scale to use.");
DEFINE_bool(exampleManifest, false, "If true, write an example manifest to 'manifest.ex.json' and quit.");
DEFINE_bool(subtractVoxelOffset, false,
            "If false, provided coordinates do not include the global voxel "
            "offset of the dataset (e.g. are 0-indexed with respect to the "
            "data on disk). If true, the voxel offset is subtracted from the "
            "cutout arguments in a pre-processing step.");
DEFINE_bool(gzip, false, "Compress output using gzip.");

int main(int argc, char* argv[]) {
    google::InstallFailureSignalHandler();

    google::InitGoogleLogging(argv[0]);
    gflags::SetVersionString(
        std::to_string(DataManager_VERSION_MAJOR) + "." + std::to_string(DataManager_VERSION_MINOR) + "." +
        std::to_string(DataManager_VERSION_PATCH) + " build " + std::to_string(DataManager_VERSION_BUILDDATE));
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    if (FLAGS_exampleManifest) {
        const auto exampleManifest = BlockManager_namespace::Manifest::MakeExampleManifest();
        exampleManifest->Write("manifest.ex.json");
        return EXIT_SUCCESS;
    }

    LOG(INFO) << "Using data store " << FLAGS_datastore;
    auto dataStoreShPtr = std::make_shared<BlockManager_namespace::FilesystemBlockStore>(
        BlockManager_namespace::FilesystemBlockStore(FLAGS_datastore));
    BlockManager_namespace::BlockSettings settings({FLAGS_gzip});
    auto manifestShPtr = dataStoreShPtr->GetManifest();

    BlockManager_namespace::BlockManager BLM(manifestShPtr, dataStoreShPtr, settings);

    auto xrng = std::array<int, 2>({{static_cast<int>(FLAGS_xoffset), static_cast<int>(FLAGS_x + FLAGS_xoffset)}});
    auto yrng = std::array<int, 2>({{static_cast<int>(FLAGS_yoffset), static_cast<int>(FLAGS_y + FLAGS_yoffset)}});
    auto zrng = std::array<int, 2>({{static_cast<int>(FLAGS_zoffset), static_cast<int>(FLAGS_z + FLAGS_zoffset)}});

    if (FLAGS_input.size() > 0) {
        // ingest
        if (FLAGS_format == "tif") {
            if (FLAGS_datatype == "uint8") {
                auto im_array = DataArray_namespace::TiffArray<uint8_t>(FLAGS_x, FLAGS_y, FLAGS_z);
                im_array.load(FLAGS_input);
    
                BLM.Put(im_array, xrng, yrng, zrng, FLAGS_scale, FLAGS_subtractVoxelOffset);
            } else if (FLAGS_datatype == "uint32") {
                auto im_array = DataArray_namespace::TiffArray<uint32_t>(FLAGS_x, FLAGS_y, FLAGS_z);
                im_array.load(FLAGS_input);
    
                BLM.Put(im_array, xrng, yrng, zrng, FLAGS_scale, FLAGS_subtractVoxelOffset);
            } else {
                LOG(WARNING) << "Data type " << FLAGS_datatype << " is currently unsupported for tif input files.";
            }
        }
#ifdef HAVE_BLOSC
        else if (FLAGS_format == "blosc") {
            if (FLAGS_datatype == "uint8") {
                auto im_array = DataArray_namespace::BloscArray<uint8_t>(FLAGS_x, FLAGS_y, FLAGS_z);
                im_array.load(FLAGS_input);
    
                BLM.Put(im_array, xrng, yrng, zrng, FLAGS_scale, FLAGS_subtractVoxelOffset);
            } else if (FLAGS_datatype == "uint32") {
                auto im_array = DataArray_namespace::BloscArray<uint32_t>(FLAGS_x, FLAGS_y, FLAGS_z);
                im_array.load(FLAGS_input);
    
                BLM.Put(im_array, xrng, yrng, zrng, FLAGS_scale, FLAGS_subtractVoxelOffset);
            } else {
                LOG(WARNING) << "Data type " << FLAGS_datatype << " is currently unsupported for blosc encoded input files.";
            }
        }
#endif
    } else if (FLAGS_output.size() > 0) {
        // cutout
        if (FLAGS_format == "tif") {
            auto output_arr = DataArray_namespace::TiffArray<uint32_t>(FLAGS_x, FLAGS_y, FLAGS_z);
            output_arr.clear();

            BLM.Get(output_arr, xrng, yrng, zrng, FLAGS_scale, FLAGS_subtractVoxelOffset);

            output_arr.save(FLAGS_output);
        } else {
            LOG(WARNING) << "Unsupported output file format: " << FLAGS_format << "\nQuitting.";
            return EXIT_FAILURE;
        }
    } else {
        LOG(WARNING) << "No input or output file specified. Nothing to do.";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}