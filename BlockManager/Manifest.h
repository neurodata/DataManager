#ifndef MANIFEST_H
#define MANIFEST_H

#include <array>
#include <memory>
#include <vector>

#include <folly/dynamic.h>
#include <folly/json.h>

namespace BlockManager_namespace {

class BlockManager;

struct Scale {
    Scale(const folly::dynamic& obj);
    Scale() {}

    folly::dynamic toDynamicObj() const;

    ~Scale() {}
    /* String value specifying the subdirectory containing the chunked
     * representation of the volume at this scale. */
    std::string key = "";

    /* 3-element array [x, y, z] of integers specifying the x, y, and z
     * dimensions of the volume in voxels. */
    int size[3] = {0, 0, 0};

    /* 3-element array [x, y, z] of integer values specifying a translation in
     * voxels of the origin of the data relative to the global coordinate frame.
     */
    int voxel_offset[3] = {0, 0, 0};

    /* 3-element array [x, y, z] of numeric values specifying the x, y, and z
     * dimensions of a voxel in nanometers. The x, y, and z "resolution" values
     * must not decrease as the index into the "scales" array increases. */
    double resolution[3] = {0., 0., 0.};

    /* Array of 3-element [x, y, z] arrays of integers specifying the x, y, and
     * z dimensions in voxels of each supported chunk size. Typically just a
     * single chunk size will be specified as [[x, y, z]]. */
    std::vector<std::array<int, 3> > chunk_sizes;

    /*  string value equal (case-insensitively) to the name of one of the
     * supported VolumeChunkEncoding values specified in base.ts. May be one of
     * "raw", "jpeg", or "compressed_segmentation". */
    std::string encoding = "";

    int compressed_segmentation_block_size[3] = {0, 0, 0};
};

class Manifest {
   public:
    Manifest(const std::string& manifest_path_name) {
        Read(manifest_path_name);
    }
    Manifest() {}
    ~Manifest() {}

    void Write(const std::string& filename);
    void Read(const std::string& filename);
    void Print() const;

    static std::shared_ptr<Manifest> MakeExampleManifest();

    // Getters
    std::string type() const { return _type; }
    std::string data_type() const { return _data_type; }
    int num_channels() const { return _num_channels; }
    size_t num_scales() const { return _scales.size(); }
    Scale get_scale(const std::string& key) const {
        for (auto& scale : _scales) {
            if (scale.key == key) {
                return scale;
            }
        }
        CHECK(false) << "Error: Failed to find scale with key " << key;
    }
    std::string mesh() const { return _mesh; }

    // Setters
    // TODO(adb): consider validating type and data_type members on set
    void set_type(const std::string& type) { _type = type; }
    void set_data_type(const std::string& data_type) { _data_type = data_type; }
    void set_num_channels(int num_channels) { _num_channels = num_channels; }
    // TODO(adb): set scales
    void set_mesh(const std::string& mesh) { _mesh = mesh; }

   private:
    /* One of "image" or "segmentation", specifying the type of the volume. */
    std::string _type;

    /* A string value equal (case-insensitively) to the name of one of the
     * supported DataType values specified in data_type.ts. May be one of
     * "uint8", "uint16", "uint32", "uint64", or "float32". "float32" should
     * only be specified for "image" volumes. */
    std::string _data_type;

    /* An integer value specifying the number of channels in the volume. Must be
     * 1 for "segmentation" volumes. */
    int _num_channels;

    /* Array specifying information about the supported resolutions
     * (downsampling scales) of the volume.  */
    std::vector<Scale> _scales;

    /* May be optionally specified if "volume_type" is "segmentation". If
     * specified, it must be a string value specifying the name of the
     * subdirectory containing the mesh data. */
    std::string _mesh = "";
    friend BlockManager;
};

}  // namespace BlockManager_namespace

#endif