// Copyright (c) 2017 The Ustore Authors.

#ifndef USTORE_EXAMPLE_CA_ANALYTICS_H_
#define USTORE_EXAMPLE_CA_ANALYTICS_H_

#include <ctime>
#include <iostream>
#include <random>
#include <string>
#include "spec/slice.h"
#include "spec/value.h"
#include "types/type.h"
#include "utils/logging.h"
#include "worker/worker_ext.h"

#include "ca/config.h"
#include "ca/relational.h"
#include "ca/utils.h"

namespace ustore {
namespace example {
namespace ca {

class Analytics {
 public:
  template<class T>
  Analytics(const T& branch, WorkerExt& db)
    : branch_(Slice(branch)), db_(db) {}

  inline const Slice& branch() { return branch_; }
  virtual int Compute(StringSet* aff_cols) = 0;

 protected:
  const Slice branch_;
  WorkerExt& db_;

  template<class T1, class T2>
  ErrorCode BranchAndLoad(const T1& col_name, const T2& base_branch,
                          Value* val);
};

template<class T1, class T2>
ErrorCode Analytics::BranchAndLoad(const T1& col_name, const T2& base_branch,
                                   Value* col) {
  Slice col_name_slice(col_name);
  Slice base_branch_slice(base_branch);
  USTORE_GUARD(db_.Branch(col_name_slice, base_branch_slice, branch_));
  USTORE_GUARD(db_.Get(col_name_slice, branch_, col));
  return ErrorCode::kOK;
}

class Random {
 public:
  Random() : rand_gen_(std::time(0)) {}
 protected:
  std::mt19937 rand_gen_;
  virtual uint32_t NextRandom() = 0;
};

class DataLoading : public Analytics {
 public:
  template<class T>
  DataLoading(const T& branch, WorkerExt& db,
              size_t n_columns, size_t n_records)
    : Analytics(branch, db), n_columns_(n_columns), n_records_(n_records) {
    std::cout << "[Parameters]"
              << " branch=\"" << branch_ << '\"' << std::endl;
  }

  int Compute(StringSet* aff_cols) override;

 private:
  const size_t n_columns_;
  const size_t n_records_;
};

class PoissonAnalytics : public Analytics, private Random {
 public:
  template<class T>
  PoissonAnalytics(const T& branch, WorkerExt& db, double mean)
    : Analytics(branch, db), distr_(mean) {
    std::cout << "[Parameters]"
              << " branch=\"" << branch_ << '\"'
              << ", lambda=" << mean << std::endl;
  }

  int Compute(StringSet* aff_cols) override;

 private:
  std::poisson_distribution<uint32_t> distr_;
  inline uint32_t NextRandom() override { return distr_(rand_gen_); }
};

class BinomialAnalytics : public Analytics, private Random {
 public:
  template<class T>
  BinomialAnalytics(const T& branch, WorkerExt& db, double p)
    : Analytics(branch, db), distr_(Config::n_records - 1, p) {
    std::cout << "[Parameters]"
              << " branch=\"" << branch_ << '\"'
              << ", p=" << p
              << ", n=" << Config::n_records << std::endl;
  }

  int Compute(StringSet* aff_cols) override;

 private:
  std::binomial_distribution<uint32_t> distr_;
  inline uint32_t NextRandom() override { return distr_(rand_gen_); }
};

class MergeAnalytics : public Analytics {
 public:
  template<class T>
  MergeAnalytics(const T& branch, WorkerExt& db)
    : Analytics(branch, db) {
    std::cout << "[Parameters]"
              << " branch=\"" << branch_ << '\"' << std::endl;
  }

  int Compute(StringSet* aff_cols) override;
};

class ColumnStoreAnalytics {
 public:
  ColumnStoreAnalytics(const std::string& branch, ColumnStore& cs)
    : branch_(branch), cs_(cs) {}

  inline const std::string& branch() { return branch_; }
  virtual int Compute(StringSet* aff_cols) = 0;

 protected:
  const std::string branch_;
  ColumnStore& cs_;
};

class SampleAnalytics : public ColumnStoreAnalytics {
  SampleAnalytics(const std::string& branch, ColumnStore& cs)
    : ColumnStoreAnalytics(branch, cs) {}

  int Compute(StringSet* aff_cols) override;
};

}  // namespace ca
}  // namespace example
}  // namespace ustore

#endif  // USTORE_EXAMPLE_CA_ANALYTICS_H_
