// Copyright (c) 2017 The Ustore Authors

#include "cluster/remote_chunk_client_service.h"

namespace ustore {

class ChunkClientCallBack : public CallBack {
 public:
  explicit ChunkClientCallBack(void* handler) : CallBack(handler) {}
  void operator()(const void *msg, int size, const node_id_t& source) override {
    (reinterpret_cast<RemoteChunkClientService *>(handler_))->HandleResponse(
                                        msg, size, source);
  }
};

void RemoteChunkClientService::Start() {
  net_->CreateNetContexts(ptt_.workerAddrs());
  cb_.reset(new ChunkClientCallBack(this));
  net_->RegisterRecv(cb_.get());

  is_running_ = true;
  net_->Start();
}

ChunkDb RemoteChunkClientService::CreateChunkDb() {
  // adding a new response blob
  ResponseBlob* resblob = new ResponseBlob();
  responses_.emplace_back(resblob);
  return ChunkDb(master_, nclients_++, net_.get(), resblob, &ptt_);
}

}  // namespace ustore
