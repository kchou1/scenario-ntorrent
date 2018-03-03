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

#include "ns3/ndnSIM-module.h"
#include "ns3/integer.h"
#include "ns3/string.h"

#include "ntorrent-producer.hpp"

namespace ns3 {
namespace ndn {

namespace ntorrent_namespace = ::ndn::ntorrent;

class NTorrentProducerApp : public Application
{
public:
  static TypeId
  GetTypeId()
  {
    static TypeId tid = TypeId("NTorrentProducerApp")
      .SetParent<Application>()
      .AddConstructor<NTorrentProducerApp>()
      .AddAttribute("Prefix", "Name of the Interest(s) to be satisfied", StringValue("/"),
                    MakeNameAccessor(&NTorrentProducerApp::torrent_prefix), MakeNameChecker());

    return tid;
  }

protected:
  // inherited from Application base class.
  virtual void
  StartApplication()
  {
    m_instance.reset(new ntorrent_namespace::NTorrentProducer);
    //m_instance.setPrefix(torrent_prefix);
    //m_instance.run();
  }

  virtual void
  StopApplication()
  {
    m_instance.reset();
  }

private:
  std::unique_ptr<ntorrent_namespace::NTorrentProducer> m_instance;
  
  Name torrent_prefix;
  uint32_t m_nFiles;
  uint32_t m_nSegmentsPerFile;

  void createTorrentFile();
  void createManifests();
};

} // namespace ndn
} // namespace ns3
