// Copyright (c) 2017 The Ustore Authors.

#include "node/cell_node.h"

#include <cstdint>
#include <cstring>  // for memcpy

namespace ustore {

Chunk CellNode::NewChunk(const UType type, const Slice& key, const Slice& data,
    const Hash& preHash1, const Hash& preHash2) {
  // First hash can not be empty
  CHECK(!preHash1.empty());
  // Data size can not be larger than 1<<16
  //CHECK(data.len() < 65536)
  //  << "Data length ("<< data.len() << ") exceeds limit for a cell";
  size_t num_pre_hash = preHash2.empty() ? 1 : 2;
  size_t chunk_len = ComputeTotalLength(num_pre_hash, key.len(), data.len());
  Chunk chunk(ChunkType::kCell, chunk_len);
  // mete fields
  *reinterpret_cast<UType*>(chunk.m_data() + kUTypePos) = type;
  *reinterpret_cast<uint8_t*>(chunk.m_data() + kNumPreHashPos) = num_pre_hash;
  *reinterpret_cast<uint16_t*>(chunk.m_data() + kKeyLengthPos) = key.len();
  *reinterpret_cast<uint16_t*>(chunk.m_data() + kKeyOffsetPos)
    = ComputeKeyOffset(num_pre_hash);
  *reinterpret_cast<int32_t*>(chunk.m_data() + kDataLengthPos) = data.len();
  *reinterpret_cast<int16_t*>(chunk.m_data() + kDataOffsetPos)
    = ComputeDataOffset(num_pre_hash, key.len());
  // pre hash
  std::memcpy(chunk.m_data() + ComputePreHashOffset(0), preHash1.value(),
              Hash::kByteLength);
  if (num_pre_hash == 2)
    std::memcpy(chunk.m_data() + ComputePreHashOffset(1), preHash2.value(),
                Hash::kByteLength);
  // key
  std::memcpy(chunk.m_data() + ComputeKeyOffset(num_pre_hash), key.data(),
              key.len());
  // data
  std::memcpy(chunk.m_data() + ComputeDataOffset(num_pre_hash, key.len()),
              data.data(), data.len());
  return chunk;
}

}  // namespace ustore
