// Copyright (c) 2017 The Ustore Authors.

#ifndef USTORE_CHUNK_CHUNKER_H_
#define USTORE_CHUNK_CHUNKER_H_

#include <vector>

#include "chunk/chunk.h"
#include "chunk/segment.h"

namespace ustore {

struct ChunkInfo {
  const Chunk* chunk;
  // a Segment that holding a single MetaEntry bytes
  const Segment* meta_seg;
};

class Chunker {
  // An interface to make chunk from multiple segments.
  //   Each type, e.g, Blob, MetaNode shall have one.
 public:
  virtual const ChunkInfo make(const std::vector<const Segment*>& segments)
      const = 0;
};
}  // namespace ustore
#endif  // USTORE_CHUNK_CHUNKER_H_