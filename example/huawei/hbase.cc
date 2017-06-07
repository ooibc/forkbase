// Copyright (c) 2017 The Ustore Authors.

#include <stdlib.h>
#include <chrono>
#include <sstream>
#include <thread>
#include <vector>

#include "cluster/remote_client_service.h"
#include "spec/relational.h"

namespace ustore {
namespace example {
namespace huawei {

constexpr int kInitForMs = 75;

std::vector<std::string> CreateIntColumn(size_t num_records, int max) {
  std::vector<std::string> result;

  for (size_t i = 0; i < num_records; ++i) {
    int r = rand() % max;
    std::stringstream stream;
    stream << r;
    result.push_back(stream.str());
  }
  return result;
}

static const char alphabet[] =
  "0123456789"
  "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
  "abcdefghijklmnopqrstuvwxyz";

std::vector<std::string> CreateStrColumn(size_t num_records, int length) {
  std::vector<std::string> result;
  for (size_t i = 0; i < num_records; ++i) {
    std::string str;
    std::generate_n(std::back_inserter(str), length, [&]() {
      int idx = rand() % 62;
      return alphabet[idx]; });
    result.push_back(str);
  }
  return result;
}

std::vector<std::string> CreateLocationColumn(size_t num_records) {
  std::vector<std::string> result;
  for (size_t i = 0; i < num_records; ++i) {
    std::stringstream sstr;
    sstr << "("
         << rand() % 90 << "." << rand() % 100 << ", "
         << rand() % 180 << "." << rand() % 100
         << ")";
    result.push_back(sstr.str());
  }
  return result;
}

std::vector<std::string> CreateFloatColumn(size_t num_records) {
  std::vector<std::string> result;
  for (size_t i = 0; i < num_records; ++i) {
    std::stringstream sstr;
    sstr << ""
         << rand() % 100 << "." << rand() % 100
         << "";
    result.push_back(sstr.str());
  }
  return result;
}

std::vector<std::string> CreateTimeColumn(size_t num_records) {
  std::vector<std::string> result;
  for (size_t i = 0; i < num_records; ++i) {
    std::stringstream sstr;
    sstr << "["
         << rand() % 24 << ":" << rand() % 60
         << "]";
    result.push_back(sstr.str());
  }
  return result;
}


int main(int argc, char* argv[]) {
  SetStderrLogging(WARNING);
  // connect to UStore servcie
  RemoteClientService ustore_svc("");
  ustore_svc.Init();
  std::thread ustore_svc_thread(&RemoteClientService::Start, &ustore_svc);
  std::this_thread::sleep_for(std::chrono::milliseconds(75));
  ClientDb client_db = ustore_svc.CreateClientDb();
  ColumnStore* cs = new ColumnStore(&client_db);

  size_t num_records = 1000;
  std::string table_name("TB_LOCATION");
  std::string branch_name("master");
  cs->CreateTable(table_name, branch_name);

  cs->PutColumn(table_name, branch_name,
                "MSISDN", CreateStrColumn(num_records, 15));
  cs->PutColumn(table_name, branch_name,
                "IMSI", CreateStrColumn(num_records, 15));
  cs->PutColumn(table_name, branch_name,
                "IMEI", CreateStrColumn(num_records, 15));
  cs->PutColumn(table_name, branch_name,
                "HOMEAREA", CreateStrColumn(num_records, 64));
  cs->PutColumn(table_name, branch_name,
                "CURAREA", CreateStrColumn(num_records, 64));
  cs->PutColumn(table_name, branch_name,
                "LOCATION", CreateLocationColumn(num_records));
  cs->PutColumn(table_name, branch_name,
                "CAPTURETIME", CreateTimeColumn(num_records));
  ustore_svc.Stop();
  ustore_svc_thread.join();
  delete cs;
  std::cout << "Table TB_LOCATION Loaded" << std::endl;
  return 0;
}

}  // namespace huawei
}  // namespace example
}  // namespace ustore

int main(int argc, char* argv[]) {
  return ustore::example::huawei::main(argc, argv);
}