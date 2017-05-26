// Copyright (c) 2017 The Ustore Authors.

#include "worker/worker_ext.h"

#include "ca/analytics.h"
#include "ca/config.h"
#include "ca/relational.h"
#include "ca/utils.h"

namespace ustore {
namespace example {
namespace ca {

WorkerExt db(43);
ColumnStore cs(&db);

int RunSample() {
  std::cout << std::endl
            << "-------------[ Sample Analytics ]------------" << std::endl;
  GUARD_INT(SampleAnalytics("sample", cs).Compute(nullptr));
  std::cout << "---------------------------------------------" << std::endl;
  return 0;
}

int LoadDataset() {
  std::cout << std::endl
            << "-------------[ Loading Dataset ]-------------" << std::endl;
  auto ana = DataLoading("master", cs, Config::n_columns, Config::n_records);
  std::unordered_set<std::string> aff_cols;
  GUARD_INT(ana.Compute(&aff_cols));
  for (const auto& col_name : aff_cols) {
    Column col;
    USTORE_GUARD_INT(cs.GetColumn("Sample", "master", col_name, &col));
    Utils::Print("Sample", "master", col_name, col);
  }
  std::cout << "---------------------------------------------" << std::endl;
  return 0;
}

int RunPoissonAnalytics(const double mean) {
  std::cout << std::endl
            << "------------[ Poisson Analytics ]------------" << std::endl;
  std::unordered_set<std::string> aff_cols;
  GUARD_INT(PoissonAnalytics("poi_ana", cs, mean).Compute(&aff_cols));
  std::cout << ">>> Affected Columns <<<" << std::endl;
  for (const auto& col_name : aff_cols) {
    Column col;
    USTORE_GUARD_INT(cs.GetColumn("Sample", "poi_ana", col_name, &col));
    Utils::Print("Sample", "poi_ana", col_name, col);
  }
  std::cout << "---------------------------------------------" << std::endl;
  return 0;
}

int RunBinomialAnalytics(const double p) {
  std::cout << std::endl
            << "-----------[ Binomial Analytics ]------------" << std::endl;
  std::unordered_set<std::string> aff_cols;
  GUARD_INT(BinomialAnalytics("bin_ana", cs, p).Compute(&aff_cols));
  std::cout << ">>> Affected Columns <<<" << std::endl;
  for (const auto& col_name : aff_cols) {
    Column col;
    USTORE_GUARD_INT(cs.GetColumn("Sample", "bin_ana", col_name, &col));
    Utils::Print("Sample", "bin_ana", col_name, col);
  }
  std::cout << "---------------------------------------------" << std::endl;
  return 0;
}

int MergeResults() {
  std::cout << std::endl
            << "-------------[ Merging Results ]-------------" << std::endl;
  auto ana = MergeAnalytics("master", cs);
  std::unordered_set<std::string> aff_cols;
  GUARD_INT(ana.Compute(&aff_cols));
  std::cout << ">>> Affected Columns <<<" << std::endl;
  for (const auto& col_name : aff_cols) {
    Column col;
    USTORE_GUARD_INT(cs.GetColumn("Sample", "master", col_name, &col));
    Utils::Print("Sample", "master", col_name, col);
  }
  std::cout << "---------------------------------------------" << std::endl;
  return 0;
}

#define MAIN_GUARD(op) do { \
  auto ec = op; \
  if (ec != 0) { \
    std::cerr << "[FAILURE] Error code: " << ec << std::endl; \
    return ec; \
  } \
} while (0)

int main(int argc, char* argv[]) {
  if (Config::ParseCmdArgs(argc, argv)) {
    MAIN_GUARD(RunSample());
    MAIN_GUARD(LoadDataset());
    MAIN_GUARD(RunPoissonAnalytics(Config::p * Config::n_records));
    MAIN_GUARD(RunBinomialAnalytics(Config::p));
    MAIN_GUARD(MergeResults());
  } else if (Config::is_help) {
    DLOG(INFO) << "Help messages have been printed";
  } else {
    std::cerr << "[FAILURE] Found invalid command-line option" << std::endl;
    return -1;
  }
  return 0;
}

}  // namespace ca
}  // namespace example
}  // namespace ustore

int main(int argc, char* argv[]) {
  return ustore::example::ca::main(argc, argv);
}
