#include "Block.h"

#include <third_party/CompressedSegmentation/compress_segmentation.h>
#include <third_party/CompressedSegmentation/decompress_segmentation.h>

using namespace BlockManager_namespace;

Block::Block(const std::string& path_name, int xdim, int ydim, int zdim, size_t dtype_size, BlockEncoding encoding,
             BlockDataType data_type, const std::shared_ptr<BlockSettings>& blockSettingsPtr)
    : _path_name(path_name),
      _blockSettingsPtr(blockSettingsPtr),
      _xdim(xdim),
      _ydim(ydim),
      _zdim(zdim),
      _dtype_size(dtype_size),
      _encoding(encoding),
      _data_type(data_type) {
    _allocate();
}

Block::~Block() { _flush(); }

void Block::zero_block() {
    size_t arr_size = _xdim * _ydim * _zdim * _dtype_size;
    std::memset(_data.get(), 0, arr_size);
    _data_loaded = true;
    _dirty = true;
}

std::string Block::SetNeuroglancerFileName(int xstart, int xend, int ystart, int yend, int zstart, int zend,
                                           const std::array<int, 3>& voxel_offset) {
    // neuroglancer files are written into the global coordinate space,
    // whereas block coordinates use the data bounding box coordinate space,
    // so we need to add the voxel_offset to the filename.
    const auto xstart_str = std::to_string(xstart + voxel_offset[0]);
    const auto xend_str = std::to_string(xend + voxel_offset[0]);
    const auto ystart_str = std::to_string(ystart + voxel_offset[1]);
    const auto yend_str = std::to_string(yend + voxel_offset[1]);
    const auto zstart_str = std::to_string(zstart + voxel_offset[2]);
    const auto zend_str = std::to_string(zend + voxel_offset[2]);

    return xstart_str + "-" + xend_str + "_" + ystart_str + "-" + yend_str + "_" + zstart_str + "-" + zend_str;
}

void Block::_allocate() {
    size_t arr_size = _xdim * _ydim * _zdim * _dtype_size;
    // Allocate and zero an empty data buffer
    _data = std::unique_ptr<char[]>(new char[arr_size]);
}

void Block::_flush() {
    if (is_dirty()) {
        save();
    }
    _data.reset();
    _data_loaded = false;
    _dirty = false;
}

SerializedBlockOutput Block::_serializeByEncoding() {
    switch (_encoding) {
        case BlockEncoding::RAW: {
            return _toRaw();
        } break;
        case BlockEncoding::COMPRESSED_SEGMENTATION: {
            return _toCompressedSegmentation();
        } break;
        case BlockEncoding::JPEG: {
            return _toJpeg();
        } break;
        default: { LOG(FATAL) << "Unable to parse block encoding"; }
    }
}

void Block::_loadSerializedDataByEncoding(std::unique_ptr<char[]> buf) {
    switch (_encoding) {
        case BlockEncoding::RAW: {
            _fromRaw(std::move(buf));
            return;
        } break;
        case BlockEncoding::COMPRESSED_SEGMENTATION: {
            _fromCompressedSegmentation(std::move(buf));
            return;
        } break;
        case BlockEncoding::JPEG: {
            _fromJpeg(std::move(buf));
            return;
        } break;
        default: { LOG(FATAL) << "Unable to parse block encoding"; }
    }
}

SerializedBlockOutput Block::_toCompressedSegmentation() {
    // The last element in each vector corresponds to the number of channels (1)
    // in each dataset
    const ptrdiff_t input_strides[4] = {_ydim * _zdim, _zdim, 1, 1};
    const ptrdiff_t volume_size[4] = {_xdim, _ydim, _zdim, 1};
    const ptrdiff_t block_size[3] = {8, 8, 8};

    std::vector<uint32_t> output_vector;
    if (_data_type == BlockDataType::UINT32) {
        neuroglancer::compress_segmentation::CompressChannels<uint32_t>(
            reinterpret_cast<uint32_t*>(_data.get()), input_strides, volume_size, block_size, &output_vector);
    } else if (_data_type == BlockDataType::UINT64) {
        neuroglancer::compress_segmentation::CompressChannels<uint64_t>(
            reinterpret_cast<uint64_t*>(_data.get()), input_strides, volume_size, block_size, &output_vector);
    } else {
        LOG(FATAL) << "Unable to serialize data type to compressed segmentation. Data type must be UINT32 or UINT64.";
    }
    auto _output = std::unique_ptr<char[]>(new char[output_vector.size() * sizeof(uint32_t)]);
    std::memcpy(_output.get(), &output_vector[0], output_vector.size() * sizeof(uint32_t));
    return {std::move(_output), output_vector.size() * sizeof(uint32_t)};
}

void Block::_fromCompressedSegmentation(std::unique_ptr<char[]> input) {
    const ptrdiff_t volume_size[4] = {_xdim, _ydim, _zdim, 1};
    const ptrdiff_t block_size[3] = {8, 8, 8};

    std::vector<uint32_t> output_vec;

    neuroglancer::compress_segmentation::DecompressChannels<uint32_t>(reinterpret_cast<uint32_t*>(input.get()),
                                                                      volume_size, block_size, &output_vec);

    CHECK_EQ(output_vec.size(), _xdim * _ydim * _zdim);

    // Decompress Channels returns a 3D array in xyz order. We need the array to be in zyx order.
    uint32_t* _data_ptr = reinterpret_cast<uint32_t*>(_data.get());
    for (int x = 0; x < _xdim; x++) {
        for (int y = 0; y < _ydim; y++) {
            for (int z = 0; z < _zdim; z++) {
                _data_ptr[z + (_zdim * y) + (_zdim * _ydim * x)] = output_vec[(z * _xdim * _ydim) + (y * _xdim) + x];
            }
        }
    }
}

SerializedBlockOutput Block::_toRaw() {
    // Convert data to Fortran order
    DataArray_namespace::DataArray3D<uint32_t> local_arr(_data, _xdim, _ydim, _zdim);

    DataArray_namespace::DataArray3D<uint32_t> fortran_arr(_xdim, _ydim, _zdim, boost::fortran_storage_order());

    for (int x = 0; x < _xdim; x++) {
        for (int y = 0; y < _ydim; y++) {
            for (int z = 0; z < _zdim; z++) {
                fortran_arr(x, y, z) = local_arr(x, y, z);
            }
        }
    }

    size_t arr_size = _xdim * _ydim * _zdim * sizeof(uint32_t);
    std::unique_ptr<char[]> _tmp_data(new char[arr_size]);
    fortran_arr.copy(_tmp_data, _xdim, _ydim, _zdim);
    return {std::move(_tmp_data), arr_size};
}

void Block::_fromRaw(std::unique_ptr<char[]> input) {
    DataArray_namespace::DataArray3D<uint32_t> fortran_arr(input, _xdim, _ydim, _zdim, boost::fortran_storage_order());

    DataArray_namespace::DataArray3D<uint32_t> local_arr(_xdim, _ydim, _zdim);

    for (int x = 0; x < _xdim; x++) {
        for (int y = 0; y < _ydim; y++) {
            for (int z = 0; z < _zdim; z++) {
                local_arr(x, y, z) = fortran_arr(x, y, z);
            }
        }
    }
    local_arr.copy(_data, _xdim, _ydim, _zdim);
}

SerializedBlockOutput Block::_toJpeg() { return SerializedBlockOutput({nullptr, 0}); }

void Block::_fromJpeg(std::unique_ptr<char[]> input) { LOG(FATAL) << "Error: reading jpeg blocks not implemented."; }
