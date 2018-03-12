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

#include "ns3/ndnSIM-module.h"
#include "ns3/integer.h"
#include "ns3/string.h"
#include "apps/ndn-app.hpp"

#include "src/torrent-file.hpp"
#include "src/torrent-manager.hpp"
#include "src/util/shared-constants.hpp"
#include "src/util/io-util.hpp"

namespace ndn_ntorrent = ndn::ntorrent;

namespace ns3 {
namespace ndn {

class NTorrentConsumerApp : public App
{
public:
  static TypeId
  GetTypeId(void);
  
  NTorrentConsumerApp();
  ~NTorrentConsumerApp();

  virtual void
  StartApplication();

  virtual void
  StopApplication();

  virtual void
  OnInterest(std::shared_ptr<const Interest> interest);

  virtual void
  OnData(std::shared_ptr<const Data> contentObject);

  virtual void
  SendInterest(const string& interestName);

  virtual void
  copyTorrentFile();

private:
  void
  SendInterest();

  uint32_t m_seq; 
  uint32_t m_seqMax;
  EventId m_sendEvent;
  Time m_retxTimer;
  EventId m_retxEvent;

  Time m_offTime; 
  Name m_interestName;
  Time m_interestLifeTime;
  
  uint32_t m_namesPerSegment;
  uint32_t m_namesPerManifest;
  uint32_t m_dataPacketSize;
    
  ndn_ntorrent::TorrentFile m_initialSegment;
  std::vector<ndn_ntorrent::TorrentFile> m_torrentSegments;
  std::vector<Name> manifests;
  std::vector<Data> dataPackets;
  shared_ptr<ndn_ntorrent::InterestQueue> m_interestQueue;

};

} // namespace ndn
} // namespace ns3
