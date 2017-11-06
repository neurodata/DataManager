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

#include "MeshExtractor.h"

#include <third_party/NeuroglancerMesh/mesh_objects.h>
#include "MarchingCubes.h"
#include "PolygonalMesh.h"
#include "include/Interpolate.h"

#include <map>
#include <set>

#include <omp.h>

using namespace MeshExtractor_namespace;
using namespace neuroglancer;

// TODO(adb): need to decide where we want to template and where we don't want to template...
typedef float Real;

MeshExtractor::MeshExtractor(const std::shared_ptr<DataArray_namespace::DataArray3D<uint32_t>> input_voxel_grid,
                             unsigned int xdim, unsigned int ydim, unsigned int zdim,
                             const std::shared_ptr<MarchingCubesProperties>& props)
    : voxel_grid(input_voxel_grid), _xdim(xdim), _ydim(ydim), _zdim(zdim), _props(props) {}

void MeshExtractor::setOffsets(unsigned int xoffset, unsigned int yoffset, unsigned int zoffset) {
    _xoffset = xoffset;
    _yoffset = yoffset;
    _zoffset = zoffset;
}

void MeshExtractor::setVoxelResolution(float xres, float yres, float zres) {
    _xres = xres;
    _yres = yres;
    _zres = zres;
}

std::vector<uint32_t> MeshExtractor::getIdsInVoxelGrid() const {
    std::set<uint32_t> id_set;
    std::vector<uint32_t> ids;
    for (unsigned int x = 0; x < _xdim; x++) {
        for (unsigned int y = 0; y < _ydim; y++) {
            for (unsigned int z = 0; z < _zdim; z++) {
                id_set.insert((*voxel_grid)(x, y, z));
            }
        }
    }

    // Remove ID 0
    id_set.erase(0);

    ids.reserve(id_set.size());
    std::copy(id_set.begin(), id_set.end(), std::back_inserter(ids));
    return ids;
}

std::vector<unsigned char> MeshExtractor::BinaryMaskGrid(const DataArray32ShPtr& input_grid, unsigned int xdim,
                                                         unsigned int ydim, unsigned int zdim, uint32_t id) {
    std::vector<unsigned char> ret(xdim * ydim * zdim);

#pragma omp parallel for
    for (unsigned int z = 0; z < zdim; z++)
        for (unsigned int y = 0; y < ydim; y++)
            for (unsigned int x = 0; x < xdim; x++)
                if ((*input_grid)(x, y, z) == id) ret[x + y * xdim + z * xdim * ydim] = 1;

    return ret;
}

void MishaMeshExtractor::extract(std::vector<std::shared_ptr<TriangleMesh<float>>>& meshes) {
    if (idsToExtract.size() == 0) {
        idsToExtract = getIdsInVoxelGrid();
    }

    for (const auto& id : idsToExtract) {
        LOG(INFO) << "Extracting id: " << id;

        auto polygonalMeshShPtr = std::make_shared<PolygonalMesh<Real>>();
        polygonalMeshShPtr->id = id;
        // Binary mask the input grid
        const auto binaryGridPtr = MeshExtractor::BinaryMaskGrid(voxel_grid, _xdim, _ydim, _zdim, id);
#if 0
        auto BinaryGridValue = [&](int idx) { return static_cast<Real>((*binaryGridPtr)[idx]); };
#else
        auto BinaryGridValue = [&](int idx) { return static_cast<Real>(binaryGridPtr[idx]); };
#endif

#define INDEX(x, y, z)                                                                                           \
    (std::min<int>(_xdim - 1, std::max<int>(0, (x))) + std::min<int>(_ydim - 1, std::max<int>(0, (y))) * _xdim + \
     std::min<int>(_zdim - 1, std::max<int>(0, (z))) * _xdim * _ydim)

        std::map<long long, int> isoVertexMap[3];

        auto flags = std::unique_ptr<unsigned char[]>(new unsigned char[_xdim * _ydim * _zdim]);

        // Mark the voxels that are larger than the iso value
#pragma omp parallel for
        for (unsigned int i = 0; i < _xdim * _ydim * _zdim; i++) {
            flags[i] = MarchingCubes::ValueLabel(BinaryGridValue(i), _isovalue);
        }

            // Get the zero-crossings along the x-edges
#pragma omp parallel for
        for (unsigned int i = 0; i < _xdim - 1; i++)
            for (unsigned int j = 0; j < _ydim; j++)
                for (unsigned int k = 0; k < _zdim; k++) {
                    int idx0 = INDEX(i, j, k), idx1 = INDEX(i + 1, j, k);
                    if (flags[idx0] != flags[idx1]) {
                        Real iso;
                        if (_props->quadratic) {
                            int _idx0 = INDEX(i - 1, j, k), _idx1 = INDEX(i + 2, j, k);
                            iso = QuadraticInterpolant(BinaryGridValue(_idx0), BinaryGridValue(idx0),
                                                       BinaryGridValue(idx1), BinaryGridValue(_idx1), _isovalue);
                        } else
                            iso = LinearInterpolant(BinaryGridValue(idx0), BinaryGridValue(idx1), _isovalue);
                        Point3D<Real> p = Point3D<Real>((Real)i + iso, (Real)j, (Real)k);
                        long long key = i + j * (_xdim) + k * (_xdim * _ydim);
#pragma omp critical
                        {
                            isoVertexMap[0][key] = static_cast<int>(polygonalMeshShPtr->vertices.size());
                            polygonalMeshShPtr->vertices.push_back(IsoVertex<Real>(p, 0, i, j, k));
                        }
                    }
                }

                    // Get the zero-crossings along the y-edges
#pragma omp parallel for
        for (unsigned int i = 0; i < _xdim; i++)
            for (unsigned int j = 0; j < _ydim - 1; j++)
                for (unsigned int k = 0; k < _zdim; k++) {
                    int idx0 = INDEX(i, j, k), idx1 = INDEX(i, j + 1, k);
                    if (flags[idx0] != flags[idx1]) {
                        Real iso;
                        if (_props->quadratic) {
                            int _idx0 = INDEX(i, j - 1, k), _idx1 = INDEX(i, j + 2, k);
                            iso = QuadraticInterpolant(BinaryGridValue(_idx0), BinaryGridValue(idx0),
                                                       BinaryGridValue(idx1), BinaryGridValue(_idx1), _isovalue);
                        } else
                            iso = LinearInterpolant(BinaryGridValue(idx0), BinaryGridValue(idx1), _isovalue);
                        Point3D<Real> p = Point3D<Real>((Real)i, (Real)j + iso, (Real)k);
                        long long key = i + j * (_xdim) + k * (_xdim * _ydim);
#pragma omp critical
                        {
                            isoVertexMap[1][key] = static_cast<int>(polygonalMeshShPtr->vertices.size());
                            polygonalMeshShPtr->vertices.push_back(IsoVertex<Real>(p, 1, i, j, k));
                        }
                    }
                }

                    // Get the zero-crossings along the z-edges
#pragma omp parallel for
        for (unsigned int i = 0; i < _xdim; i++)
            for (unsigned int j = 0; j < _ydim; j++)
                for (unsigned int k = 0; k < _zdim - 1; k++) {
                    int idx0 = INDEX(i, j, k), idx1 = INDEX(i, j, k + 1);
                    if (flags[idx0] != flags[idx1]) {
                        Real iso;
                        if (_props->quadratic) {
                            int _idx0 = INDEX(i, j, k - 1), _idx1 = INDEX(i, j, k + 2);
                            iso = QuadraticInterpolant(BinaryGridValue(_idx0), BinaryGridValue(idx0),
                                                       BinaryGridValue(idx1), BinaryGridValue(_idx1), _isovalue);
                        } else
                            iso = LinearInterpolant(BinaryGridValue(idx0), BinaryGridValue(idx1), _isovalue);
                        Point3D<Real> p = Point3D<Real>((Real)i, (Real)j, (Real)k + iso);
                        long long key = i + j * (_xdim) + k * (_xdim * _ydim);
#pragma omp critical
                        {
                            isoVertexMap[2][key] = static_cast<int>(polygonalMeshShPtr->vertices.size());
                            polygonalMeshShPtr->vertices.push_back(IsoVertex<Real>(p, 2, i, j, k));
                        }
                    }
                }

        // Iterate over the cubes and get the polygons
        if (_props->fullCaseTable)
            MarchingCubes::SetFullCaseTable();
        else
            MarchingCubes::SetCaseTable();

#pragma omp parallel for
        for (unsigned int i = 0; i < _xdim - 1; i++)
            for (unsigned int j = 0; j < _ydim - 1; j++)
                for (unsigned int k = 0; k < _zdim - 1; k++) {
                    Real _values[Cube::CORNERS];
                    for (int cx = 0; cx < 2; cx++)
                        for (int cy = 0; cy < 2; cy++)
                            for (int cz = 0; cz < 2; cz++)
                                _values[Cube::CornerIndex(cx, cy, cz)] =
                                    BinaryGridValue((i + cx) + (j + cy) * _xdim + (k + cz) * _xdim * _ydim);
                    int mcIndex = _props->fullCaseTable ? MarchingCubes::GetFullIndex(_values, _isovalue)
                                                        : MarchingCubes::GetIndex(_values, _isovalue);
                    const std::vector<std::vector<int>>& isoPolygons =
                        MarchingCubes::caseTable(mcIndex, _props->fullCaseTable);
                    for (unsigned int p = 0; p < isoPolygons.size(); p++) {
                        const std::vector<int>& isoPolygon = isoPolygons[p];
                        std::vector<int> polygon(isoPolygon.size());
                        for (unsigned int v = 0; v < isoPolygon.size(); v++) {
                            int orientation, i1, i2;
                            Cube::FactorEdgeIndex(isoPolygon[v], orientation, i1, i2);
                            long long key;
                            switch (orientation) {
                                case 0:
                                    key = (i) + (j + i1) * _xdim + (k + i2) * _xdim * _ydim;
                                    break;
                                case 1:
                                    key = (i + i1) + (j)*_xdim + (k + i2) * _xdim * _ydim;
                                    break;
                                case 2:
                                    key = (i + i1) + (j + i2) * _xdim + (k)*_xdim * _ydim;
                                    break;
                            }
                            std::map<long long, int>::const_iterator iter = isoVertexMap[orientation].find(key);
                            if (iter == isoVertexMap[orientation].end())
                                LOG(FATAL) << "Couldn't find iso-vertex in map.";
                            if (_props->flip)
                                polygon[polygon.size() - 1 - v] = iter->second;
                            else
                                polygon[v] = iter->second;
                        }
#pragma omp critical
                        polygonalMeshShPtr->polygons.push_back(polygon);
                    }
                }

        flags.reset();

#undef INDEX

#pragma omp parallel for
        for (size_t i = 0; i < polygonalMeshShPtr->vertices.size(); i++) {
            polygonalMeshShPtr->vertices[i].p[0] =
                (polygonalMeshShPtr->vertices[i].p[0] + static_cast<Real>(_xoffset)) * _xres;
            polygonalMeshShPtr->vertices[i].p[1] =
                (polygonalMeshShPtr->vertices[i].p[1] + static_cast<Real>(_yoffset)) * _yres;
            polygonalMeshShPtr->vertices[i].p[2] =
                (polygonalMeshShPtr->vertices[i].p[2] + static_cast<Real>(_zoffset)) * _zres;
        }

        meshes.push_back(std::make_shared<TriangleMesh<Real>>(*polygonalMeshShPtr));
    }
}

void NeuroglancerMeshExtractor::extract(std::vector<std::shared_ptr<TriangleMesh<float>>>& meshes) {
    std::unordered_map<uint64_t, meshing::TriangleMesh> mesh_map;
    meshing::MeshObjects(voxel_grid->origin(), {_xdim, _ydim, _zdim}, {_ydim * _zdim, _zdim, 1}, &mesh_map);

    for (const auto& mesh : mesh_map) {
        auto triMeshPtr = std::make_shared<TriangleMesh<float>>();
        triMeshPtr->vertices.resize(mesh.second.vertex_positions.size());
        triMeshPtr->triangles.resize(mesh.second.triangles.size());

#pragma omp parallel for
        for (size_t i = 0; i < triMeshPtr->vertices.size(); i++) {
            triMeshPtr->vertices[i][0] = (mesh.second.vertex_positions[i][0] + static_cast<float>(_xoffset)) * _xres;
            triMeshPtr->vertices[i][1] = (mesh.second.vertex_positions[i][1] + static_cast<float>(_yoffset)) * _yres;
            triMeshPtr->vertices[i][2] = (mesh.second.vertex_positions[i][2] + static_cast<float>(_zoffset)) * _zres;
        }

#pragma omp parallel for
        for (size_t i = 0; i < triMeshPtr->triangles.size(); i++) {
            for (size_t d = 0; d < 3; d++) triMeshPtr->triangles[i][d] = mesh.second.triangles[i][d];
        }

        triMeshPtr->id = static_cast<uint32_t>(mesh.first);

        meshes.push_back(triMeshPtr);
    }
}
