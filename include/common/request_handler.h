// Copyright (c) 2017 The Ustore Authors.

#ifndef INCLUDE_COMMON_REQUEST_HANDLER_H_
#define INCLUDE_COMMON_REQUEST_HANDLER_H_
#include <mutex>
#include <condition_variable>
#include <unordered_map>
#include <vector>
#include "proto/messages.pb.h"
#include "net/net.h"
#include "spec/slice.h"
using std::unordered_map;
using std::condition_variable;
using std::mutex;
namespace ustore {

/**
 * A unit on the response queue. Each client request thread (RequestHandler)
 * waits on one of this object. The thread goes to sleep waiting for has_msg
 * condition to hold true. The has_msg variable will be set by the registered
 * callback method of the network thread
 *
 * This object is used under the assumption that the client issues requests in
 * a synchronous manner. As a result, the msg must be cleared before another
 * response is set.
 */
typedef struct {
  mutex lock;
  condition_variable condition;
  bool has_msg;
  Message *message = nullptr;
} ResponseBlob;


/**
 * Main entrance to the storage. It interfaces with the client (same process),
 * the master and the worker. It has 3 main tasks:
 *
 * 1. Maintain a list of worker, which is synced with the master.  
 * 2. Contain storage APIs to be invoked by the client. For each method it 
 * forwards the corresponding request to the appropriate worker, then waits
 * for a response.
 * 3. When the response indicates error (INVALID_RANGE, for example), it
 * syncs with the master.
 *
 * Each RequestHandler thread processes request synchronously, but responses
 * arrive asynchronously from the network. To route the response back to the
 * correct thread, we use thread ID to identify the message. That is:
 *    + Each UStoreMessage request now contains a field named "source"
 *    + After sending the message, the thread waits on a ResponseBlob
 *      (associated with this "source")
 *    + When a UStoreMessage response arrives, the network thread (callback)
 *      checks for the source and wakes up the corresponding ResponseBlob
 *
 * Basically the old ClientService, a client can invoke RequestHandler in
 * multiple threads, like this:
 *
 *    void ClientThread(node_id_t master, int id) {
 *      RequestHandler *rh = new RequestHandler(...);
 *      while (true) {
 *        status = rh->Get(..); // synchronous
 *        if (status!=SUCESS)
 *          rh->Get(..); // try again
 *        ...         
 *      }
 *    }
 */


class RequestHandler {
 public:
    explicit RequestHandler(const node_id_t& master, int id, Net *net,
        ResponseBlob *blob)
      : master_(master), id_(id), net_(net), res_blob_(blob) {}
    ~RequestHandler();

    /**
     * Initialization. Must perform at least the following:
     * 1. Connect to the master.
     * 2. Connect to all other workers.
     * 3. Initialize the worker list.
     */
    virtual void Init();

    /**
     * Storage APIs. The returned Message contains the Status field indicate
     * if it is successful or not. 
     */
    virtual Message* Put(const Slice &key, const Slice &branch,
        const Slice &version, const Slice &value, bool forward = false,
        bool force = false);
    virtual Message* Get(const Slice &key, const Slice &branch,
        const Slice &version);
    virtual Message* Branch(const Slice &key, const Slice &old_branch,
        const Slice &version, const Slice &new_branch);
    virtual Message* Move(const Slice &key, const Slice &old_branch,
        const Slice &new_branch);
    virtual Message* Merge(const Slice &key, const Slice &value,
        const Slice &target_branch, const Slice &ref_branch,
        const Slice &ref_version, forward = false, force = false);

    inline int id() const noexcept { return id_; }

 private:
    // send request to a node. Return false if there are
    // errors with network communication.
    bool Send(const Message *msg, const node_id_t& node_id);

    // wait for response, and take ownership of the message.
    Message* WaitForResponse();

    // sync the worker list, whenever the storage APIs return error
    virtual bool SyncWithMaster();

    int id_ = 0;  // thread identity, in order to identify the waiting thread
    node_id_t master_;  // address of the master node
    Net *net_ = nullptr;  // for network communication
    WorkerList *workers_;  // lists of workers to which requests are dispatched
    ResponseBlob *blob_;  // response blob
};

/**
 * List of workers and their key ranges. It is initialized and updated
 * with the information from the Master. The RequestHandler uses this to
 * determine where to send the requests to.
 *
 * Different partition strategies may implement the list differently, especially
 * regarding the mapping from key to worker ID (Get_Worker method).
 */
class WorkerList {
 public:
    explicit WorkerList(const vector<RangeInfo> &workers);

    /**
     * Invoked whenever the list is out of date.
     */
    virtual bool Update(const vector<RangeInfor> &workers);

    /**
     * Return the ID (address string) of the worker node whose key range
     * contains the given key.
     * It always returns a valid ID, but the node may have gone offline. The calling
     * function is responsible for updating the list.
     */
    virtual node_id_t get_worker(const Slice& key);

 private:
    // should be sorted by the range
    vector<RangeInfo> workers_;
};
}  // namespace ustore

#endif  // INCLUDE_COMMON_REQUEST_HANDLER_H_
