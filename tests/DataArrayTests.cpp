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

#include <memory>

#include <DataArray/DataArray.h>

using namespace DataArray_namespace;

namespace {

static const uint32_t input1[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
static const uint32_t input2[] = {0, 10, 20, 30, 40, 50, 60, 70, 80, 90};

TEST(DataArray, ArrayCreation) {
    auto dataArray = DataArray<uint32_t>(2, 2, 3);

    ASSERT_EQ(dataArray.num_elements(), 2 * 2 * 3);
    ASSERT_EQ(dataArray.num_bytes(), 2 * 2 * 3 * sizeof(uint32_t));
}

TEST(DataArray, GetAndSetOperators) {
    unsigned int xdim = 2;
    unsigned int ydim = 2;
    unsigned int zdim = 3;
    auto dataArray = DataArray<uint32_t>(xdim, ydim, zdim);
    dataArray(0, 0, 0) = 1;
    dataArray(1, 1, 0) = 10;
    dataArray(1, 1, 1) = 20;
    dataArray(1, 1, 2) = 50;

    ASSERT_EQ(dataArray(0, 0, 0), 1);
    ASSERT_EQ(dataArray[0], 1);

    ASSERT_EQ(dataArray(1, 1, 0), 10);
    ASSERT_EQ(dataArray[0 + 1 * zdim + 1 * zdim * ydim], 10);

    ASSERT_EQ(dataArray(1, 1, 1), 20);
    ASSERT_EQ(dataArray[1 + 1 * zdim + 1 * zdim * ydim], 20);

    ASSERT_EQ(dataArray(1, 1, 2), 50);
    ASSERT_EQ(dataArray[2 + 1 * zdim + 1 * zdim * ydim], 50);
}

TEST(DataArray, View) {
    unsigned int xdim = 3;
    unsigned int ydim = 3;
    unsigned int zdim = 3;
    auto dataArray = DataArray<uint32_t>(xdim, ydim, zdim);
    dataArray(0, 0, 0) = 10;
    dataArray(1, 1, 1) = 20;
    dataArray(1, 2, 1) = 30;
    dataArray(1, 0, 2) = 40;
    dataArray(2, 2, 1) = 50;
    dataArray(1, 1, 2) = 60;
    dataArray(2, 2, 2) = 70;

    // Generate a view that is offset by 1 in each dimension
    auto arrView = dataArray.view({1, 3}, {1, 3}, {1, 3});
    ASSERT_EQ(arrView[0][0][0], dataArray(1, 1, 1));
    ASSERT_EQ(arrView[0][1][0], dataArray(1, 2, 1));
    ASSERT_EQ(arrView[1][1][0], dataArray(2, 2, 1));
    ASSERT_EQ(arrView[0][0][1], dataArray(1, 1, 2));
    ASSERT_EQ(arrView[1][1][1], dataArray(2, 2, 2));
}

TEST(DataArray, Clear) {
    unsigned int xdim = 2;
    unsigned int ydim = 2;
    unsigned int zdim = 3;
    auto dataArray = DataArray<uint32_t>(xdim, ydim, zdim);
    dataArray(0, 0, 0) = 1;
    dataArray(1, 1, 0) = 10;
    dataArray(1, 1, 1) = 20;
    dataArray(1, 1, 2) = 50;

    dataArray.clear();

    ASSERT_EQ(dataArray(0, 0, 0), 0);
    ASSERT_EQ(dataArray(1, 1, 0), 0);
    ASSERT_EQ(dataArray(1, 1, 1), 0);
    ASSERT_EQ(dataArray(1, 1, 2), 0);
}

TEST(DataArray, ConstructorFromCharArray) {
    unsigned int xdim = 2;
    unsigned int ydim = 2;
    unsigned int zdim = 3;
    auto inputData = std::unique_ptr<char[]>(new char[xdim * ydim * zdim * sizeof(uint32_t)]);
    std::memset(inputData.get(), 0, xdim * ydim * zdim * sizeof(uint32_t));
    auto inputDataPtr = reinterpret_cast<uint32_t*>(inputData.get());
    inputDataPtr[0] = 10;
    inputDataPtr[4] = 20;
    inputDataPtr[5] = 30;
    inputDataPtr[10] = 40;
    inputDataPtr[11] = 50;

    auto dataArray = DataArray<uint32_t>(inputData, xdim, ydim, zdim);

    ASSERT_EQ(dataArray[0], 10);
    ASSERT_EQ(dataArray[4], 20);
    ASSERT_EQ(dataArray[5], 30);
    ASSERT_EQ(dataArray[10], 40);
    ASSERT_EQ(dataArray[11], 50);
}

TEST(DataArray, CopyOut) {
    unsigned int xdim = 2;
    unsigned int ydim = 2;
    unsigned int zdim = 3;
    auto inputData = std::unique_ptr<char[]>(new char[xdim * ydim * zdim * sizeof(uint32_t)]);
    std::memset(inputData.get(), 0, xdim * ydim * zdim * sizeof(uint32_t));
    auto inputDataPtr = reinterpret_cast<uint32_t*>(inputData.get());
    inputDataPtr[0] = 10;
    inputDataPtr[4] = 20;
    inputDataPtr[5] = 30;
    inputDataPtr[10] = 40;
    inputDataPtr[11] = 50;

    auto dataArray = DataArray<uint32_t>(inputData, xdim, ydim, zdim);
    auto outputData = std::unique_ptr<char[]>(new char[xdim * ydim * zdim * sizeof(uint32_t)]);
    dataArray.copy(outputData, xdim, ydim, zdim);
    auto outputDataPtr = reinterpret_cast<uint32_t*>(outputData.get());

    for (size_t i = 0; i < xdim * ydim * zdim; i++) {
        ASSERT_EQ(outputDataPtr[i], inputDataPtr[i]);
    }
}

/**
 * The following tests a simple create, set, get, clear, size for differening datatypes
 */
TEST(DataArray, uint8_DataType) {
    unsigned int xdim = 2;
    unsigned int ydim = 2;
    unsigned int zdim = 3;
    auto inputData = std::unique_ptr<char[]>(new char[xdim * ydim * zdim * sizeof(uint8_t)]);
    std::memset(inputData.get(), 0, xdim * ydim * zdim * sizeof(uint8_t));
    auto inputDataPtr = reinterpret_cast<uint8_t*>(inputData.get());
    inputDataPtr[0] = 10;
    inputDataPtr[4] = 20;
    inputDataPtr[5] = 30;
    inputDataPtr[10] = 40;
    inputDataPtr[11] = 50;

    auto dataArray = DataArray<uint8_t>(inputData, xdim, ydim, zdim);

    ASSERT_EQ(dataArray[0], 10);
    ASSERT_EQ(dataArray[4], 20);
    ASSERT_EQ(dataArray[5], 30);
    ASSERT_EQ(dataArray[10], 40);
    ASSERT_EQ(dataArray[11], 50);
}

TEST(DataArray, uint16_DataType) {
    unsigned int xdim = 2;
    unsigned int ydim = 2;
    unsigned int zdim = 3;
    auto inputData = std::unique_ptr<char[]>(new char[xdim * ydim * zdim * sizeof(uint16_t)]);
    std::memset(inputData.get(), 0, xdim * ydim * zdim * sizeof(uint16_t));
    auto inputDataPtr = reinterpret_cast<uint16_t*>(inputData.get());
    inputDataPtr[0] = 100;
    inputDataPtr[4] = 200;
    inputDataPtr[5] = 300;
    inputDataPtr[10] = 400;
    inputDataPtr[11] = 500;

    auto dataArray = DataArray<uint16_t>(inputData, xdim, ydim, zdim);

    ASSERT_EQ(dataArray[0], 100);
    ASSERT_EQ(dataArray[4], 200);
    ASSERT_EQ(dataArray[5], 300);
    ASSERT_EQ(dataArray[10], 400);
    ASSERT_EQ(dataArray[11], 500);
}

TEST(DataArray, uint64_DataType) {
    unsigned int xdim = 2;
    unsigned int ydim = 2;
    unsigned int zdim = 3;
    auto inputData = std::unique_ptr<char[]>(new char[xdim * ydim * zdim * sizeof(uint64_t)]);
    std::memset(inputData.get(), 0, xdim * ydim * zdim * sizeof(uint64_t));
    auto inputDataPtr = reinterpret_cast<uint64_t*>(inputData.get());
    inputDataPtr[0] = 100;
    inputDataPtr[4] = 200;
    inputDataPtr[5] = 300;
    inputDataPtr[10] = 400;
    inputDataPtr[11] = 500;

    auto dataArray = DataArray<uint64_t>(inputData, xdim, ydim, zdim);

    ASSERT_EQ(dataArray[0], 100);
    ASSERT_EQ(dataArray[4], 200);
    ASSERT_EQ(dataArray[5], 300);
    ASSERT_EQ(dataArray[10], 400);
    ASSERT_EQ(dataArray[11], 500);
}

TEST(DataArray, Float32_DataType) {
    unsigned int xdim = 2;
    unsigned int ydim = 2;
    unsigned int zdim = 3;
    auto inputData = std::unique_ptr<char[]>(new char[xdim * ydim * zdim * sizeof(float)]);
    std::memset(inputData.get(), 0, xdim * ydim * zdim * sizeof(float));
    auto inputDataPtr = reinterpret_cast<float*>(inputData.get());
    inputDataPtr[0] = 0.5;
    inputDataPtr[4] = 1.0;
    inputDataPtr[5] = 20.0;
    inputDataPtr[10] = 50.0;
    inputDataPtr[11] = 50000.5;

    auto dataArray = DataArray<float>(inputData, xdim, ydim, zdim);

    ASSERT_EQ(dataArray[0], 0.5);
    ASSERT_EQ(dataArray[4], 1.0);
    ASSERT_EQ(dataArray[5], 20.0);
    ASSERT_EQ(dataArray[10], 50.0);
    ASSERT_EQ(dataArray[11], 50000.5);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

};  // namespace