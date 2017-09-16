#include "NeuroDataManager.h"

#include "BlockManager/BlockManager.h"
#include "BlockManager/Manifest.h"
#include "DataArray/TiffArray.h"

#include <memory>

#include <glog/logging.h>
#include <gflags/gflags.h>
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

DEFINE_string(ingestdir, "", "Path to the ingest directory containing a Neuroglancer JSON Manifest for this ingest job.");
DEFINE_string(input, "", "Path to the input file to ingest.");
DEFINE_string(format, "tif", "Input file format. Currently only 'tif' is supported. Defaults to 'tif'.");
DEFINE_string(mode, "image", "Ingest mode. Currently only image or annotation ingest is supported.");
DEFINE_int64(x, 0, "The x-dim of the input file.");
DEFINE_int64(y, 0, "The y-dim of the input file.");
DEFINE_int64(z, 0, "The z-dim of the input file.");
DEFINE_int64(xoffset, 0, "The x-offset of the input file.");
DEFINE_int64(yoffset, 0, "The y-offset of the input file.");
DEFINE_int64(zoffset, 0, "The z-offset of the input file.");
DEFINE_string(scale, "0", "String key for scale to use.");
DEFINE_bool(exampleManifest, false, "If true, write an example manifest to 'manifest.ex.json' and quit.");
DEFINE_bool(subtractVoxelOffset, false, "If false, provided coordinates do not include the global voxel offset of the dataset (e.g. are 0-indexed with respect to the data on disk). If true, the voxel offset is subtracted from the cutout arguments in a pre-processing step.");

int main(int argc, char* argv[]) {
    google::InstallFailureSignalHandler();

    google::InitGoogleLogging(argv[0]);
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    if (FLAGS_exampleManifest) {
        const auto exampleManifest = BlockManager_namespace::Manifest::MakeExampleManifest();
        exampleManifest->Write("manifest.ex.json");
        LOG(INFO) << "I have " << exampleManifest->num_scales() << " scales.";
        return EXIT_SUCCESS;
    }

    const auto path = fs::path(FLAGS_ingestdir);
    CHECK(fs::is_directory(path)) << "Error: Ingest directory '" << FLAGS_ingestdir << "' does not exist.";
    
    LOG(INFO) << "Using ingest directory " << FLAGS_ingestdir;
    const auto manifest_path = path / fs::path("info");
    CHECK(fs::is_regular_file(manifest_path));
    
    auto manifestShPtr = std::make_shared<BlockManager_namespace::Manifest>(BlockManager_namespace::Manifest(manifest_path.string()));

    BlockManager_namespace::BlockManager BLM(path.string(), manifestShPtr);

    // Open the file to ingest
    if (FLAGS_format == "tif") {
        auto im_array = DataArray_namespace::TiffArray32(FLAGS_x, FLAGS_y, FLAGS_z);
        im_array.load(FLAGS_input);

        auto xrng = std::array<int, 2>({{static_cast<int>(FLAGS_xoffset), static_cast<int>(FLAGS_x + FLAGS_xoffset)}});
        auto yrng = std::array<int, 2>({{static_cast<int>(FLAGS_yoffset), static_cast<int>(FLAGS_y + FLAGS_yoffset)}});
        auto zrng = std::array<int, 2>({{static_cast<int>(FLAGS_zoffset), static_cast<int>(FLAGS_z + FLAGS_zoffset)}});
        
        BLM.Put(im_array, xrng, yrng, zrng, FLAGS_scale, FLAGS_subtractVoxelOffset);   
    }

    return EXIT_SUCCESS;
}