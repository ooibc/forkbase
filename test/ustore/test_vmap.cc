// Copyright (c) 2017 The Ustore Authors.

#include "gtest/gtest.h"

#include "spec/object_db.h"
#include "types/client/vmap.h"
#include "worker/worker.h"

std::vector<std::string> smap_key {"The", "quick", "brown", "fox", "jumps",
                                   "over", "the", "lazy", "dog"};
std::vector<std::string> smap_val {"v1", "v2", "v3", "v4", "v5",
                                   "v6", "v7", "v8", "v9"};

using ustore::byte_t;
using ustore::Slice;
using ustore::VMeta;
using ustore::ErrorCode;
using ustore::Hash;

ustore::Worker worker_vmap(17);
const char key_vmap[] = "key_vmap";
const char branch_vmap[] = "branch_vmap";

TEST(VMap, CreateNewVMap) {
  ustore::ObjectDB db(&worker_vmap);
  std::sort(smap_key.begin(), smap_key.end());
  std::vector<Slice> slice_key, slice_val;
  for (const auto& s : smap_key) slice_key.push_back(Slice(s));
  for (const auto& s : smap_val) slice_val.push_back(Slice(s));
  // create buffered new map
  ustore::VMap map(slice_key, slice_val);
  // put new map
  VMeta put = db.Put(Slice(key_vmap), map, Slice(branch_vmap));
  EXPECT_TRUE(ErrorCode::kOK == put.code());
  EXPECT_TRUE(put.cell().empty());
  EXPECT_FALSE(put.version().empty());
  // get map
  VMeta get = db.Get(Slice(key_vmap), Slice(branch_vmap));
  EXPECT_TRUE(ErrorCode::kOK == get.code());
  EXPECT_FALSE(get.cell().empty());
  EXPECT_TRUE(get.version().empty());
  auto v = get.Map();
  // check data
  auto it = v.Scan();
  for (int i = 0; i < slice_key.size(); ++i) {
    EXPECT_EQ(slice_key[i], it.key());
    EXPECT_EQ(slice_val[i], it.value());
    it.next();
  }
}

TEST(VMap, AddToExistingVMap) {
  ustore::ObjectDB db(&worker_vmap);
  std::sort(smap_key.begin(), smap_key.end());
  std::vector<Slice> slice_key, slice_val;
  for (const auto& s : smap_key) slice_key.push_back(Slice(s));
  for (const auto& s : smap_val) slice_val.push_back(Slice(s));
  // create buffered new map
  ustore::VMap map(slice_key, slice_val);
  // put new map
  Hash hash = db.Put(Slice(key_vmap), map, Slice(branch_vmap)).version();
  // get map
  auto v = db.Get(Slice(key_vmap), Slice(branch_vmap)).Map();
  // update map
  std::string delta_key = "z delta";
  std::string delta_val = "v delta";
  slice_key.push_back(Slice(delta_key));
  slice_val.push_back(Slice(delta_val));
  v.Set(Slice(delta_key), Slice(delta_val));
  VMeta update = db.Put(Slice(key_vmap), v, Slice(branch_vmap));
  EXPECT_TRUE(ErrorCode::kOK == update.code());
  EXPECT_TRUE(update.cell().empty());
  EXPECT_FALSE(update.version().empty());
  // add to map
  VMeta get = db.Get(Slice(key_vmap), Slice(branch_vmap));
  EXPECT_TRUE(ErrorCode::kOK == get.code());
  EXPECT_FALSE(get.cell().empty());
  EXPECT_TRUE(get.version().empty());
  v = get.Map();
  // check data
  auto it = v.Scan();
  for (int i = 0; i < slice_key.size(); ++i) {
    EXPECT_EQ(slice_key[i], it.key());
    EXPECT_EQ(slice_val[i], it.value());
    it.next();
  }
}

TEST(VMap, RemoveFromExistingVMap) {
  ustore::ObjectDB db(&worker_vmap);
  std::sort(smap_key.begin(), smap_key.end());
  std::vector<Slice> slice_key, slice_val;
  for (const auto& s : smap_key) slice_key.push_back(Slice(s));
  for (const auto& s : smap_val) slice_val.push_back(Slice(s));
  // create buffered new map
  ustore::VMap map(slice_key, slice_val);
  // put new map
  Hash hash = db.Put(Slice(key_vmap), map, Slice(branch_vmap)).version();
  // get map
  auto v = db.Get(Slice(key_vmap), Slice(branch_vmap)).Map();
  // remove from map
  v.Remove(slice_key[slice_key.size()-1]);
  VMeta update = db.Put(Slice(key_vmap), v, Slice(branch_vmap));
  EXPECT_TRUE(ErrorCode::kOK == update.code());
  EXPECT_TRUE(update.cell().empty());
  EXPECT_FALSE(update.version().empty());
  // get updated map
  VMeta get = db.Get(Slice(key_vmap), Slice(branch_vmap));
  EXPECT_TRUE(ErrorCode::kOK == get.code());
  EXPECT_FALSE(get.cell().empty());
  EXPECT_TRUE(get.version().empty());
  v = get.Map();
  // check data
  auto it = v.Scan();
  for (int i = 0; i < slice_key.size() - 1; ++i) {
    EXPECT_EQ(slice_key[i], it.key());
    EXPECT_EQ(slice_val[i], it.value());
    it.next();
  }
}

TEST(VMap, UpdateExistingVMap) {
  ustore::ObjectDB db(&worker_vmap);
  std::sort(smap_key.begin(), smap_key.end());
  std::vector<Slice> slice_key, slice_val;
  for (const auto& s : smap_key) slice_key.push_back(Slice(s));
  for (const auto& s : smap_val) slice_val.push_back(Slice(s));
  // create buffered new map
  ustore::VMap map(slice_key, slice_val);
  // put new map
  Hash hash = db.Put(Slice(key_vmap), map, Slice(branch_vmap)).version();
  // get map
  auto v = db.Get(Slice(key_vmap), Slice(branch_vmap)).Map();
  // update map
  std::string nv = "new_v";
  v.Set(slice_key[0], Slice(nv));
  slice_val[0] = Slice(nv);
  VMeta update = db.Put(Slice(key_vmap), v, Slice(branch_vmap));
  EXPECT_TRUE(ErrorCode::kOK == update.code());
  EXPECT_TRUE(update.cell().empty());
  EXPECT_FALSE(update.version().empty());
  // get updated map
  VMeta get = db.Get(Slice(key_vmap), Slice(branch_vmap));
  EXPECT_TRUE(ErrorCode::kOK == get.code());
  EXPECT_FALSE(get.cell().empty());
  EXPECT_TRUE(get.version().empty());
  v = get.Map();
  // check data
  auto it = v.Scan();
  for (int i = 0; i < slice_key.size(); ++i) {
    EXPECT_EQ(slice_key[i], it.key());
    EXPECT_EQ(slice_val[i], it.value());
    it.next();
  }
}
