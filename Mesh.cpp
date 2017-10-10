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
#include "BlockManager/Manifest.h"
#include "Mesh/MeshExtractor.h"
#include "Mesh/TriangleMesh.h"
#include "Mesh/TriangleMeshIO.h"

#include <memory>

#include <gflags/gflags.h>
#include <glog/logging.h>
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

static bool ValidateMeshFileFormat(const char* filename, const std::string& value) {
    if (value == std::string("neuroglancer") || value == std::string("ply")) {
        return true;
    }
    return false;
}

static bool ValidateMeshExtractor(const char* filename, const std::string& value) {
    if (value == std::string("misha") || value == std::string("neuroglancer")) {
        return true;
    }
    return false;
}

DEFINE_string(datadir, "", "Path to the data directory containing a Neuroglancer JSON manifest.");
DEFINE_int64(x, 0, "The x-dim of the input/output file.");
DEFINE_int64(y, 0, "The y-dim of the input/output file.");
DEFINE_int64(z, 0, "The z-dim of the input/output file.");
DEFINE_int64(xoffset, 0, "The x-offset of the input/output file.");
DEFINE_int64(yoffset, 0, "The y-offset of the input/output file.");
DEFINE_int64(zoffset, 0, "The z-offset of the input/output file.");
DEFINE_string(prefix, "",
              "Output directory and file prefix for mesh files. Files will be written as <FLAGS_prefix>.<id>");
DEFINE_string(suffix, "", "An optional suffix for mesh files. Appended directly after the ID in the filename.");
DEFINE_int64(id, -1,
             "A specific annotation identifier to use for extraction (if less than 0, all meshes in the given region "
             "will be extracted).");
DEFINE_string(scale, "0", "String key for scale to use.");
DEFINE_string(format, "neuroglancer",
              "Output format for the generated mesh files. Supported options: neuroglancer, ply");
DEFINE_validator(format, &ValidateMeshFileFormat);
DEFINE_string(extractor, "misha", "Extraction algorithm to use. Supported options: misha, neuroglancer");
DEFINE_validator(extractor, &ValidateMeshExtractor);
DEFINE_bool(gzip, false, "Set if precomputed chunks in the data directory are compressed using gzip.");
DEFINE_bool(subtractVoxelOffset, true,
            "If false, provided coordinates do not include the global voxel "
            "offset of the dataset (e.g. are 0-indexed with respect to the "
            "data on disk). If true, the voxel offset is subtracted from the "
            "cutout arguments in a pre-processing step.");
DEFINE_bool(64bitHeader, false,
            "Use 64-bit header values to specify the number of indices and the number of vertices in each mesh.");

int main(int argc, char* argv[]) {
    google::InstallFailureSignalHandler();

    google::InitGoogleLogging(argv[0]);
    gflags::SetVersionString(
        std::to_string(DataManager_VERSION_MAJOR) + "." + std::to_string(DataManager_VERSION_MINOR) + "." +
        std::to_string(DataManager_VERSION_PATCH) + " build " + std::to_string(DataManager_VERSION_BUILDDATE));
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    // TODO(adb): Merge into a single file and share with NDM(?)
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

    auto voxel_grid = std::make_shared<DataArray_namespace::DataArray3D<uint32_t>>(FLAGS_x, FLAGS_y, FLAGS_z);
    LOG(INFO) << "Making cutout request";
    BLM.Get(*voxel_grid, xrng, yrng, zrng, FLAGS_scale, FLAGS_subtractVoxelOffset);
    LOG(INFO) << "Retrieved a " << xrng[1] - xrng[0] << " x " << yrng[1] - yrng[0] << " x " << zrng[1] - zrng[0]
              << " cutout.";

    auto propsShPtr = std::make_shared<MeshExtractor_namespace::MarchingCubesProperties>();

    std::vector<std::shared_ptr<MeshExtractor_namespace::TriangleMesh<float>>> meshes;

    if (FLAGS_extractor == std::string("misha")) {
        // TODO(adb): set with command line args (note that some of these may be specific to Misha Marching Cubes)
        propsShPtr->flip = true;

        auto extractor = MeshExtractor_namespace::MishaMeshExtractor(voxel_grid, FLAGS_x, FLAGS_y, FLAGS_z, propsShPtr);
        extractor.setOffsets(FLAGS_xoffset, FLAGS_yoffset, FLAGS_zoffset);
        const auto scale = manifestShPtr->get_scale(FLAGS_scale);
        extractor.setVoxelResolution(scale.resolution[0], scale.resolution[1], scale.resolution[2]);

        LOG(INFO) << "Setup mesh extractor";

        if (FLAGS_id > 0) {
            extractor.extractID(static_cast<uint32_t>(FLAGS_id));
        }

        extractor.extract(meshes);

        LOG(INFO) << "Finished extraction";

    } else if (FLAGS_extractor == std::string("neuroglancer")) {
        auto extractor =
            MeshExtractor_namespace::NeuroglancerMeshExtractor(voxel_grid, FLAGS_x, FLAGS_y, FLAGS_z, propsShPtr);
        extractor.setOffsets(FLAGS_xoffset, FLAGS_yoffset, FLAGS_zoffset);
        const auto scale = manifestShPtr->get_scale(FLAGS_scale);
        extractor.setVoxelResolution(scale.resolution[0], scale.resolution[1], scale.resolution[2]);

        LOG(INFO) << "Setup mesh extractor";

        extractor.extract(meshes);

        LOG(INFO) << "Finished extraction";

    } else {
        LOG(FATAL) << "Error: undefined mesh extraction algorithm: " << FLAGS_extractor;
    }

    if (FLAGS_prefix.length() > 0 || FLAGS_suffix.length() > 0) {
        for (const auto& triangleMeshPtr : meshes) {
            std::string filename = FLAGS_prefix + std::string(".") + std::to_string(triangleMeshPtr->id) + FLAGS_suffix;
            LOG(INFO) << "Writing mesh " << triangleMeshPtr->id << " with " << triangleMeshPtr->vertices.size()
                      << " vertices and " << triangleMeshPtr->triangles.size() << " triangles to file " << filename;
            if (FLAGS_format == "neuroglancer")
                MeshExtractor_namespace::TriangleMeshIO<float>::WriteNeuroglancerBin(filename, *triangleMeshPtr);
            else if (FLAGS_format == "ply")
                MeshExtractor_namespace::TriangleMeshIO<float>::WritePLY(filename, *triangleMeshPtr);
        }
    }

    return EXIT_SUCCESS;
}
