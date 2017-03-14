// Copyright (c) 2017 The Ustore Authors.

#ifndef USTORE_TYPES_UBLOB_H_
#define USTORE_TYPES_UBLOB_H_

#include <cstddef>
#include <memory>

#include "chunk/chunk.h"
#include "hash/hash.h"
#include "node/chunk_loader.h"
#include "node/node.h"
#include "types/type.h"
#include "utils/noncopyable.h"

namespace ustore {
class UBlob : private Noncopyable {
 public:
  static const UBlob* Load(const Hash& root_hash);
  // Create the new UBlob from initial data
  // Create the ChunkLoader
  static const UBlob* Create(const byte_t* data, size_t num_bytes);

  ~UBlob();

  // Return the number of bytes in this Blob
  size_t size() const { return root_node_->numElements();}

  /** Delete some bytes from a position and insert new bytes
   *
   *  Args:
   *    pos: the byte position to remove or insert bytes
   *    num_delete: the number of bytes to be deleted
   *    data: the byte array to insert after deletion
   *    num_insert: number of bytes in array to be inserted into current blob
   *
   *  Return:
   *    the new Blob reflecting the operation
   */
  const UBlob* Splice(size_t pos, size_t num_delete,
                      const byte_t* data,
                      size_t num_insert) const;
  /** Insert bytes given a position
   *
   *  Use Splice internally
   */
  const UBlob* Insert(size_t pos, const byte_t* data, size_t num_insert) const;
  /** Delete bytes from a given position
   *
   *  Use Splice internally
   */
  const UBlob* Delete(size_t pos, size_t num_delete) const;
  /** Append bytes from the last position of Blob
   *
   *  Use Splice internally
   */
  const UBlob* Append(byte_t* data, size_t num_insert) const;
  /** Read the blob data and copy into buffer
   *    Args:
   *      pos: the number of position to read
   *      len: the number of subsequent bytes to read into buffer
   *      buffer: the byte array which the data is copied to
   *
   *    Return:
   *      the number of bytes that actually read
   */
  size_t Read(size_t pos, size_t len, byte_t* buffer) const;

  inline const Hash hash() const { return root_node_->hash();}

 private:
  // Private contrucstor to create an instance based on the root chunk data
  // To be called by Load() and Init()
  explicit UBlob(const Chunk* chunk, std::shared_ptr<ChunkLoader> loader);

  // Can either be a leaf(BlobLeafNode) or a non-leaf (MetaNode)
  // Responsible to remove
  const SeqNode* root_node_;

  std::shared_ptr<ChunkLoader> chunk_loader_;  // responsible to delete
};

}  // namespace ustore
#endif  // USTORE_TYPES_UBLOB_H_
