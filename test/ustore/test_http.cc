// Copyright (c) 2017 The Ustore Authors.

#include <thread>
#include <iostream>
#include <fstream>
#include <vector>
#include "gtest/gtest.h"
#include "utils/env.h"
#include "utils/logging.h"
#include "http/server.h"
#include "http/net.h"
#include "worker/worker.h"

using namespace ustore;

const int kSleepTime = 100000;

void Start(HttpServer* server) {
  server->Start();
}

// mock request to send
const string kHeaders = "HTTP/1.1\r\n"
  " Host:  localhost:12345\r\n"
    "User-Agent: Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:44.0) Gecko/20100101 Firefox/44.0\r\n"
    "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
    "Accept-Language: en-US,en;q=0.5\n"
    "Accept-Encoding: gzip, deflate\r\n"
    "Connection: keep-alive\r\n\r\n";

string PutB(const string& key, const string& value, const string& branch, ClientSocket& cs) {
  string post_content = "POST /put " + kHeaders +
      "key=" + key + "&branch=" + branch + "&value=" + value;
  cs.Send(post_content.c_str(), post_content.length());
  string data = cs.Recv();
  int i = data.find("\r\n\r\n");
  return data.substr(i+4);
}

string PutV(const string& key, const string& value,
            const string& version, ClientSocket& cs) {
  string post_content = "POST /put " +
      kHeaders + "key=" + key + "&version=" + version + "&value=" + value;
  cs.Send(post_content.c_str(), post_content.length());
  string data = cs.Recv();
  int i = data.find("\r\n\r\n");
  return data.substr(i+4);
}

string Get(const string& key, const string& version, ClientSocket& cs) {
  string post_content = "POST /get " +
      kHeaders + "key=" + key + "&version=" + version;
  cs.Send(post_content.c_str(), post_content.length());
  string data = cs.Recv();
  int i = data.find("\r\n\r\n");
  return data.substr(i+4);
}

string BranchV(const string& key, const string& version,
               const string& new_branch, ClientSocket& cs) {
  string post_content = "POST /branch " + kHeaders +
      "key=" + key + "&version=" + version + "&new_branch=" + new_branch;
  cs.Send(post_content.c_str(), post_content.length());
  string data = cs.Recv();
  int i = data.find("\r\n\r\n");
  return data.substr(i+4);
}

string BranchB(const string& key, const string& old_branch,
               const string& new_branch, ClientSocket& cs) {
  string post_content = "POST /branch " + kHeaders +
      "key=" + key + "&old_branch=" + old_branch + "&new_branch=" + new_branch;
  cs.Send(post_content.c_str(), post_content.length());
  string data = cs.Recv();
  int i = data.find("\r\n\r\n");
  return data.substr(i+4);
}

string Rename(const string& key, const string& old_branch,
              const string& new_branch, ClientSocket& cs) {
  string post_content = "POST /rename " + kHeaders +
      "key=" + key + "&old_branch=" + old_branch + "&new_branch=" + new_branch;
  cs.Send(post_content.c_str(), post_content.length());
  string data = cs.Recv();
  int i = data.find("\r\n\r\n");
  return data.substr(i+4);
}

string MergeBB(const string& key, const string& tgt_branch,
               const string& ref_branch, const string& value, ClientSocket& cs) {
    string post_content = "POST /merge " + kHeaders +
        "key=mykey&ref_branch=" + ref_branch + "&tgt_branch=" + tgt_branch +
        "&value=" + value;
    cs.Send(post_content.c_str(), post_content.length());
    string data = cs.Recv();
    int i = data.find("\r\n\r\n");
    return data.substr(i+4);
}

string MergeBV(const string& key, const string& tgt_branch,
               const string& version, const string& value, ClientSocket& cs) {
  string post_content = "POST /merge " + kHeaders +
      "key=mykey&ref_version1=" + version + "&tgt_branch=" + tgt_branch +
      "&value=" + value;
  cs.Send(post_content.c_str(), post_content.length());
  string data = cs.Recv();
  int i = data.find("\r\n\r\n");
  return data.substr(i+4);
}

string MergeVV(const string& key, const string& ref_version1,
               const string& ref_version2, const string& value, ClientSocket& cs) {
  string post_content = "POST /merge " + kHeaders +
      "key=mykey&ref_version1=" + ref_version1 +
      "&ref_version2=" + ref_version2 +
      "&value=" + value;
  cs.Send(post_content.c_str(), post_content.length());
  string data = cs.Recv();
  int i = data.find("\r\n\r\n");
  return data.substr(i+4);
}


TEST(HttpTest, BasicOps) {
  Worker worker {2017};
  int port = Env::Instance()->config().http_port();

  // start the http server
  HttpServer server(&worker, port);
  std::thread server_thread(Start, &server);
  sleep(1);

  // connect to the http server
  ClientSocket cs("localhost", port);
  if (cs.Connect() != ST_SUCCESS) {
    DLOG(INFO) << "cannot connect to the server";
    return;
  }

  string key = "mykey";
  string value1 = "value1";
  string branch1 = "mybranch1";
  // put a new key value
  string version = PutB(key, value1, branch1, cs);
  string version1 = version;
  DLOG(INFO) << "Got version: " << version;

  // get the value
  string value = Get(key, version, cs);
  DLOG(INFO) << "Got value: " << value;
  CHECK_EQ(value1, value);

  // put a key value based on the previous version
  string value2 = "value2";
  version = PutV(key, value2, version, cs);
  string version2 = version;
  DLOG(INFO) << "Got version: " << version;

  // get the value
  value = Get(key, version, cs);
  DLOG(INFO) << "Got value: " << value;
  CHECK_EQ(value, value2);

  // branch based on version
  string branch2 = "mybranch2";
  string status = BranchV(key, version, branch2, cs);
  DLOG(INFO) << "New branch " << branch2 << ": " << status;
  CHECK(status == "OK" || status == "Branch Error: 5");

  // branch based on branch
  string branch3 = "mybranch3";
  status = BranchB(key, branch1, branch3, cs);
  DLOG(INFO) << "New branch " << branch3 << ": " << status;
  CHECK(status == "OK" || status == "Branch Error: 5");

  // rename a branch
  string branch4 = "mybranch4";
  status = Rename(key, branch1, branch4, cs);
  DLOG(INFO) << "Rename branch from " <<
      branch1 << " to " << branch4 << ": " << status;
  CHECK(status == "OK" || status == "Rename Error: 5");

  // merge two values
  // merge target branch to a referring branch
  string value3 = "value3";
  version = MergeBB(key, branch2, branch3, value3, cs);
  DLOG(INFO) << "Merge branch "
     << branch3 << " based on " << branch2 << ": " << version;

  // get back the value to check
  value = Get(key, version, cs);
  CHECK_EQ(value, value3);

  // merge target branch to a referring version
  string value4 = "value4";
  string version3 = MergeBV(key, branch2, version, value4, cs);
  DLOG(INFO) << "Merge branch "
      << branch2 << " based on " << version << ": " << version3;

  // get back the value to check
  value = Get(key, version3, cs);
  CHECK_EQ(value, value4);

  // merge two existing versions
  string value5 = "value5";
  string version4 = MergeVV(key, version1, version2, value5, cs);
  DLOG(INFO) << "Merge version "
      << version1 << " based on " << version2 << ": " << version4;

  // get back the value to check
  value = Get(key, version4, cs);
  CHECK_EQ(value, value5);

  server.Stop();
  sleep(1);
  server_thread.join();
}