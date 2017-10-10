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

#include "TriangleMeshIO.h"
#include "include/Geometry.h"
#include "include/PLY/Ply.h"

using namespace MeshExtractor_namespace;

template <class Real>
void TriangleMeshIO<Real>::WritePLY(const std::string& filename, const TriangleMesh<Real>& mesh, bool ascii) {
    std::vector<PlyVertex<Real> > _vertices(mesh.vertices.size());
    for (size_t i = 0; i < mesh.vertices.size(); i++)
        for (size_t d = 0; d < 3; d++) _vertices[i].point[d] = mesh.vertices[i][d];

    PlyWriteTriangles(filename.c_str(), _vertices, mesh.triangles, PlyVertex<Real>::WriteProperties,
                      PlyVertex<Real>::WriteComponents, ascii ? PLY_ASCII : PLY_BINARY_NATIVE);
}

template <class Real>
void TriangleMeshIO<Real>::WriteNeuroglancerBin(const std::string& filename, const TriangleMesh<Real>& mesh,
                                                bool gzip) {
    auto buf = TriangleMesh<Real>::AsNeuroglancerBin(mesh);

    try {
        const auto filepath = fs::path(filename);
        io::filtering_ostream out;
        if (gzip) {
            out.push(io::gzip_compressor(io::gzip_params(io::gzip::default_compression)));
        }
        out.push(io::file_sink(filepath.string()));
        out.write(buf.get(), mesh.bytes());
    } catch (const fs::filesystem_error& ex) {
        LOG(FATAL) << "Error: Failed to write mesh to disk. " << ex.what();
    }
}

#define DO_INSTANTIATE(Real)              \
    template struct TriangleMeshIO<Real>; \
/**/
DO_INSTANTIATE(float)
DO_INSTANTIATE(double)
#undef DO_INSTANTIATE
