#include "NeuroDataManager.h"

#include "BlockManager/BlockManager.h"
#include "BlockManager/Manifest.h"
#include "DataArray/TiffArray.h"

#include <iostream>
#include <memory>

#include <gflags/gflags.h>
#include <glog/logging.h>
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

static bool ValidateInputFileFormat(const char *filename, const std::string &value) {
    if (value == std::string("tif")) {
        return true;
    }
    return false;
}

DEFINE_string(datadir, "",
              "Path to the data directory containing a Neuroglancer JSON "
              "Manifest.");
DEFINE_string(input, "", "Path to the input file (for ingest).");
DEFINE_string(output, "", "Path to the output file (for cutout).");
DEFINE_string(format, "tif", "Input/output file format. Currently only 'tif' is supported.");
DEFINE_validator(format, &ValidateInputFileFormat);
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

int main(int argc, char *argv[]) {
    google::InstallFailureSignalHandler();

    google::InitGoogleLogging(argv[0]);
    gflags::SetVersionString(std::to_string(DataManager_VERSION_MAJOR) + "." +
                             std::to_string(DataManager_VERSION_MINOR) + " build " +
                             std::to_string(DataManager_VERSION_BUILDDATE));
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    if (FLAGS_exampleManifest) {
        const auto exampleManifest = BlockManager_namespace::Manifest::MakeExampleManifest();
        exampleManifest->Write("manifest.ex.json");
        return EXIT_SUCCESS;
    }

    const auto path = fs::path(FLAGS_datadir);
    CHECK(fs::is_directory(path)) << "Error: data directory '" << FLAGS_datadir << "' does not exist.";

    LOG(INFO) << "Using data directory " << FLAGS_datadir;
    const auto manifest_path = path / fs::path("info");
    CHECK(fs::is_regular_file(manifest_path));

    auto manifestShPtr =
        std::make_shared<BlockManager_namespace::Manifest>(BlockManager_namespace::Manifest(manifest_path.string()));

    BlockManager_namespace::BlockSettings settings({FLAGS_gzip});
    BlockManager_namespace::BlockManager BLM(path.string(), manifestShPtr,
                                             BlockManager_namespace::BlockDataStore::FILESYSTEM, settings);

    auto xrng = std::array<int, 2>({{static_cast<int>(FLAGS_xoffset), static_cast<int>(FLAGS_x + FLAGS_xoffset)}});
    auto yrng = std::array<int, 2>({{static_cast<int>(FLAGS_yoffset), static_cast<int>(FLAGS_y + FLAGS_yoffset)}});
    auto zrng = std::array<int, 2>({{static_cast<int>(FLAGS_zoffset), static_cast<int>(FLAGS_z + FLAGS_zoffset)}});

    if (FLAGS_input.size() > 0) {
        // ingest
        if (FLAGS_format == "tif") {
            auto im_array = DataArray_namespace::TiffArray32(FLAGS_x, FLAGS_y, FLAGS_z);
            im_array.load(FLAGS_input);

            BLM.Put(im_array, xrng, yrng, zrng, FLAGS_scale, FLAGS_subtractVoxelOffset);
        }
    } else if (FLAGS_output.size() > 0) {
        // cutout
        if (FLAGS_format == "tif") {
            auto output_arr = DataArray_namespace::TiffArray32(FLAGS_x, FLAGS_y, FLAGS_z);
            output_arr.clear();

            BLM.Get(output_arr, xrng, yrng, zrng, FLAGS_scale, FLAGS_subtractVoxelOffset);

            output_arr.save(FLAGS_output);
        }
    } else {
        LOG(WARNING) << "No input or output file specified. Nothing to do.";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}