// Copyright (c) 2017 The Ustore Authors.

#ifndef USTORE_NODE_NODE_BUILDER_H_
#define USTORE_NODE_NODE_BUILDER_H_

#include <list>
#include <memory>
#include <vector>

#include "chunk/chunker.h"
#include "chunk/chunk_loader.h"
#include "chunk/chunk_writer.h"
#include "node/cursor.h"
#include "node/rolling_hash.h"
#include "types/type.h"
#include "utils/noncopyable.h"

namespace ustore {
class NodeBuilder : private Noncopyable {
 public:
  // Perform operation at element with key at leaf rooted at root_hash
  NodeBuilder(const Hash& root_hash, const OrderedKey& key,
              ChunkLoader* chunk_loader, ChunkWriter* chunk_writer,
              const Chunker* chunker, const Chunker* parent_chunker,
              bool isFixedEntryLen) noexcept;

  // Perform operation at idx-th element at leaf rooted at root_hash
  NodeBuilder(const Hash& root_hash, size_t idx,
              ChunkLoader* chunk_loader, ChunkWriter* chunk_writer,
              const Chunker* chunker, const Chunker* parent_chunker,
              bool isFixedEntryLen) noexcept;

  // Construct a node builder to construct a fresh new Prolly Tree
  NodeBuilder(ChunkWriter* chunk_writer, const Chunker* chunker,
              const Chunker* parent_chunker,
              bool isFixedEntryLen) noexcept;

  ~NodeBuilder() = default;

  // First delete num_delete elements from cursor and then
  // Append elements in Segment in byte array
  void SpliceElements(size_t num_delete, const Segment* element_seg);

  // First delete num_delete elements from cursor and then
  // Append elements in Segments in byte array
  void SpliceElements(size_t num_delete,
                      std::vector<const Segment*> element_segs);

  // Commit the uncommited operation
  // Create and dump the chunk into storage
  // @return The hash (a.k.a. the key) of the newly commited root chunk.
  inline Hash Commit() {
    bool found_canonical_root;
    return Commit(&found_canonical_root);
  }

 private:
  // Internal constructor used to recursively construct Parent NodeBuilder
  // is_leaf shall set to FALSE
  NodeBuilder(std::unique_ptr<NodeCursor>&& cursor,
              size_t level, ChunkWriter* chunk_writer,
              const Chunker* chunker, const Chunker* parent_chunker,
              bool isFixedEntryLen) noexcept;

  NodeBuilder(size_t level, ChunkWriter* chunk_writer,
              const Chunker* chunker, const Chunker* parent_chunker,
              bool isFixedEntryLen) noexcept;

  // Commit the uncommited operation
  // Create and dump the chunk into storage
  // @return The hash (a.k.a. the key) of the newly commited root chunk.
  // Return from from argument whether the upper builder has found the canonical root
  Hash Commit(bool* found_canonical_root);

  // Remove elements from cursor
  // Return the number of elements actually removed
  size_t SkipEntries(size_t num_elements);

  // Append entries in a segment
  void AppendSegmentEntries(const Segment* entry_seg);

  // Make chunks based on the entries in the segments
  // Pass the created metaentry(a segment with a single entry)
  //   to upper builders to append
  // reset the rolling hasher
  // return the created chunk
  Chunk HandleBoundary(const std::vector<const Segment*>& segments);
  // Two things to do:
  //  * Populate the rolling hash with preceding elements before cursor point
  //      until its window size filled up
  //  * Populate the buffer with data from SeqNode head until cursor point
  //  NOTE: Be sure NOT to rolling hash num_entries bytes at MetaNode head
  void Resume();

  // Whether a node builder will build a single-entry MetaNode
  //   This node is invalid and shall be excluded from final built tree
  inline bool isInvalidNode() const {
    return cursor_ == nullptr && numAppendSegs() <= 1;
  }

  // Create an empty segment pointing data from current cursor
  //   This created segs are pushed to created_segs of this nodebuilder
  Segment* SegAtCursor();

  inline size_t numAppendSegs() const { return appended_segs_.size(); }
  // Access the parent builder.
  // Construct a new one, if not exists.
  NodeBuilder* parent_builder();

 private:
  std::unique_ptr<NodeCursor> cursor_;
  std::unique_ptr<NodeBuilder> parent_builder_;
  // a vector of appended segments for chunking
  std::vector<const Segment*> appended_segs_;
  // A vector to collect and own segs created by this nodebuilder
  std::vector<std::unique_ptr<const Segment>> created_segs_;

  Segment* pre_cursor_seg_;
  std::unique_ptr<RollingHasher> rhasher_;
  bool commited_ = true;    // false if exists operation to commit
  size_t num_skip_entries_ = 0;
  size_t level_ = 0;
  ChunkWriter* const chunk_writer_;
  const Chunker* const chunker_;
  const Chunker* const parent_chunker_;
  // whether the built entry is fixed length
  // type blob: true
  const bool isFixedEntryLen_;
  size_t num_created_entries_ = 0;
};


class AdvancedNodeBuilder : Noncopyable  {
/* A node builder that can support multiple operation in a single transaction

AdvancedNodeBuilder applies NodeBuilder for multiple rounds for edition. However,
it buffers the created chunks in the middle and provides access to these intermidiate chunks.
At last, it dumps all the created chunks that only occur in the final prolly tree
to Chunk Storage. Useless intermediate chunks will be discarded.

NOTE: AdvancedNodeBuilder utilizes the NodeBuilder property that any chunks read by
a NodeBuilder during commit will NOT be included in the built prolly tree from this NodeBuilder.

In this way, any intermediate created chunks that are read by NodeBuilder
in later around will NOT be dumped to final storage.

To construct a new prolly tree:
  AdvancedNodeBuilder nb(writer);
  const Hash hash = nb.Insert(0, segment).Commit();

To work on an existing prolly tree:
  AdvancedNodeBuilder nb(root, loader, writer);
  const Hash hash = nb.Insert(0, segment)
                      .Splice(1, 4, segment)
                      .Remove(4, 6)
                      .Commit();
*/
 public:
  AdvancedNodeBuilder(const Hash& root, ChunkLoader* loader_,
                      ChunkWriter* writer);

  // ctor to create a prolly tree from start
  explicit AdvancedNodeBuilder(ChunkWriter* writer);

  inline AdvancedNodeBuilder& Insert(uint64_t start_idx,
                                     const std::vector<const Segment*>& segs) {
    return Splice(start_idx, 0, segs);
  }

  inline AdvancedNodeBuilder& Insert(uint64_t start_idx,
                                     const Segment& seg) {
    return Splice(start_idx, 0, {&seg});
  }

  inline AdvancedNodeBuilder& Remove(uint64_t start_idx,
                                     uint64_t num_delete) {
    return Splice(start_idx, num_delete, {});
  }

  // segs can be empty.
  // NOTE: The segments in the argument list should be alive
  //   until commit is called.
  AdvancedNodeBuilder& Splice(uint64_t start_idx, uint64_t num_delete,
                              const std::vector<const Segment*>& segs);

  AdvancedNodeBuilder& Splice(uint64_t start_idx, uint64_t num_delete,
                              const Segment& seg) {
    return  Splice(start_idx, num_delete, {&seg});
  }

  Hash Commit(const Chunker& chunker, bool isFixedEntryLen);

 private:
  class PersistentChunker : public Chunker {
  /* PersistentChunker is a wrapper for a normal chunker.

  It persists the chunk internal bytes while
    the created chunks points to these bytes.
  */
   public:
    explicit PersistentChunker(const Chunker* chunker) noexcept :
      chunker_(chunker) {}

    inline ChunkInfo Make(const std::vector<const Segment*>& segments) const
        override {
      ChunkInfo chunk_info = chunker_->Make(segments);
      created_chunks_.push_back(std::move(chunk_info.chunk));

      Chunk new_chunk(created_chunks_.rbegin()->head());

      return {std::move(new_chunk),
              std::move(chunk_info.meta_seg)};
    }

    ~PersistentChunker() = default;

   private:
    const Chunker* chunker_;
    // created_chunks_ own the actual data.
    mutable std::list<Chunk> created_chunks_;
  };

  class ChunkCacher : public ChunkLoader, public ChunkWriter {
  /*
  ChunkCacher allows to cache the written chunks locally and provide priority access
  to them if loaded.
  */

   public:
    ChunkCacher(ChunkLoader* loader, ChunkWriter* writer) noexcept :
      loader_(loader), writer_(writer) {}

    inline bool Write(const Hash& key, const Chunk& chunk) override {
      cache_[key.Clone()] = chunk.head();
      has_read_[key.Clone()] = false;

      return true;
    }

    // Delete the cached data
    ~ChunkCacher() = default;

    // Return true if all unread cached chunks are dumpted to ChunkWriter
    bool DumpUnreadCacheChunks();
   protected:
    // First try to find chunk from the cache
    // If not found, find from ChunkLoader
    Chunk GetChunk(const Hash& key) override;

   private:
    ChunkLoader* loader_;
    ChunkWriter* writer_;
    std::map<Hash, const byte_t*> cache_;
    std::map<Hash, bool> has_read_;
  };
  // relevant information to perform one spliced operation
  struct SpliceOperand {
    uint64_t start_idx;
    // the number of entries to delete
    size_t num_delete;
    // a vector of segments to append
    // can be empty
    std::vector<const Segment*> appended_segs;
  };

  const Hash root_;
  ChunkLoader* loader_;
  ChunkWriter* writer_;
  std::list<SpliceOperand> operands_;
};
}  // namespace ustore

#endif  // USTORE_NODE_NODE_BUILDER_H_
