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

#ifndef TRIANGLE_MESH_H
#define TRIANGLE_MESH_H

#include <fstream>
#include <memory>
#include <vector>

#include "Mesh.h"
#include "PolygonalMesh.h"
#include "include/Geometry.h"

#include <boost/filesystem.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_stream.hpp>

namespace fs = boost::filesystem;
namespace io = boost::iostreams;

namespace MeshExtractor_namespace {

template <class Real>
using Vertex = Point3D<Real>;

template <class Real>
struct TriangleMesh : public Mesh {
    std::vector<Vertex<Real>> vertices;
    std::vector<TriangleIndex> triangles;

    TriangleMesh() {}
    TriangleMesh(const PolygonalMesh<Real>& polyMesh) : TriangleMesh(polyMesh, /*nonManifold=*/true) {}
    TriangleMesh(const PolygonalMesh<Real>& polyMesh, bool nonManifold) {
        id = polyMesh.id;

        MinimalAreaTriangulation<Real> mat;

        vertices.resize(polyMesh.vertices.size());
        for (size_t i = 0; i < polyMesh.vertices.size(); i++) {
            for (size_t j = 0; j < 3; j++) vertices[i][j] = polyMesh.vertices[i].p[j];
        }

        for (size_t i = 0; i < polyMesh.polygons.size(); i++) {
            // To ensure that we have no more than two triangles adjacent on an edge,
            // we avoid creating a minimial area triangulation when it could introduce a new
            // edge that is on a face of a cube
            bool isCofacial = false;
            if (!nonManifold)
                for (size_t j = 0; j < polyMesh.polygons[i].size(); j++)
                    for (size_t k = 0; k < j; k++)
                        if ((j + 1) % polyMesh.polygons[i].size() != k && (k + 1) % polyMesh.polygons[i].size() != j)
                            if (IsoVertex<Real>::CoFacial(polyMesh.vertices[polyMesh.polygons[i][j]],
                                                          polyMesh.vertices[polyMesh.polygons[i][k]]))
                                isCofacial = true;

            if (isCofacial) {
                TriangleIndex triangle;
                Vertex<Real> vertex;
                for (size_t j = 0; j < polyMesh.polygons[i].size(); j++)
                    vertex += polyMesh.vertices[polyMesh.polygons[i][j]].p;
                vertex /= (Real)polyMesh.polygons[i].size();
                int cIdx = static_cast<int>(vertices.size());
                vertices.push_back(vertex);
                for (size_t j = 0; j < polyMesh.polygons[i].size(); j++) {
                    triangle[0] = polyMesh.polygons[i][j];
                    triangle[1] = polyMesh.polygons[i][(j + 1) % polyMesh.polygons[i].size()];
                    triangle[2] = cIdx;
                    triangles.push_back(triangle);
                }
            } else {
                std::vector<Point3D<Real>> _polygon(polyMesh.polygons[i].size());
                std::vector<TriangleIndex> _triangles;
                for (size_t j = 0; j < polyMesh.polygons[i].size(); j++)
                    _polygon[j] = polyMesh.vertices[polyMesh.polygons[i][j]].p;
                mat.GetTriangulation(_polygon, _triangles);
                for (size_t j = 0; j < _triangles.size(); j++) {
                    TriangleIndex tri;
                    for (int k = 0; k < 3; k++) tri[k] = polyMesh.polygons[i][_triangles[j][k]];
                    triangles.push_back(tri);
                }
            }
        }
    }

    size_t bytes() const {
        return sizeof(uint32_t) + (3 * vertices.size() * sizeof(float)) + (3 * triangles.size() * sizeof(uint32_t));
    }

    /**
     * Neuroglancer Binary Format
     * num_vertices (uint32 or uint64)
     * <<vertex list>> (float, float, float)
     * <<index list>> (uint32, uint32, uint32)
     */
    static std::unique_ptr<char[]> AsNeuroglancerBin(const TriangleMesh& mesh) {
        auto buf = std::unique_ptr<char[]>(new char[mesh.bytes()]);

        // TODO(adb): support 64-bit headers (though we may be phasing out the use of those files for meshes at least)
        size_t header_byte_sz = sizeof(uint32_t);

        auto buf_ptr = buf.get();
        uint32_t num_vertices = static_cast<uint32_t>(mesh.vertices.size());
        std::memcpy(buf_ptr, &num_vertices, header_byte_sz);
        buf_ptr += header_byte_sz;

        // Vertex list
        size_t vertex_size = 3 * sizeof(float);
        for (const auto& vertex : mesh.vertices) {
            std::memcpy(buf_ptr, &vertex.coords[0], vertex_size);
            buf_ptr += vertex_size;
        }

        // Triangle list
        size_t triangle_size = 3 * sizeof(uint32_t);
        for (const auto& triangle : mesh.triangles) {
            uint32_t tri[3] = {triangle[0], triangle[1], triangle[2]};
            std::memcpy(buf_ptr, tri, triangle_size);
            buf_ptr += triangle_size;
        }

        return std::move(buf);
    }
};

}  // namespace MeshExtractor_namespace

#endif