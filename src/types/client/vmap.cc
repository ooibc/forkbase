// Copyright (c) 2017 The Ustore Authors.

#include "types/client/vmap.h"

namespace ustore {

VMap::VMap(const std::vector<Slice>& keys, const std::vector<Slice>& vals)
    noexcept {
  buffer_ = {UType::kMap, Hash::kNull, 0, 0, vals, keys};
}

VMap::VMap(std::shared_ptr<ChunkLoader> loader, const Hash& root_hash)
    noexcept : UMap(loader) {
  SetNodeForHash(root_hash);
}

Hash VMap::Set(const Slice& key, const Slice& val) const {
  buffer_ = {UType::kMap, root_node_->hash(), 0, 0, {val}, {key}};
  return Hash::kNull;
}

Hash VMap::Remove(const Slice& key) const {
  buffer_ = {UType::kMap, root_node_->hash(), 0, 1, {}, {key}};
  return Hash::kNull;
}

}  // namespace ustore
