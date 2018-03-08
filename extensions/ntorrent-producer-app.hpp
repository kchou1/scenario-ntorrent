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
#include "apps/ndn-app.hpp"

#include "src/torrent-file.hpp"
#include "src/util/shared-constants.hpp"

namespace ns3 {
namespace ndn {

class NTorrentProducerApp : public App
{
public:
  static TypeId
  GetTypeId(void);
  
  NTorrentProducerApp();
  ~NTorrentProducerApp();

  void
  OnInterest(shared_ptr<const Interest> interest);

  void
  generateTorrentFile();

protected:
  virtual void
  StartApplication();

  virtual void
  StopApplication();

private:
  Name m_prefix;
  Name m_postfix;
  uint32_t m_virtualPayloadSize;
  Time m_freshness;
  uint32_t m_signature;
  Name m_keyLocator;

  uint32_t m_nFiles;
  uint32_t m_nSegmentsPerFile;

};

} // namespace ndn
} // namespace ns3
