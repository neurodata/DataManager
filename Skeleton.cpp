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

#include "Skeleton/SkeletonBuilder.h"

#include <gflags/gflags.h>
#include <glog/logging.h>
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

static bool ValidateInputFileFormat(const char *filename, const std::string &value) {
    if (value == std::string("json")) {
        return true;
    }
    return false;
}

DEFINE_string(input, "", "Path to the input file.");
DEFINE_string(format, "json", "Skeleton file input format.");
DEFINE_validator(format, &ValidateInputFileFormat);
DEFINE_string(
    output, "",
    "Path to the output file (format determined by extension. No extension implies Neuroglancer formatted files.");

int main(int argc, char *argv[]) {
    google::InstallFailureSignalHandler();

    google::InitGoogleLogging(argv[0]);
    gflags::SetVersionString(
        std::to_string(DataManager_VERSION_MAJOR) + "." + std::to_string(DataManager_VERSION_MINOR) + "." +
        std::to_string(DataManager_VERSION_PATCH) + " build " + std::to_string(DataManager_VERSION_BUILDDATE));
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    if (FLAGS_input.size() > 0) {
        const auto file_path = fs::path(FLAGS_input);
        CHECK(fs::is_regular_file(file_path));

        Skeleton_namespace::SkeletonBuilder builder;

        if (FLAGS_format == std::string("json")) {
            builder.FromJson(FLAGS_input);
        }

        if (FLAGS_output.size() > 0) {
            builder.writeNeuroglancerFile(FLAGS_output);
        }

    } else {
        LOG(WARNING) << "No input file specified. Nothing to do.";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
