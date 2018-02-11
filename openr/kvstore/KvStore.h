/**
 * Copyright (c) 2014-present, Facebook, Inc.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <chrono>
#include <map>
#include <memory>
#include <queue>
#include <string>

#include <boost/serialization/strong_typedef.hpp>
#include <fbzmq/async/ZmqEventLoop.h>
#include <fbzmq/async/ZmqTimeout.h>
#include <fbzmq/service/monitor/ZmqMonitorClient.h>
#include <fbzmq/service/stats/ThreadData.h>
#include <fbzmq/zmq/Zmq.h>
#include <folly/Optional.h>
#include <folly/io/IOBuf.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

#include <openr/common/Constants.h>
#include <openr/common/ExponentialBackoff.h>
#include <openr/common/Types.h>
#include <openr/if/gen-cpp2/KvStore_types.h>

namespace openr {

struct TtlCountdownQueueEntry {
  std::chrono::steady_clock::time_point expiryTime;
  std::string key;
  int64_t version{0};
  int64_t ttlVersion{0};
  bool
  operator>(TtlCountdownQueueEntry other) const {
    return expiryTime > other.expiryTime;
  }
};

using TtlCountdownQueue = std::priority_queue<
    TtlCountdownQueueEntry,
    std::vector<TtlCountdownQueueEntry>,
    std::greater<TtlCountdownQueueEntry> // Always returns smallest first
    >;

// The class represent a server that stores KV pairs in internal map.
// it listens for submission on REP socket, subscribes to peers via
// SUB socket, and publishes to peers via PUB socket. The configuration
// is passed via constructor arguments.

class KvStore final : public fbzmq::ZmqEventLoop {
 public:
  KvStore(
      // the zmq context to use for IO
      fbzmq::Context& zmqContext,
      // the name of this node (unique in domain)
      std::string nodeId,
      // the url we use to publish our updates to
      // local subscribers
      KvStoreLocalPubUrl localPubUrl,
      // the url we use to publish our updates to
      // any subscriber (often encrypted)
      KvStoreGlobalPubUrl globalPubUrl,
      // the url to receive commands from local
      // clients (same host)
      KvStoreLocalCmdUrl localCmdUrl,
      // the url to receive command from local and
      // non local clients (often encrypted channel)
      KvStoreGlobalCmdUrl globalCmdUrl,
      // the url to submit to monitor
      MonitorSubmitUrl monitorSubmitUrl,
      // IP TOS value to set on sockets using TCP
      folly::Optional<int> ipTos,
      // the key pair to use for crypto sockets. if set to
      // folly::none, crypto is not used.
      folly::Optional<fbzmq::KeyPair> keyPair,
      // how often to request full db sync from peers
      std::chrono::seconds dbSyncInterval,
      // how often to submit to monitor
      std::chrono::seconds monitorSubmitInterval,
      // initial list of peers to connect to
      std::unordered_map<std::string, thrift::PeerSpec> peers,
      // optional: pre allocated and bound global pub socket
      // will allocate new pub socket if not supplied
      folly::Optional<fbzmq::Socket<ZMQ_PUB, fbzmq::ZMQ_SERVER>>
          preBoundGlobalPubSock = folly::none,
      // optional: pre allocated and bound global cmd socket
      // will allocate new cmd socket if not supplied
      folly::Optional<fbzmq::Socket<ZMQ_ROUTER, fbzmq::ZMQ_SERVER>>
          preBoundGlobalCmdSock = folly::none);

  // process the key-values publication, and attempt to
  // merge it in existing map (first argument)
  // Return a publication made out of the updated values
  static thrift::Publication mergeKeyValues(
      std::unordered_map<std::string, thrift::Value>& kvStore,
      std::unordered_map<std::string, thrift::Value> const& update);

 private:
  // disable copying
  KvStore(KvStore const&) = delete;
  KvStore& operator=(KvStore const&) = delete;

  //
  // Private methods
  //

  // consume a publication pending on sub_ socket
  // (i.e. announced by some of our peers)
  // relays the original publication if needed
  void processPublication();

  // get multiple keys at once
  thrift::Publication getKeyVals(std::vector<std::string> const& keys);

  // dump the entries of my KV store whose keys match the given prefix
  // if prefix is the empty sting, the full KV store is dumped
  thrift::Publication dumpAllWithPrefix(std::string const& prefix) const;

  // dump the hashes of my KV store whose keys match the given prefix
  // if prefix is the empty sting, the full hash store is dumped
  thrift::Publication dumpHashWithPrefix(std::string const& prefix) const;

  // dump the keys on which hashes differ from given keyVals
  thrift::Publication dumpDifference(
      std::unordered_map<std::string, thrift::Value> const& keyValHash) const;

  // add new peers to sync with
  void addPeers(std::unordered_map<std::string, thrift::PeerSpec> const& peers);

  // delete some peers we are subscribed to
  void delPeers(std::vector<std::string> const& peers);

  // request full-sync (KEY_DUMP) with peersToSyncWith_
  void requestFullSyncFromPeers();

  // dump all peers we are subscribed to
  thrift::PeerCmdReply dumpPeers();

  // add new query entries into ttlCountdownQueue from publication
  // and Reschedule ttl expiry timer if needed
  void updateTtlCountdownQueue(const thrift::Publication& publication);

  // process a request pending on cmdSock socket
  void processRequest(
      fbzmq::Socket<ZMQ_ROUTER, fbzmq::ZMQ_SERVER>& cmdSock) noexcept;

  // process received KV_DUMP from one of our neighbor
  void processSyncResponse() noexcept;

  // randomly request sync from one connected neighbor
  void requestSync();

  // this will poll the sockets listening to the requests
  void attachCallbacks();

  // periodically count down and purge expired keys if any
  void countdownTtl();

  // count number of prefixes in kvstore
  int getPrefixCount();

  // Extracts the counters and submit them to monitor
  void submitCounters();

  // Submit events to monitor
  void logKvEvent(const std::string& event, const std::string& key);

  //
  // Private variables
  //

  //
  // Immutable state
  //

  // zmq context
  fbzmq::Context& zmqContext_;

  // unique among all nodes, identifies this particular node
  const std::string nodeId_;

  // we only encrypt inter-node traffic and don't encrypt intra-node traffic
  // tcp*_ sockets are used to communicate with external node
  // inproc*_ sockets within a node

  // The ZMQ URL we'll be using for publications
  const std::string localPubUrl_;
  const std::string globalPubUrl_;

  // The ZMQ URL we'll be listening for commands on
  const std::string localCmdUrl_;
  const std::string globalCmdUrl_;

  // base interval to run syncs with (jitter will be added)
  const std::chrono::seconds dbSyncInterval_;

  // Interval to submit to monitor. Default value is high
  // to avoid submission of counters in testing.
  const std::chrono::seconds monitorSubmitInterval_;

  //
  // Mutable state
  //

  // The peers we will be talking to: both PUB and CMD URLs for each
  std::unordered_map<std::string, thrift::PeerSpec> peers_;

  // the socket to publish changes to kv-store
  fbzmq::Socket<ZMQ_PUB, fbzmq::ZMQ_SERVER> localPubSock_;
  fbzmq::Socket<ZMQ_PUB, fbzmq::ZMQ_SERVER> globalPubSock_;

  // the socket we use to subscribe to other KvStores
  fbzmq::Socket<ZMQ_SUB, fbzmq::ZMQ_CLIENT> peerSubSock_;

  // the socket we listen for commands on
  fbzmq::Socket<ZMQ_ROUTER, fbzmq::ZMQ_SERVER> localCmdSock_;
  fbzmq::Socket<ZMQ_ROUTER, fbzmq::ZMQ_SERVER> globalCmdSock_;

  // zmq ROUTER socket for requesting full dumps from peers
  fbzmq::Socket<ZMQ_ROUTER, fbzmq::ZMQ_CLIENT> peerSyncSock_;

  // set of peers to perform full sync from. We use exponential backoff to try
  // repetitively untill we succeeed (without overwhelming anyone with too
  // many requests).
  std::unordered_map<std::string, ExponentialBackoff<std::chrono::milliseconds>>
      peersToSyncWith_{};

  // Callback timer to get full KEY_DUMP from peersToSyncWith_
  std::unique_ptr<fbzmq::ZmqTimeout> fullSyncTimer_;

  // the serializer/deserializer helper we'll be using
  apache::thrift::CompactSerializer serializer_;

  // store keys mapped to (version, originatoId, value)
  std::unordered_map<std::string, thrift::Value> kvStore_;

  // Timer for submitting to monitor periodically
  std::unique_ptr<fbzmq::ZmqTimeout> monitorTimer_;

  // TTL count down queue
  TtlCountdownQueue ttlCountdownQueue_;

  // TTL count down timer
  std::unique_ptr<fbzmq::ZmqTimeout> ttlCountdownTimer_;

  // Data-struct for maintaining stats/counters
  fbzmq::ThreadData tData_;

  // client to interact with monitor
  std::unique_ptr<fbzmq::ZmqMonitorClient> zmqMonitorClient_;

  // Map of latest peer sync up request send to each peer
  // this is used to measure full-dump sync time between this node and each of
  // its peers
  std::unordered_map<
      std::string,
      std::chrono::time_point<std::chrono::steady_clock>>
      latestSentPeerSync_;
};

} // namespace openr
