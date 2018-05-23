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
#include "ns3/string.h"
#include "apps/ndn-app.hpp"
#include "NFD/rib/rib-manager.hpp"
#include "ns3/ndnSIM/helper/ndn-strategy-choice-helper.hpp"

#include "src/torrent-file.hpp"
#include "src/file-manifest.hpp"
#include "src/torrent-manager.hpp"
#include "src/util/shared-constants.hpp"
#include "src/util/simulation-constants.hpp"
#include "src/util/io-util.hpp"

namespace ndn_ntorrent = ndn::ntorrent;
namespace nfd_rib = nfd::rib;
namespace nfd_fw = nfd::fw;

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
  std::vector<ndn_ntorrent::TorrentFile> m_torrentSegments;
  std::vector<ndn_ntorrent::FileManifest> manifests;
  std::vector<Data> dataPackets;
  std::vector<ndn::Name> torrent_list;
                
  nfd_rib::Rib m_rib;

  //Specific to consumer
  uint32_t m_seq; 
  Name m_interestName;
  Time m_interestLifeTime;
  
  ndn_ntorrent::TorrentFile m_initialSegment;
  
  //The 3 below variables aren't needed by the consumer
  //They are just there to "generate" the torrent
  //(to simulate copying in the real world)
  uint32_t m_namesPerSegment;
  uint32_t m_namesPerManifest;
  uint32_t m_dataPacketSize;
};

} // namespace ndn
} // namespace ns3
