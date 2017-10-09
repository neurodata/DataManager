# NeuroDataManager Client Example

The following example illustrates the directory structure for Neuroglancer precomputed data as well as how to use the `ndm` program in both **Ingest** and **Cutout** modes.

### Neuroglancer Precomputed Directory Structure

Neuroglancer uses the following directory format for precomputed data sources:

```
    MyDataDir/
        info
        0
            0-512_0-512_0-16
            0-512_512-1014_016
            ...
        1
            0-512_0-512_0-16
            ...
```

The `info` file is a json file (with no file suffix) that provides information about the dimensionality of the data stored in each scale directory. In `NeuroDataManager` we call the `info` file the *Neuroglancer JSON Manifest*, or just the manifest. The directories below the `info` file are scale directories, each corresponding to a given scaling of the dataset. The directory names match the scale keys defined in the `info` file. 

### Neuroglancer JSON Manifest

For the example directory above, the `info` file might look something like the following:

```json
{
    "scales": [
        {
            "key": "0",
            "size": [
                8192,
                8192,
                194
            ],
            "voxel_offset": [
                0,
                0,
                0
            ],
            "resolution": [
                2.0,
                2.0,
                50.0
            ],
            "chunk_sizes": [[512, 512, 16]],
            "encoding": "compressed_segmentation",
            "compressed_segmentation_block_size": [8, 8, 8]
        },
        {
            "key": "1",
            "size": [
                4096,
                4096,
                194
            ],
            "voxel_offset": [
                0,
                0,
                0
            ],
            "resolution": [
                4.0,
                4.0,
                50.0
            ],
            "chunk_sizes": [[512, 512, 16]],
            "encoding": "compressed_segmentation",
            "compressed_segmentation_block_size": [8, 8, 8]
        }
    ],
    "num_channels": 1,
    "data_type": "uint32",
    "type": "segmentation"
}
```

A complete description of the Neuroglancer JSON Manifest format is available at the [Neuroglancer Github Repository](https://github.com/google/neuroglancer/blob/master/src/neuroglancer/datasource/precomputed/README.md). We will just note that the file above describes a segmentation dataset that is 8192 by 8192 by 194 pixels at base resolution, is downsampled once, and is stored in the [Neuroglancer compressed segmentation format](https://github.com/google/neuroglancer/blob/master/src/neuroglancer/sliceview/compressed_segmentation/README.md). 

### Data Ingest

Support we have a file containing annotations for slices 0 to 16 in the dataset above. We can use NeuroDataManager to load the file into the precomputed data directory using the following command:

`bin/ndm -datadir /path/to/MyDataDir/ -input MyInputFile.tif -x 8192 -y 8192 -z 16 -scale 0 -gzip`

Note that since the bounding box of our input file starts at `(0,0,0)` we don't need to supply any offsets. For slices 16 to 32 we might do something like the following:

`bin/ndm -datadir /path/to/MyDataDir/ -input MyInputFile.tif -x 8192 -y 8192 -z 16 -zoffset 16 -scale 0 -gzip`

The `zoffset` flag indicates that the bounding box of the input file starts at `(0,0,16)`. For more information about the NeuroDataManager coordinate system, see **Coordinates.md**. 

Flag information is available in the **NDM.md** reference guide. However, we will briefly point out that the above command is loading data in Neuroglancer compressed semgnetation format (file format is dictated by the manifest) at scale `0` and using `gzip` to compress the Neuroglancer precomputed blocks (on top of the compression we get from the Neuroglancer compressed segmentation format).

### Loading a Spatial Cutout 

After ingesting, suppose we want to load a small region of data, possibly as part of a processing pipeline. After running the following command...

`bin/ndm -datadir /path/to/MyDataDir/ -output MyOutputFile.tif -x 512 -y 512 -z 16 -xoffset 1024 -yoffset 1024 -zoffset 16 -scale 0 -gzip`

... we will end up with a 512 by 512 by 16 tif file containing data from the following bounding box:

`(1024, 1024, 16)` to `(1536, 1536, ,32)`

The output file will be a `uint32` multi-page tif that you can open with `PILLOW` in Python or using Fiji/ImageJ.
