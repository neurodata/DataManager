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

#ifndef EXTRACT_NEUROGLANCER_MESH_H
#define EXTRACT_NEUROGLANCER_MESH_H

#ifdef __cplusplus
extern "C" {
#endif

extern void meshExtractedCallbackGo(char* filename, int id, int chunkID);

void ConvertArray64to32Bit(void* input, void* output, int length);
void ExtractNeuroglancerMeshFromChunk(void* input, char* file_path_prefix, int xdim, int ydim, int zdim, int xoffset,
                                      int yoffset, int zoffset, float xres, float yres, float zres, int chunkID,
                                      int* num_meshes);
#ifdef __cplusplus
}
#endif

#endif  // NEUROGLANCER_MESH_H