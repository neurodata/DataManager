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

#include "gtest/gtest.h"

#include <iostream>
#include <memory>

#include <boost/filesystem.hpp>

#include <BlockManager/BlockManager.h>
#include <BlockManager/Blocks/Block.h>
#include <BlockManager/Datastore/FilesystemBlockStore.h>
#include <DataArray/DataArray.h>

using namespace BlockManager_namespace;

namespace {

static std::string test_directory("/tmp/ndm_test");

static void delete_directory(const std::string& dir) {
    const auto dir_path = boost::filesystem::path(dir);
    boost::filesystem::remove_all(dir_path);
}

static void make_test_directory() {
    const auto dir_path = boost::filesystem::path(test_directory);
    if (boost::filesystem::exists(dir_path)) {
        delete_directory(dir_path.string());
    }
    assert(boost::filesystem::create_directory(dir_path));
    assert(boost::filesystem::create_directory(dir_path / boost::filesystem::path("0")));
}

static std::shared_ptr<Manifest> make_manifest() {
    Scale scale;
    scale.key = "0";
    const int size[3] = {1024, 1025, 64};
    const int voxel_offset[3] = {0, 1, 0};
    const double resolution[3] = {1.0, 1.0, 1.0};
    for (int i = 0; i < 3; i++) {
        scale.size[i] = size[i];
        scale.voxel_offset[i] = voxel_offset[i];
        scale.resolution[i] = resolution[i];
    }
    auto chunk_size = std::array<int, 3>({128, 128, 16});
    auto chunk_sizes = std::vector<std::array<int, 3>>();
    chunk_sizes.push_back(chunk_size);
    scale.chunk_sizes = chunk_sizes;
    scale.encoding = "raw";

    const auto manifestShPtr = std::make_shared<Manifest>();
    manifestShPtr->set_type("segmentation");
    manifestShPtr->set_data_type("uint32");
    manifestShPtr->set_num_channels(1);
    manifestShPtr->add_scale(scale);

    return manifestShPtr;
}

static std::shared_ptr<Manifest> setup_filesystem_datastore() {
    make_test_directory();
    return make_manifest();
}

static std::shared_ptr<FilesystemBlockStore> filesystem_datastore_ptr() {
    return std::make_shared<FilesystemBlockStore>(FilesystemBlockStore(test_directory));
}

std::shared_ptr<DataArray_namespace::DataArray<uint32_t>> make_test_array(unsigned int xsize, unsigned int ysize,
                                                                          unsigned int zsize, int test_id) {
    auto dataArrayShPtr = std::make_shared<DataArray_namespace::DataArray<uint32_t>>(xsize, ysize, zsize);

    for (unsigned int z = 0; z < zsize; z++) {
        for (unsigned int y = 0; y < ysize; y++) {
            for (unsigned int x = 0; x < xsize; x++) {
                (*dataArrayShPtr)(x, y, z) = x + test_id * 1e3;
            }
        }
    }

    return dataArrayShPtr;
}

void check_arr_equal(const DataArray_namespace::DataArray<uint32_t>& A,
                     const DataArray_namespace::DataArray<uint32_t>& B, int xsize, int ysize, int zsize) {
    for (int x = 0; x < xsize; x++) {
        for (int y = 0; y < ysize; y++) {
            for (int z = 0; z < zsize; z++) {
                ASSERT_EQ(A(x, y, z), B(x, y, z)) << "(" << x << ", " << y << ", " << z << ")";
            }
        }
    }
}

class BlockManagerTest : public ::testing::Test {
   protected:
    BlockManagerTest() {
        auto manifestShPtr = setup_filesystem_datastore();
        BLMShPtr = std::make_shared<BlockManager>(
            BlockManager(manifestShPtr, filesystem_datastore_ptr(), BlockSettings({/*gzip=*/false})));
    }

    ~BlockManagerTest() { delete_directory(test_directory); }

    std::shared_ptr<BlockManager> BLMShPtr;
};

TEST_F(BlockManagerTest, Aligned) {
    int xsize = 128;
    int ysize = 128;
    int zsize = 16;
    const auto testArr = make_test_array(xsize, ysize, zsize, 1);
    const auto xrng = std::array<int, 2>({0, 128});
    const auto yrng = std::array<int, 2>({0, 128});
    const auto zrng = std::array<int, 2>({0, 16});
    const auto scale_key = std::string("0");
    BLMShPtr->Put(*testArr, xrng, yrng, zrng, scale_key);

    const auto outArr = DataArray_namespace::DataArray<uint32_t>(xsize, ysize, zsize);

    BLMShPtr->Get(outArr, xrng, yrng, zrng, scale_key);
    check_arr_equal(*testArr, outArr, xsize, ysize, zsize);
}

TEST_F(BlockManagerTest, AlignedOffsetSubtraction) {
    int xsize = 128;
    int ysize = 128;
    int zsize = 16;
    const auto testArr = make_test_array(xsize, ysize, zsize, 2);
    const auto xrng = std::array<int, 2>({0, 128});
    const auto yrng = std::array<int, 2>({1, 129});
    const auto zrng = std::array<int, 2>({0, 16});
    const auto scale_key = std::string("0");
    BLMShPtr->Put(*testArr, xrng, yrng, zrng, scale_key, /*subtractVoxelOffset=*/true);

    const auto outArr = DataArray_namespace::DataArray<uint32_t>(xsize, ysize, zsize);

    BLMShPtr->Get(outArr, xrng, yrng, zrng, scale_key, /*subtractVoxelOffset=*/true);
    check_arr_equal(*testArr, outArr, xsize, ysize, zsize);
}

TEST_F(BlockManagerTest, Slab) {
    int xsize = 1024;
    int ysize = 1024;
    int zsize = 1;
    const auto testArr = make_test_array(xsize, ysize, zsize, 3);
    const auto xrng = std::array<int, 2>({0, 1024});
    const auto yrng = std::array<int, 2>({0, 1024});
    const auto zrng = std::array<int, 2>({16, 17});
    const auto scale_key = std::string("0");
    BLMShPtr->Put(*testArr, xrng, yrng, zrng, scale_key);

    const auto outArr = DataArray_namespace::DataArray<uint32_t>(xsize, ysize, zsize);

    BLMShPtr->Get(outArr, xrng, yrng, zrng, scale_key);
    check_arr_equal(*testArr, outArr, xsize, ysize, zsize);
}

TEST_F(BlockManagerTest, UnalignedOffset) {
    int xsize = 200;
    int ysize = 351;
    int zsize = 19;
    const auto testArr = make_test_array(xsize, ysize, zsize, 4);
    const auto xrng = std::array<int, 2>({100, 300});
    const auto yrng = std::array<int, 2>({501, 852});
    const auto zrng = std::array<int, 2>({28, 47});
    const auto scale_key = std::string("0");
    BLMShPtr->Put(*testArr, xrng, yrng, zrng, scale_key);

    const auto outArr = DataArray_namespace::DataArray<uint32_t>(xsize, ysize, zsize);

    BLMShPtr->Get(outArr, xrng, yrng, zrng, scale_key);
    check_arr_equal(*testArr, outArr, xsize, ysize, zsize);
}

TEST_F(BlockManagerTest, UnalignedEdge) {
    int xsize = 200;
    int ysize = 351;
    int zsize = 19;
    const auto testArr = make_test_array(xsize, ysize, zsize, 5);
    const auto xrng = std::array<int, 2>({772, 972});
    const auto yrng = std::array<int, 2>({662, 1013});
    const auto zrng = std::array<int, 2>({40, 59});
    const auto scale_key = std::string("0");
    BLMShPtr->Put(*testArr, xrng, yrng, zrng, scale_key);

    auto outArr = DataArray_namespace::DataArray<uint32_t>(xsize, ysize, zsize);

    BLMShPtr->Get(outArr, xrng, yrng, zrng, scale_key);
    check_arr_equal(*testArr, outArr, xsize, ysize, zsize);
}

TEST_F(BlockManagerTest, UnalignedDoublePut) {
    // Put, Put, Get, Get (to ensure reading from the datasource is working)
    int xsize = 200;
    int ysize = 351;
    int zsize = 19;
    const auto testArr1 = make_test_array(xsize, ysize, zsize, 6);
    const auto testArr2 = make_test_array(xsize, ysize, zsize, 7);
    const auto scale_key = std::string("0");

    // Put 1
    {
        const auto xrng = std::array<int, 2>({300, 500});
        const auto yrng = std::array<int, 2>({150, 501});
        const auto zrng = std::array<int, 2>({28, 47});
        BLMShPtr->Put(*testArr1, xrng, yrng, zrng, scale_key);
    }
    // Put 2
    {
        const auto xrng = std::array<int, 2>({300, 500});
        const auto yrng = std::array<int, 2>({501, 852});
        const auto zrng = std::array<int, 2>({28, 47});
        BLMShPtr->Put(*testArr2, xrng, yrng, zrng, scale_key);
    }

    // Get 1
    {
        const auto xrng = std::array<int, 2>({300, 500});
        const auto yrng = std::array<int, 2>({150, 501});
        const auto zrng = std::array<int, 2>({28, 47});
        auto outArr = DataArray_namespace::DataArray<uint32_t>(xsize, ysize, zsize);
        BLMShPtr->Get(outArr, xrng, yrng, zrng, scale_key);
        check_arr_equal(*testArr1, outArr, xsize, ysize, zsize);
    }

    // Get 2
    {
        const auto xrng = std::array<int, 2>({300, 500});
        const auto yrng = std::array<int, 2>({501, 852});
        const auto zrng = std::array<int, 2>({28, 47});
        auto outArr = DataArray_namespace::DataArray<uint32_t>(xsize, ysize, zsize);
        BLMShPtr->Get(outArr, xrng, yrng, zrng, scale_key);
        check_arr_equal(*testArr2, outArr, xsize, ysize, zsize);
    }
}

TEST_F(BlockManagerTest, LastCorner) {
    int xsize = 200;
    int ysize = 200;
    int zsize = 18;
    const auto testArr = make_test_array(xsize, ysize, zsize, 8);
    const auto xrng = std::array<int, 2>({824, 1024});
    const auto yrng = std::array<int, 2>({825, 1025});
    const auto zrng = std::array<int, 2>({46, 64});
    const auto scale_key = std::string("0");
    BLMShPtr->Put(*testArr, xrng, yrng, zrng, scale_key, /*subtractVoxelOffset=*/true);

    const auto outArr = DataArray_namespace::DataArray<uint32_t>(xsize, ysize, zsize);

    BLMShPtr->Get(outArr, xrng, yrng, zrng, scale_key, /*subtractVoxelOffset=*/true);
    check_arr_equal(*testArr, outArr, xsize, ysize, zsize);
}

// TODO(adb): Move to a block/file format test case
class BlockManagerTestGzip : public ::testing::Test {
   protected:
    BlockManagerTestGzip() {
        // Note that since setup is done only once, it is important to ensure regions below do not overlap (or only
        // overlap fully). Otherwise, test failures may occur due to "old" data being present in the test results.
        auto manifestShPtr = setup_filesystem_datastore();
        BLMShPtr = std::make_shared<BlockManager>(
            BlockManager(manifestShPtr, filesystem_datastore_ptr(), BlockSettings({/*gzip=*/true})));
    }

    ~BlockManagerTestGzip() { delete_directory(test_directory); }

    std::shared_ptr<BlockManager> BLMShPtr;
};

TEST_F(BlockManagerTestGzip, AlignedGzip) {
    int xsize = 128;
    int ysize = 128;
    int zsize = 16;
    const auto testArr = make_test_array(xsize, ysize, zsize, 9);
    const auto xrng = std::array<int, 2>({0, 128});
    const auto yrng = std::array<int, 2>({0, 128});
    const auto zrng = std::array<int, 2>({0, 16});
    const auto scale_key = std::string("0");
    BLMShPtr->Put(*testArr, xrng, yrng, zrng, scale_key);

    const auto outArr = DataArray_namespace::DataArray<uint32_t>(xsize, ysize, zsize);

    BLMShPtr->Get(outArr, xrng, yrng, zrng, scale_key);
    check_arr_equal(*testArr, outArr, xsize, ysize, zsize);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

};  // namespace
