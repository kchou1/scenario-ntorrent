/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 University of California, Los Angeles
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Authors: Spyridon (Spyros) Mastorakis <mastorakis@cs.ucla.edu>
 *          Alexander Afanasyev <alexander.afanasyev@ucla.edu>
 */

#include <ndn-cxx/data.hpp>
#include <ndn-cxx/encoding/tlv.hpp>

#include "ns3/ndnSIM-module.h"
#include "ns3/integer.h"
#include "ns3/double.h"
#include "ns3/string.h"
#include "ns3/random-variable-stream.h"
#include "ns3/nstime.h"
#include "ns3/application.h"
#include "ns3/ptr.h"
#include "ns3/callback.h"
#include "ns3/traced-callback.h"
#include "ns3/boolean.h"

#include "apps/ndn-app.hpp"
#include "NFD/rib/rib-manager.hpp"
#include "ns3/ndnSIM/helper/ndn-strategy-choice-helper.hpp"

#include <tuple>

#define MAX_PACKETS_TO_FETCH 5

namespace ns3 {
namespace ndn {

class NTorrentAdHocAppNaive : public App
{
public:
  static TypeId
  GetTypeId(void);

  NTorrentAdHocAppNaive();
  ~NTorrentAdHocAppNaive();

  virtual void
  StartApplication();

  virtual void
  StopApplication();

  virtual void
  OnInterest(shared_ptr<const Interest> interest);

  virtual void
  OnData(shared_ptr<const Data> data);

  virtual void
  SendInterest();

  virtual void
  SendInterest(const string& interestName);

private:
  void
  SendBitmap(uint32_t retransmissions = 0);

  void
  PopulateBitmap();

  void
  CreateAndSendBitmap(shared_ptr<const Interest> interest);

  std::string
  DecodeBitmap(shared_ptr<const Data> data);

  std::string
  DecodeBitmap(shared_ptr<const Interest> interest);

  void
  SendInterestForData(std::string nodeId, std::string bitmap);

  void
  SendData(Name interestName);

  void
  SendBeacon();

  void
  ResendInterestForData(Name interestName, uint8_t numberOfRetransmissions);

private:
  uint32_t m_torrentPacketNum;
  Name m_torrentPrefix;
  bool m_isTorrentProducer;
  uint32_t m_nodeId;

  Time m_beaconTimer;
  Time m_randomTimerRange;
  Time m_expirationTimer;

  Ptr<RandomVariableStream> m_random;
  Ptr<RandomVariableStream> m_randomBeacon;

  // sequence number used for bitmap Interests
  uint64_t m_seq;

  // sequence number for beacons
  uint64_t m_beaconSeq;

  // Piece scarcity data structure. It contains pairs of
  // <torrent packet sequence number, counter>
  std::list<std::pair<uint32_t, uint32_t>> m_scarcity;

  // bitmap structure (array of chars with value 1 if the peer has a packet,
  // 0 if the peer does not)
  char* m_bitmap;

  // Structure for the data a node has. It contains pairs of
  // <data packet seq number, value that indicates whether the nodes has this data packet>
  // 1 if the node has this data packet, 0 if it does not
  std::vector<std::pair<uint32_t, uint32_t>> m_downloadedData;

  // outstanding Interests (data packets for which an Interest has been sent, but a
  // data packet has not been received yet
  // tuple of <seq number, nodeid, bitmap, Interest sending event>
  std::vector<std::tuple<uint32_t, std::string, std::string, ns3::EventId>> m_outstandingInterests;

  ns3::EventId m_beaconSent;

  ns3::EventId m_bitmapSent;

  // has this peer downloaded all the data
  bool m_downloadedAllData;
};

} // namespace ndn
} // namespace ns3
