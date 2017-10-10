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

#ifndef MESH_EXTRACTOR_H
#define MESH_EXTRACTOR_H

#include <DataArray/DataArray.h>
#include "TriangleMesh.h"

#include <memory>
#include <vector>

namespace MeshExtractor_namespace {

typedef DataArray_namespace::DataArray3D<uint32_t> DataArray32;
typedef std::shared_ptr<DataArray32> DataArray32ShPtr;

struct MarchingCubesProperties {
    bool quadratic;
    bool fullCaseTable;
    bool flip;
    MarchingCubesProperties() : quadratic(false), fullCaseTable(false), flip(false) {}
};

class MeshExtractor {
   public:
    MeshExtractor(const std::shared_ptr<DataArray_namespace::DataArray3D<uint32_t>> input_voxel_grid, unsigned int xdim,
                  unsigned int ydim, unsigned int zdim, const std::shared_ptr<MarchingCubesProperties>& props);
    ~MeshExtractor() {}

    virtual void extract(std::vector<std::shared_ptr<TriangleMesh<float>>>& meshes) = 0;

    void setOffsets(unsigned int xoffset, unsigned int yoffset, unsigned int zoffset);
    void setVoxelResolution(float xres, float yres, float zres);

    std::vector<uint32_t> getIdsInVoxelGrid() const;

    static std::vector<unsigned char> BinaryMaskGrid(const DataArray32ShPtr& input_grid, unsigned int xdim,
                                                     unsigned int ydim, unsigned int zdim, uint32_t id);

   protected:
    std::shared_ptr<DataArray_namespace::DataArray3D<uint32_t>> voxel_grid;
    unsigned int _xdim;
    unsigned int _ydim;
    unsigned int _zdim;
    unsigned int _xoffset = 0;
    unsigned int _yoffset = 0;
    unsigned int _zoffset = 0;

    float _xres = 1.0;
    float _yres = 1.0;
    float _zres = 1.0;

    std::shared_ptr<MarchingCubesProperties> _props;
};

class MishaMeshExtractor : public MeshExtractor {
   public:
    MishaMeshExtractor(const std::shared_ptr<DataArray_namespace::DataArray3D<uint32_t>> input_voxel_grid,
                       unsigned int xdim, unsigned int ydim, unsigned int zdim,
                       const std::shared_ptr<MarchingCubesProperties>& props)
        : MeshExtractor(input_voxel_grid, xdim, ydim, zdim, props) {}
    ~MishaMeshExtractor() {}

    void extractID(uint32_t id) { idsToExtract.push_back(id); }

    void clearIDs() { idsToExtract.clear(); }

    std::vector<uint32_t> getIds() const { return idsToExtract; }

    void setIsovalue(float isovalue) { _isovalue = isovalue; }

    float getIsoValue() const { return _isovalue; }

    void extract(std::vector<std::shared_ptr<TriangleMesh<float>>>& meshes);

   protected:
    std::vector<uint32_t> idsToExtract;
    float _isovalue = 0.5;
};

class NeuroglancerMeshExtractor : public MeshExtractor {
   public:
    NeuroglancerMeshExtractor(const std::shared_ptr<DataArray_namespace::DataArray3D<uint32_t>> input_voxel_grid,
                              unsigned int xdim, unsigned int ydim, unsigned int zdim,
                              const std::shared_ptr<MarchingCubesProperties>& props)
        : MeshExtractor(input_voxel_grid, xdim, ydim, zdim, props) {}
    ~NeuroglancerMeshExtractor() {}

    void extract(std::vector<std::shared_ptr<TriangleMesh<float>>>& meshes);
};

}  // namespace MeshExtractor_namespace

#endif