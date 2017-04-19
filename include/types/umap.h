// Copyright (c) 2017 The Ustore Authors.

#ifndef USTORE_TYPES_UMAP_H_
#define USTORE_TYPES_UMAP_H_

#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

#include "chunk/chunk.h"
#include "hash/hash.h"
#include "node/cursor.h"
#include "node/node.h"
#include "node/map_node.h"
#include "store/chunk_loader.h"
#include "spec/slice.h"
#include "types/type.h"
#include "types/base.h"
#include "types/iterator.h"
#include "utils/noncopyable.h"

namespace ustore {

class KVIterator : public Iterator {
 public:
  explicit KVIterator(std::unique_ptr<NodeCursor> cursor);

  const Slice key() const;
  const Slice value() const;
};

class UMap : public ChunkableType {
 public:
  explicit UMap(std::shared_ptr<ChunkLoader> loader) :
      ChunkableType(loader) {}
  // construct chunk loader for server
  virtual ~UMap() = default;

  // Use chunk loader to load chunk and read value
  // return empty slice if key not found
  const Slice Get(const Slice& key) const;

  // Return an iterator that scan from map start
  std::unique_ptr<KVIterator> iterator() const;

 protected:
  bool SetNodeForHash(const Hash& hash) override;
};

class SMap : public UMap {
// UMap for server side
 public:
  // Load an existing map using hash
  explicit SMap(const Hash& root_hash,
                std::shared_ptr<ChunkLoader> loader);
  // create an SMap using the kv_items
  // kv_items must be sorted in strict ascending order based on key
  explicit SMap(const std::vector<Slice>& keys,
                const std::vector<Slice>& vals,
                std::shared_ptr<ChunkLoader> loader);

  ~SMap() = default;

  // Both Use chunk builder to do splice
  // this kv_items must be sorted in descending order before
  const Hash Set(const Slice& key, const Slice& val) const;

  const Hash Remove(const Slice& key) const;
};

}  // namespace ustore

#endif  // USTORE_TYPES_UMAP_H_
