// Copyright (c) 2017 The Ustore Authors.

#ifndef USTORE_TYPES_ULIST_H_
#define USTORE_TYPES_ULIST_H_

#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

#include "chunk/chunk.h"
#include "hash/hash.h"
#include "node/node.h"
#include "store/chunk_loader.h"
#include "types/base.h"
#include "types/type.h"
#include "types/iterator.h"
#include "utils/noncopyable.h"

namespace ustore {

class ListIterator : public Iterator {
 public:
  explicit ListIterator(std::unique_ptr<NodeCursor> cursor) :
      Iterator(std::move(cursor)) {}

  const Slice entry() const {
    // Exclude the first four bytes that encode entry len
    size_t len = cursor_->numCurrentBytes() - sizeof(uint32_t);
    auto data = reinterpret_cast<const char*>(cursor_->current()
                                              // Skip the first 4 bytes
                                              + sizeof(uint32_t));
    return Slice(data, len);
  }
};

class UList : public ChunkableType {
 public:
  // create an empty map
  // construct chunk loader for server
  explicit UList(std::shared_ptr<ChunkLoader> loader) :
      ChunkableType(loader) {}
  // construct chunk loader for server
  virtual ~UList() = default;

  // For idx > total # of elements
  //    return empty slice
  const Slice Get(size_t idx) const;

  // Return an iterator that scan from map start
  inline std::unique_ptr<ListIterator> iterator() const {
    CHECK(!empty());
    std::unique_ptr<NodeCursor> cursor(
        NodeCursor::GetCursorByIndex(root_node_->hash(),
                                     0, chunk_loader_.get()));

    return std::unique_ptr<ListIterator>(new ListIterator(std::move(cursor)));
  }

 protected:
  bool SetNodeForHash(const Hash& hash) override;

  // friend vector<size_t> diff(const UList& lhs, const UList& rhs);

  // friend vector<size_t> intersect(const UList& lhs, const UList& rhs);
};

class SList : public UList {
// UMap for server side
 public:
  // Load an existing map using hash
  SList(const Hash& root_hash, std::shared_ptr<ChunkLoader> loader);

  // create an SList using the initial elements
  SList(const std::vector<Slice>& elements,
        std::shared_ptr<ChunkLoader> loader);

  // create an empty map
  // construct chunk loader for server
  ~SList() = default;

  // entry vector can be empty
  const Hash Splice(size_t start_idx, size_t num_to_delete,
                    const std::vector<Slice>& entries) const;
};
}  // namespace ustore

#endif  //  USTORE_TYPES_ULIST_H_
