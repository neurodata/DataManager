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

#ifndef POLYGONAL_MESH_H
#define POLYGONAL_MESH_H

#include "Mesh.h"
#include "include/Geometry.h"

namespace MeshExtractor_namespace {

template <class Real>
struct IsoVertex {
    int dir, idx[3];
    Point3D<Real> p;
    IsoVertex(Point3D<Real> p, int dir, int x, int y, int z) {
        this->p = p, this->dir = dir, idx[0] = x, idx[1] = y, idx[2] = z;
    }
#define _ABS_(a) ((a) < 0 ? -(a) : (a))
    static bool CoFacial(const IsoVertex& t1, const IsoVertex& t2) {
        int d[] = {_ABS_(t1.idx[0] - t2.idx[0]), _ABS_(t1.idx[1] - t2.idx[1]), _ABS_(t1.idx[2] - t2.idx[2])};
        if (t1.dir == t2.dir)
            return d[t1.dir] == 0 && ((d[(t1.dir + 1) % 3] == 0 && d[(t1.dir + 2) % 3] <= 1) ||
                                      (d[(t1.dir + 2) % 3] == 0 && d[(t1.dir + 1) % 3] <= 1));
        else
            return d[3 - t1.dir - t2.dir] == 0 && d[t1.dir] <= 1 && d[t2.dir] <= 1;
    }
#undef _ABS_
};

template <class Real>
struct PolygonalMesh : public Mesh {
    std::vector<IsoVertex<Real> > vertices;
    std::vector<std::vector<int> > polygons;
};

}  // namespace MeshExtractor_namespace
#endif