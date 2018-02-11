/**
 * Copyright (c) 2014-present, Facebook, Inc.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

namespace cpp2 openr.thrift
namespace py openr.Platform

include "IpPrefix.thrift"
/**
 * We provide simple API to publish link/address/neighbor updating events
 * through PUB-SUB mechanism to all of its subscriber modules in OpenR
 */
struct LinkEntry {
  1: string ifName;
  2: i64 ifIndex;
  3: bool isUp;
  4: i64 weight = 1; // used for weighted ecmp
}

struct AddrEntry {
  1: string ifName;
  2: IpPrefix.IpPrefix ipPrefix;
  3: bool isValid;
}

struct NeighborEntry {
  1: string ifName;
  2: IpPrefix.BinaryAddress destination;
  3: string linkAddr;
  4: bool isReachable;
}

struct Link {
  1: i64 ifIndex;
  2: bool isUp;
  3: list<IpPrefix.IpPrefix> networks;
  4: string ifName;
  5: i64 weight = 1; // used for weighted ecmp
}

/**
 * Enum to keep track of Client name to Client-ID mapping. Indicates which
 * client-ids are used and which are available to use.
 */
enum FibClient {
  // OpenR Client
  OPENR = 786,

  // Some Placeholder Clients
  CLIENT_1 = 1,
  CLIENT_2 = 2,
  CLIENT_3 = 3,
  CLIENT_4 = 4,
  CLIENT_5 = 5,
}

/**
 * Message sent over to subscriber of Platform Event.
 * eventType to indicate type of netlink event to be updated
 * eventData to indicate exact object entry to be updated
 * Notice: when sending out PlatformEvent make sure to send multi part messages:
 * part1: header to indicate event type,
 * which is 2 byte of PlatformEventType cast to unsigned int
 * part2: real message
 */
 enum PlatformEventType {
   /*
    * Command type to publish changes of link/address/neighbor
    */
   LINK_EVENT = 1,
   ADDRESS_EVENT = 2,
   NEIGHBOR_EVENT = 3,
 }

struct PlatformEvent {
  1: PlatformEventType eventType;
  2: binary eventData;
}

exception PlatformError {
  1: string message
} ( message = "message" )

/**
 * Thrift Service API definitions for on-box system information like links,
 * addresses and neighbors. OpenR leverages links and address information as
 * a part of link discovery and uses it to perform neighbor discovery on
 * retrieved links. There is also PUB/SUB mechanism over which updates can be
 * relayed to OpenR in realtime.
 */
service SystemService {
  /**
   * SystemService client can query the following items:
   * 1. query all links keyed by interface names
   * 2. query all reachable neighbors
   */
  list<Link> getAllLinks(
  ) throws (1: PlatformError error)

  list<NeighborEntry> getAllNeighbors()
    throws (1: PlatformError error)
}

/**
 * Common status reporting mechanism across all services
 */
enum ServiceStatus {
  DEAD = 0,
  STARTING = 1,
  ALIVE = 2,
  STOPPING = 3,
  STOPPED = 4,
  WARNING = 5,
}

/**
 * Interface to on-box Fib.
 */
service FibService {
  void addUnicastRoute(
    1: i16 clientId,
    2: IpPrefix.UnicastRoute route,
  ) throws (1: PlatformError error)

  void deleteUnicastRoute(
    1: i16 clientId,
    2: IpPrefix.IpPrefix prefix,
  ) throws (1: PlatformError error)

  void addUnicastRoutes(
    1: i16 clientId,
    2: list<IpPrefix.UnicastRoute> routes,
  ) throws (1: PlatformError error)

  void deleteUnicastRoutes(
    1: i16 clientId,
    2: list<IpPrefix.IpPrefix> prefixes,
  ) throws (1: PlatformError error)

  void syncFib(
    1: i16 clientId,
    2: list<IpPrefix.UnicastRoute> routes,
  ) throws (1: PlatformError error)

  /**
   * DEPRECATED ... Use `aliveSince` API instead
   * openr should periodically call this to let Fib know that it is alive
   */
  i64 periodicKeepAlive(
    1: i16 clientId,
  )

  /**
   * Returns the unix time that the service has been running since
   */
  i64 aliveSince() (priority = 'IMPORTANT')

  /**
   * Get the status of this service
   */
  ServiceStatus getStatus() (priority = 'IMPORTANT')

  /**
   * Get number of routes
   */
  map<string, i64> getCounters()

  // Retreive list of routes per client
  list<IpPrefix.UnicastRoute> getRouteTableByClient(
    1: i16 clientId
  ) throws (1: PlatformError error)
}
