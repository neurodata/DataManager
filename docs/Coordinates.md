# Coordinate Systems

Spatial neuroscience datasets typically use multiple coordinate systems. A voxel based coordinate system describes the number of voxels (pixels) in the image data, while a physical coordinate system (e.g. nanometers) relates the images in the database to the size of the real world specimen that was imaged. Physical coordinates are related to voxel coordinates by the `resolution` parameter, an `(x,y,z)` tuple that gives the size of each voxel in physical coordinates. [Neuroglancer](https://github.com/google/neuroglancer) uses the `resolution` parameter to appropriately scale the image data and allow data from multiple systems to be overlaid in one universal coordinate space.

## Voxel Coordinates

The voxel coordinate system is in fact comprised of two related coordinate systems; the image size space and the global voxel space. On disk, images are stored in the image size space with the bottom left corner translated to the origin, and the top right corner taking the `(x,y,z)` position corresponding to the size of the dataset. In other words, the image size is simply the size of the dataset. However, it is often convenient to attribute an offset parameter to one or more axis of the dataset -- for example, data may be a subset of a larger dataset or specimen. Or, data may need to be used in a program that has a different method of indexing (e.g. Matlab). The `voxel_offset` parameter supports an arbitrary origin for the bottom left corner and defines the global voxel space.

### Ingest

Most `NeuroDataManager` utilities support both image size coordinates and global voxel coordinates. The `ingest` utility provides a flag allowing the user to switch between the two spaces:
```
-subtractVoxelOffset (If false, provided coordinates do not include the
      global voxel offset of the dataset (e.g. are 0-indexed with respect to
      the data on disk). If true, the voxel offset is subtracted from the
      cutout arguments in a pre-processing step.) type: bool default: false
```

The flag is best explained with the following examples. Assume the dataset is `1024` by `1024` by `1024` (in `(x,y,z)`) and suppose you have a single `z`-slice through a dataset and want to ingest the slice. If the slice starts at `z=44` and is only one voxel thick, and we wanted to use image size coordinates, the following comamand would be used:
```
bin/ingest -ingestdir data_dir_path/ -input test_slice.tif -x 1024 -y 1024 -z 1 -zoffset 44
```

Now, if `z=44` is instead in the global voxel coordinate space, we simply pass in the `subtractVoxelOffset` flag as follows:
```
bin/ingest -ingestdir data_dir_path/ -input test_slice.tif -x 1024 -y 1024 -z 1 -zoffset 44 -subtractVoxelOffset
```

Note that the user does not have to specify the global voxel offset -- *NeuroDataManager* will calculate the global voxel offset automatically based on the manifest file in the ingest directory. 


#### Command Line Offset vs Voxel Offset

The `ingest` utility command line offsets provide an offset of the input `tif` file into the dataset. Note that the offsets default to `0`. Therefore, if you pass in the `subtractVoxelOffset` flag, you must also specify an offset for `x` and `y` if the voxel offset is other than `0`. For example, if our dataset above were offset at `(0,512,16)` we might pass in the following:
```
bin/ingest -ingestdir data_dir_path/ -input test_slice.tif -x 1024 -xoffset 0 -y 1024 -yoffset 512 -z 1 -zoffset 44 -subtractVoxelOffset
```
which would be equivalent to
```
bin/ingest -ingestdir data_dir_path/ -input test_slice.tif -x 1024 -xoffset 0 -y 1024 -yoffset 0 -z 1 -zoffset 28
```