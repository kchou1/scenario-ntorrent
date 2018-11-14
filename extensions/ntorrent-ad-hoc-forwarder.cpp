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

#include "ntorrent-ad-hoc-forwarder.hpp"

NS_LOG_COMPONENT_DEFINE("NTorrentAdHocForwarder");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(NTorrentAdHocForwarder);

TypeId
NTorrentAdHocForwarder::GetTypeId(void)
{
    static TypeId tid = TypeId("NTorrentAdHocForwarder")
      .SetParent<Application>()
      .AddConstructor<NTorrentAdHocForwarder>()
      // Node id
      .AddAttribute("NodeId", "Id of the current node", IntegerValue(-1),
                    MakeIntegerAccessor(&NTorrentAdHocForwarder::m_nodeId), MakeIntegerChecker<int32_t>())
      // Random timer between 0 and RandomTimerRange
      .AddAttribute("RandomTimerRange", "Random Timer Range", StringValue("1s"),
                    MakeTimeAccessor(&NTorrentAdHocForwarder::m_randomTimerRange), MakeTimeChecker())
                    // Random timer between 0 and RandomTimerRange
      .AddAttribute("ExpirationTimer", "Timer for an outstanding Interest to expire", StringValue("30ms"),
                    MakeTimeAccessor(&NTorrentAdHocForwarder::m_expirationTimer), MakeTimeChecker())
      .AddAttribute("ForwardProbability", "Probability packet is forwarded.", IntegerValue(0),
                    MakeIntegerAccessor(&NTorrentAdHocForwarder::m_forwardProbability), MakeIntegerChecker<int32_t>());
    return tid;
}

NTorrentAdHocForwarder::NTorrentAdHocForwarder()
{
  NS_ASSERT(m_nodeId != -1);
}

NTorrentAdHocForwarder::~NTorrentAdHocForwarder()
{
}

void
NTorrentAdHocForwarder::StartApplication()
{
    NS_LOG_DEBUG("Starting application for node: " << m_nodeId);
    App::StartApplication();
    ndn::FibHelper::AddRoute(GetNode(), "movie1", m_face, 0);
    ndn::FibHelper::AddRoute(GetNode(), "bitmap", m_face, 0);
    ndn::FibHelper::AddRoute(GetNode(), "beacon", m_face, 0);

    m_random = CreateObject<UniformRandomVariable>();
    m_random->SetAttribute("Min", DoubleValue(0.0));
    m_random->SetAttribute("Max", DoubleValue(m_randomTimerRange.GetMilliSeconds()));
}

void
NTorrentAdHocForwarder::StopApplication()
{
    App::StopApplication();
}

void
NTorrentAdHocForwarder::SendInterest()
{
}

void
NTorrentAdHocForwarder::ForwardInterest(shared_ptr<const Interest> interest)
{
  Name interestName = interest->getName();
  shared_ptr<Interest> interestForwarded = make_shared<Interest>(interestName);
  NS_LOG_DEBUG("Forwarding Interest. (" + interestName.toUri() + ")");

  m_transmittedInterests(interestForwarded, this, m_face);
  m_appLink->onReceiveInterest(*interestForwarded);
}

void
NTorrentAdHocForwarder::ForwardData(shared_ptr<const Data> data)
{
  m_transmittedDatas(data, this, m_face);
  m_appLink->onReceiveData(*data);
}

void
NTorrentAdHocForwarder::OnInterest(shared_ptr<const Interest> interest)
{
  ndn::App::OnInterest(interest);

  Name interestName = interest->getName();

  // Decide whether to forward or not
  unsigned int chosenProb = rand() % 100 + 1;

  NS_LOG_DEBUG("Deciding whether to forward Interest or not. [" + interestName.toUri() + "]");
  if (chosenProb >= m_forwardProbability) {
    // Forward Interest
    Simulator::Schedule(ns3::MilliSeconds(m_random->GetValue()), &NTorrentAdHocForwarder::ForwardData, this, interest);
  } else {
    NS_LOG_DEBUG("Nothing to be Done to Interest: " + interestName.toUri());
  }
}

void
NTorrentAdHocForwarder::OnData(shared_ptr<const Data> data)
{
  // Decide whether to forward or not
  unsigned int chosenProb = rand() % 100 + 1;

  NS_LOG_DEBUG("Deciding whether to forward Data or not. [" + data->getName() + "]");
  if (chosenProb >= m_forwardProbability) {
    Simulator::Schedule(ns3::MilliSeconds(m_random->GetValue()), &NTorrentAdHocForwarder::ForwardData, this, data);
  } else {
    NS_LOG_DEBUG("Nothing to be Done to Data: " + data->getName());
  }

}

} // namespace ndn
} // namespace ns3
