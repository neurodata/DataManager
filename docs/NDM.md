# NeuroDataManager Reference

The NeuroDataManager client program (`ndm`) allows one to read and write data to a Neuroglancer precomputed chunk format datastore. 

### Modes of Operation

The `ndm` client program has two primary modes of operation:

1. **Ingest**: Given an input data file, extract the appropriate region from the precomputed data store and add the values in the input file into that region. By default, all values are added. However, an overwrite interface is available that allows overwriting existing values on a per-block basis. The overwrite interface is expected to be exposed in a future release.

2. **Cutout**: Given an `(x,y,z)` bounding box, extract a region of data from the precomputed data store and save the region locally in an user-specified output format.

### Program Reference

All commands are prefixed with a single dash (`-`). Below is a listing of the `ndm` program options as of version 0.2. 

* `help` : List these options.

* `version` : Obtain the current NeuroDataManager version and build date.
* `datadir` : The path to the directory containing a Neuroglancer JSON manifest. Currently, only directories on the local filesystem (filesystem datastore) are supported.
* `exampleManifest` : Generate an example Neuroglancer manifest to use as a template for setting up a new data directory. Can be supplied with no other arguments. Will generate the manifest and exit. The example manifest will be written to `manifest.ex.json` in the calling directory. 
* `format` : Input/output file format. Currently `tif` is default and is the only format supported.
* `gzip` : Indicates the precomputed chunk data in the data directory is compressed using gzip. If you are attempting to read data from the data directory and are getting errors loading precompued chunks, the data is likely compressed with gzip.
* `input` : Path to the input file for Ingest. Passing this flag indicates `ndm` should run in ingest mode. Only one operation can be run at a time, and Ingest takes priority over Cutout (if both flags are passed). 
* `output` : Path to the output file for Cutout. 
* `scale` : String indicating the scale key to use for this ingest/cutout operation. Must match the scale key defined in the Neuroglancer JSON manifest.
* `subtractVoxelOffset` : If false, provided coordinates do not include the global voxel offset of the dataset (e.g. are 0-indexed with respect to the data on disk). If true, the voxel offset is subtracted from the cutout arguments in a pre-processing step. For more information, see **Coordinates.md**.
* `x` : The x-dimension of the input/output file.
* `xoffset` : The x-dimension of the offset into the data of the input/output file.
* `y` : The y-dimension of the input/output file.
* `yoffset`: The y-dimension of the offset into the data of the input/output file.
* `z` : The z-dimension of the input/output file.
* `zoffset` : The z-dimension of the offset into the data of the input/output file.

### Logging 

Note that NDM includes logging curtousy of the Google Logging library ([glog](https://github.com/google/glog)). Logging can be enabled many ways, but the following are typically the most useful:

1. Run `export GLOG_logtostderr=1` in your shell before running the `ndm` executable. As of version 0.2 this is done by default in `ndm` docker containers.

2. Prefix the `ndm` command with `GLOG_logtostderr=1`. E.g.:
   
   `GLOG_logtostderr=1 bin/ndm ...` 