// Copyright (c) 2017 The Ustore Authors.
#include <cstring>
#include <string>

#include "chunk/chunk.h"
#include "node/cell_node.h"
#include "gtest/gtest.h"
#include "types/ustring.h"

#ifdef USE_LEVELDB
#include "store/ldb_store.h"
#include "utils/singleton.h"
#endif  // USE_LEVELDB

#include "utils/debug.h"
#include "utils/logging.h"

const ustore::byte_t raw_data[] =
    "The quick brown fox jumps over the lazy dog";

TEST(UString, Load) {
  //////////////////////////////////////////////////////
  // Prepare the chunk to load
  const ustore::Chunk* chunk =
      ustore::StringNode::NewChunk(raw_data, sizeof(raw_data));

  #ifdef USE_LEVELDB
  ustore::ChunkStore* cs = ustore::Singleton<ustore::LDBStore>::Instance();
  #else
  // other storage
  #endif  // USE_LEVELDB
  // Put the chunk into storage
  cs->Put(chunk->hash(), *chunk);
  //////////////////////////////////////////////////////

  const ustore::UString* ustring = ustore::UString::Load(chunk->hash());
  ASSERT_EQ(ustring->len(), sizeof(raw_data));


  ustore::byte_t* buffer = new ustore::byte_t[sizeof(raw_data)];
  ASSERT_EQ(ustring->data(buffer), sizeof(raw_data));

  std::string buf_str = ustore::byte2str(buffer, sizeof(raw_data));
  std::string expected_str = ustore::byte2str(raw_data, sizeof(raw_data));

  ASSERT_EQ(buf_str, expected_str);
}

TEST(UString, Create) {
  const ustore::UString* ustring = ustore::UString::Create(raw_data,
                                                           sizeof(raw_data));

  ASSERT_EQ(ustring->len(), sizeof(raw_data));

  ustore::byte_t* buffer = new ustore::byte_t[sizeof(raw_data)];
  ASSERT_EQ(ustring->data(buffer), sizeof(raw_data));

  std::string buf_str = ustore::byte2str(buffer, sizeof(raw_data));
  std::string expected_str = ustore::byte2str(raw_data, sizeof(raw_data));

  ASSERT_EQ(buf_str, expected_str);
}
