#ifndef BLOCK_MANAGER_H
#define BLOCK_MANAGER_H 

#include "Manifest.h"
#include "Blocks/Block.h"
#include "Blocks/AnnoBlock.h"

#include "../DataArray/DataArray.h"

#include <glog/logging.h>
#include <boost/filesystem.hpp>

#include <map>
#include <unordered_map>
#include <memory>
#include <vector>

namespace BlockManager_namespace {
namespace fs = boost::filesystem;
    
struct BlockKey {
    uint64_t morton_index;
    int x;
    int y;
    int z;
    bool operator<(const BlockKey& other) const {
        return (morton_index < other.morton_index);
    }
};

typedef std::map< BlockKey, std::shared_ptr<Block> > BlockMortonIndexMap;

class BlockManager {

public:
    BlockManager(const std::string& directory_path_name, std::shared_ptr<Manifest> manifestShPtr);
    ~BlockManager();

    template< typename T >
    void Put(const DataArray_namespace::DataArray3D< T >& data, const std::array<int, 2>& xrng, const std::array<int, 2>& yrng, const std::array<int, 2>& zrng, const std::string& scale_key) {
        const auto chunk_size = getChunkSizeForScale(scale_key);

        auto block_keys = _blocksForBoundingBox(xrng, yrng, zrng, scale_key);
        auto blockMortonIndexMapItr = block_index_by_res.find(scale_key);
        CHECK(blockMortonIndexMapItr != block_index_by_res.end()) << "Failed to find scale key " << scale_key << " in block map.";        
        auto blockMortonIndexMap = blockMortonIndexMapItr->second;
        for(const auto& block_key : block_keys) {
            std::array<int, 2> _x({{(block_key.x * chunk_size[0]) - xrng[0], ((block_key.x + 1) * chunk_size[0]) - xrng[0]}});
            std::array<int, 2> _y({{(block_key.y * chunk_size[1]) - yrng[0], ((block_key.y + 1) * chunk_size[1]) - yrng[0]}});
            std::array<int, 2> _z({{(block_key.z * chunk_size[2]) - zrng[0], ((block_key.z + 1) * chunk_size[2]) - zrng[0]}});

            // TODO(adb): if the input request isn't cube aligned, we need to modify starting index of the slice 
            // and ensure that we use the dimension of the slice in any addition loop in the DataArray3D object

            const auto arr_view = data.view(_x, _y, _z);

            auto itr = blockMortonIndexMap->find(block_key);
            std::shared_ptr<AnnoBlock32> blockShPtr;
            if (itr == blockMortonIndexMap->end()) {
                // Create a new block and add it to the map
                // TODO(adb): pick the correct block type
                std::string block_name = Block::SetNeuroglancerFileName(static_cast<int>(block_key.x * chunk_size[0]), static_cast<int>((block_key.x + 1) * chunk_size[0]), static_cast<int>(block_key.y * chunk_size[1]), static_cast<int>((block_key.y + 1) * chunk_size[1]), static_cast<int>(block_key.z * chunk_size[2]), static_cast<int>((block_key.z + 1) * chunk_size[2]));
                const auto block_path = fs::path(directory_path_name) / fs::path(scale_key) / fs::path(block_name);
                std::string block_path_name = block_path.string();
                blockShPtr = std::make_shared<AnnoBlock32>(block_path_name, chunk_size[0], chunk_size[1], chunk_size[2], sizeof(uint32_t));
                blockMortonIndexMap->insert(std::make_pair(block_key, blockShPtr));
            } else {
                blockShPtr = std::dynamic_pointer_cast<AnnoBlock32>(itr->second);
            }
            // TODO(adb): Just add in this function. No need to drop in the arr view and create such a mess when we can just use the DataArray operator[]
            blockShPtr->add<uint32_t>(arr_view);
        }
        return;
    }

    template< typename T >
    void Get(DataArray_namespace::DataArray3D< T >& data, const std::array<int, 2>& xrng, const std::array<int, 2>& yrng, const std::array<int, 2>& zrng, const std::string& scale_key) const {
        return;
    }

    std::array<int, 3> getChunkSizeForScale(const std::string& scale_key);
    
protected:
    std::vector< BlockKey > _blocksForBoundingBox(const std::array<int, 2>& xrng, const std::array<int, 2>& yrng, const std::array<int, 2>& zrng, const std::string& scale_key);    
    void _init();
    // void _flush(); TODO(adb): automatically flushed on destruction, but maybe we want to implement this eventually
    std::shared_ptr<BlockMortonIndexMap> _createIndexForScale(const std::string& scale);
    
    std::string directory_path_name;
    std::unordered_map< std::string, std::shared_ptr<BlockMortonIndexMap> > block_index_by_res;
    std::shared_ptr<Manifest> manifest;

};

}

#endif // BLOCK_MANAGER_H 