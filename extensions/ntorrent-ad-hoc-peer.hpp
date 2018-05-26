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

#include "src/torrent-file.hpp"
#include "src/file-manifest.hpp"
#include "src/torrent-manager.hpp"
#include "src/util/shared-constants.hpp"
#include "src/util/simulation-constants.hpp"
#include "src/util/io-util.hpp"

#include <tuple>

#define MAX_PACKETS_TO_FETCH 5

namespace ns3 {
namespace ndn {

class NTorrentAdHocApp : public App
{
public:
  static TypeId
  GetTypeId(void);

  NTorrentAdHocApp();
  ~NTorrentAdHocApp();

  virtual void
  StartApplication();

  virtual void
  StopApplication();

  virtual void
  OnInterest(shared_ptr<const Interest> interest);

  virtual void
  copyTorrentFile();

  virtual void
  OnData(shared_ptr<const Data> data);

  virtual void
  SendInterest();

  virtual void
  SendInterest(const string& interestName);

private:
  void
  SendBeacon();

  void
  PopulateIBF();

  void
  CreateAndSendIBF(shared_ptr<const Interest> interest);

  void
  DecodeIBFAndComputeRearestPiece(shared_ptr<const Data> data, std::vector<std::tuple<Name, uint32_t, uint32_t>>* receivedIBF);

  /**
   * @brief encode IBF
   */
  template<::ndn::encoding::Tag TAG>
  size_t
  EncodeContent(::ndn::EncodingImpl<TAG>& encoder) const;

  void
  SendInterestForData(std::vector<std::tuple<Name, uint32_t, uint32_t>> receivedIBF,
                      uint32_t interestsSent = 0);

  void
  SendData(Name interestName);

private:
  uint32_t m_torrentPacketNum;
  Name m_torrentPrefix;
  bool m_isTorrentProducer;
  uint32_t m_nodeId;

  Time m_beaconTimer;
  Time m_randomTimerRange;

  Ptr<RandomVariableStream> m_random;

  // sequence number used for beacons
  uint64_t m_seq;

  // IBF-like data structure. It contains tuples of
  // <torrent packet name, nonce, counter>
  std::vector<std::tuple<Name, uint32_t, uint32_t>> m_IBF;

  // Structure for the data a node has. It contains pairs of
  // <data packet name, value that indicates whether the nodes has this data packet>
  // 1 if the node has this data packet, 0 if it does not
  std::vector<std::pair<Name, uint32_t>> m_downloadedData;

  // store the encoded IBF block
  ::ndn::Block m_encodedIBF;

  // boolean to check whether the IBF has changed since the last time
  // that was encoded
  bool m_IBFhasChanged;

  // outstanding Interests (data packets for which an Interest has been sent, but a
  // data packet has not been received yet
  // std::vector<std::tuple<Name, std::vector<std::tuple<Name, uint32_t, uint32_t>>, uint32_t>> m_outstandingInterests;
};

} // namespace ndn
} // namespace ns3
