// Copyright (c) 2017 The Ustore Authors.
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <thread>
#include <vector>
#include <set>
#include <string>
#include "gtest/gtest.h"
#include "proto/messages.pb.h"
#include "types/type.h"
#include "types/ucell.h"
#include "utils/env.h"
#include "cluster/worker_service.h"
#include "cluster/remote_client_service.h"
#include "hash/hash.h"
#include "spec/slice.h"
#include "spec/blob.h"
#include "spec/value.h"
#include "utils/logging.h"

using ustore::UStoreMessage;
using ustore::byte_t;
using ustore::WorkerService;
using ustore::RemoteClientService;
using ustore::Config;
using ustore::Slice;
using ustore::Hash;
using ustore::Blob;
using ustore::Env;
using ustore::ErrorCode;
using ustore::ClientDb;
using ustore::Value;
using ustore::Value2;
using ustore::UType;
using ustore::UCell;
using std::thread;
using std::vector;
using std::set;
using std::ifstream;
using std::string;

const int NREQUESTS = 4;
const string keys[] = {"aaa", "bbb", "ccc", "ddd"};
const string values[] = {"where is the wisdome in knowledge",
                         "where is the knowledge in information",
                         "the brown fox",
                         "jump over"};

// i^th thread issue requests from i*(nthreads/nreqs) to
// (i+1)*(nthreads/nreqs)
void TestClientRequest(ClientDb* client, int idx, int len) {
  Hash HEAD_VERSION = Hash::ComputeFrom((const byte_t*)("head"), 4);

  // put a string
  Value2 string_val;
  string_val.type = UType::kString;
  string_val.base = Hash::kNull;
  string_val.vals.push_back(Slice(values[idx]));
  Hash version;
  EXPECT_EQ(client->Put(Slice(keys[idx]),
        //Value(Blob((const byte_t*)values[idx].data(),
        //values[idx].length())), 
        string_val,
        HEAD_VERSION, &version), ErrorCode::kOK);
  LOG(INFO) << "PUT version (string): " << version.ToBase32();

  // put a list of 2 values
  Value2 list_val;
  list_val.type = UType::kList;
  list_val.base = Hash::kNull;
  list_val.vals.push_back(Slice(values[0]));
  list_val.vals.push_back(Slice(values[idx]));
  Hash version_list;
  EXPECT_EQ(client->Put(Slice(keys[idx]),
        //Value(Blob((const byte_t*)values[idx].data(),
        //values[idx].length())), 
        list_val,
        HEAD_VERSION, &version_list), ErrorCode::kOK);
  LOG(INFO) << "PUT version (list): " << version_list.ToBase32();

  // get the string back
  UCell string_value;
  EXPECT_EQ(client->Get(Slice(keys[idx]), version, &string_value),
                                        ErrorCode::kOK);
  EXPECT_EQ(string_value.type(), UType::kString); 
  LOG(INFO) << "GET datahash (string): "
              <<  string_value.dataHash().ToBase32();

  // get the list back
  UCell list_value;
  EXPECT_EQ(client->Get(Slice(keys[idx]), version_list, &list_value),
                                        ErrorCode::kOK);
  EXPECT_EQ(list_value.type(), UType::kList); 
  LOG(INFO) << "GET datahash (list): " <<  list_value.dataHash().ToBase32();

  // branch from head
  string new_branch = "branch_"+std::to_string(idx);
  EXPECT_EQ(client->Branch(Slice(keys[idx]),
                              version, Slice(new_branch)), ErrorCode::kOK);

  // put on the new branch (string value)
  Hash branch_version;
  EXPECT_EQ(client->Put(Slice(keys[idx]),
      //Value(Blob((const byte_t *)values[idx].data(),
      //values[idx].length())), 
      string_val,
      Slice(new_branch), &branch_version), ErrorCode::kOK);
  
  LOG(INFO) << "PUT version (new branch): " << branch_version.ToBase32() 
            << std::endl;

  // merge
  Hash merge_version;
  EXPECT_EQ(client->Merge(Slice(keys[idx]),
        //Value(Blob((const byte_t *)values[idx].data(), values[idx].length())),
        string_val,
                Slice(new_branch), version, &merge_version), ErrorCode::kOK);
  LOG(INFO) << "MERGE version (w/o branch): " << merge_version.ToBase32()
            << std::endl;

  EXPECT_EQ(client->Merge(Slice(keys[idx]),
        //Value(Blob((const byte_t *)values[idx].data(), values[idx].length())),
                string_val,
                version, branch_version, &merge_version),
                ErrorCode::kOK);
  LOG(INFO) << "MERGE version (with branch): " << merge_version.ToBase32()
            << std::endl;
}

TEST(TestMessage, TestClient1Thread) {
  ustore::SetStderrLogging(ustore::WARNING);
  // launch workers
  ifstream fin(Env::Instance()->config()->worker_file());
  string worker_addr;
  vector<WorkerService*> workers;
  while (fin >> worker_addr)
    workers.push_back(new WorkerService(worker_addr, ""));

  vector<thread> worker_threads;
  for (int i = 0; i < workers.size(); i++)
    workers[i]->Init();

  for (int i = 0; i < workers.size(); i++)
      worker_threads.push_back(thread(&WorkerService::Start, workers[i]));

  // launch clients
  ifstream fin_client(Env::Instance()->config()->clientservice_file());
  string clientservice_addr;
  fin_client >> clientservice_addr;
  RemoteClientService *service
    = new RemoteClientService(clientservice_addr, "");
  service->Init();
  // service->Start();
  thread client_service_thread(&RemoteClientService::Start, service);
  sleep(1);

  // 1 thread
  ClientDb *client = service->CreateClientDb();
  TestClientRequest(client, 0, NREQUESTS);

  service->Stop();
  client_service_thread.join();

  // then stop workers
  for (WorkerService *ws : workers) {
    ws->Stop();
  }
  for (int i = 0; i < worker_threads.size(); i++)
    worker_threads[i].join();

  // delete workers and client
  for (int i=0; i< workers.size(); i++)
    delete workers[i];
  delete service;
}

/*
TEST(TestMessage, TestClient2Threads) {
  ustore::SetStderrLogging(ustore::WARNING);
  // launch workers
  ifstream fin(Env::Instance()->config()->worker_file());
  string worker_addr;
  vector<WorkerService*> workers;
  while (fin >> worker_addr)
    workers.push_back(new WorkerService(worker_addr, ""));

  vector<thread> worker_threads;
  vector<thread> client_threads;
  for (int i = 0; i < workers.size(); i++)
    workers[i]->Init();

  for (int i = 0; i < workers.size(); i++)
      worker_threads.push_back(thread(&WorkerService::Start, workers[i]));

  // launch clients
  ifstream fin_client(Env::Instance()->config()->clientservice_file());
  string clientservice_addr;
  fin_client >> clientservice_addr;
  RemoteClientService *service
    = new RemoteClientService(clientservice_addr, "");
  service->Init();
  // service->Start();
  thread client_service_thread(&RemoteClientService::Start, service);
  sleep(1);

  // 2 clients thread
  for (int i = 0; i < 2; i++) {
    ClientDb *client = service->CreateClientDb();
    client_threads.push_back(thread(
                        &TestClientRequest, client, i*2, NREQUESTS/2));
  }

  // wait for them to join
  for (int i = 0; i < 2; i++)
    client_threads[i].join();

  service->Stop();
  client_service_thread.join();

  // then stop workers
  for (WorkerService *ws : workers) {
    ws->Stop();
  }
  for (int i = 0; i < worker_threads.size(); i++)
    worker_threads[i].join();

  // clean up
  for (int i=0; i< workers.size(); i++)
    delete workers[i];
  delete service;
}
*/
