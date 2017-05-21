// Copyright (c) 2017 The Ustore Authors.

#ifndef USTORE_CLUSTER_WORKER_SERVICE_H_
#define USTORE_CLUSTER_WORKER_SERVICE_H_

#include <vector>
#include "net/net.h"
#include "proto/messages.pb.h"
#include "types/ucell.h"
#include "spec/value.h"

namespace ustore {

class Worker;

/**
 * The WorkerService receives requests from ClientService and invokes
 * the Worker to process the message.
 * Basically a simple version of ClientService. It only has as many 
 * processing threads as provided by the Net implementation.
 */
class WorkerService {
 public:
    // Dispatching requests from the network.
    // Basically call this->HandleRequest that invoke Worker methods.
    static void RequestDispatch(const void *msg, int size, void *handler,
                                const node_id_t& source);

    static int range_cmp(const RangeInfo& a, const RangeInfo& b);

    explicit WorkerService(const node_id_t& addr, const node_id_t& master):
                                  node_addr_(addr), master_(master) {}
    ~WorkerService();

    // initialize the network, the worker and register callback
    virtual void Init();
    virtual void Start();
    virtual void Stop();

    /**
     * Handle requests:
     * 1. It parse msg into a UStoreMessage
     * 2. Invoke the processing logic from Worker.
     * 3. Construct a response and send back to source.
     */
    virtual void HandleRequest(const void *msg, int size,
                               const node_id_t& source);

 private:
    node_id_t master_;  // master node
    node_id_t node_addr_;  // this node's address
    Net *net_;
    std::vector<RangeInfo> ranges_;  // global knowledge about key ranges
    Worker* worker_;  // where the logic happens
    std::vector<node_id_t> addresses_;  // worker addresses
    CallBack* cb_ = nullptr;

    // helper methods for parsing request/response
    bool CreateUCellPayload(const UCell &val, UCellPayload *payload);
    Value2* Value2FromRequest(Value2Payload *payload);
};
}  // namespace ustore

#endif  // USTORE_CLUSTER_WORKER_SERVICE_H_
