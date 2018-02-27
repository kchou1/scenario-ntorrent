/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2015  Regents of the University of California.
 *
 * This file is part of ndnSIM. See AUTHORS for complete list of ndnSIM authors and
 * contributors.
 *
 * ndnSIM is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * ndnSIM is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ndnSIM, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Spyridon (Spyros) Mastorakis <mastorakis@cs.ucla.edu>
 */

#include "ns3/ndnSIM-module.h"
#include "ns3/integer.h"
#include "ns3/string.h"
#include "ns3/boolean.h"

#include "ntorrent.hpp"

namespace ns3 {
namespace ndn {

class NTorrentApp : public Application
{
public:
  static TypeId
  GetTypeId()
  {
    static TypeId tid = TypeId("NTorrentApp")
      .SetParent<Application>()
      .AddConstructor<NTorrentApp>()
      .AddAttribute("SyncPrefix", "Sync Prefix", StringValue("/"),
                    MakeNameAccessor(&NTorrentApp::m_syncPrefix), MakeNameChecker())
      .AddAttribute("UserPrefix", "User Prefix", StringValue("/"),
                    MakeNameAccessor(&NTorrentApp::m_userPrefix), MakeNameChecker())
      .AddAttribute("RoutingPrefix", "Routing Prefix", StringValue("/"),
                    MakeNameAccessor(&NTorrentApp::m_routingPrefix), MakeNameChecker())
      .AddAttribute("MinNumberMessages", "Minimum number of messages", IntegerValue(1),
                    MakeIntegerAccessor(&NTorrentApp::m_minNumberMessages), MakeIntegerChecker<int32_t>())
      .AddAttribute("PeriodicPublishing", "Periodic data publishing", BooleanValue(false),
                    MakeBooleanAccessor(&NTorrentApp::m_periodicPublishing), MakeBooleanChecker())
      .AddAttribute("MaxNumberMessages", "Maximum number of messages", IntegerValue(2),
                    MakeIntegerAccessor(&NTorrentApp::m_maxNumberMessages), MakeIntegerChecker<int32_t>());

    return tid;
  }

protected:
  // inherited from Application base class.
  virtual void
  StartApplication()
  {
    m_instance.reset(new ::ndn::NTorrent(m_minNumberMessages, m_maxNumberMessages));
    m_instance->setSyncPrefix(m_syncPrefix);
    m_instance->setUserPrefix(m_userPrefix);
    m_instance->setRoutingPrefix(m_routingPrefix);
    m_instance->initializeSync();
    if (m_periodicPublishing) {
      m_instance->runPeriodically();
    }
    else {
      m_instance->run();
    }
  }

  virtual void
  StopApplication()
  {
    m_instance.reset();
  }

private:
  std::unique_ptr<::ndn::NTorrent> m_instance;
  Name m_syncPrefix;
  Name m_userPrefix;
  Name m_routingPrefix;
  int m_minNumberMessages;
  int m_maxNumberMessages;
  bool m_periodicPublishing;
};

} // namespace ndn
} // namespace ns3
