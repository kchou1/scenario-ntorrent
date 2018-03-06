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

#include "ntorrent-consumer-app.hpp"

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(NTorrentConsumerApp);

TypeId
NTorrentConsumerApp::GetTypeId(void)
{
    static TypeId tid = TypeId("NTorrentConsumerApp")
      .SetParent<Application>()
      .AddConstructor<NTorrentConsumerApp>()
      .AddAttribute("StartSeq", "Initial sequence number", IntegerValue(0),
                    MakeIntegerAccessor(&NTorrentConsumerApp::m_seq), MakeIntegerChecker<int32_t>())

      .AddAttribute("Prefix", "Name of the Interest", StringValue("/"),
                    MakeNameAccessor(&NTorrentConsumerApp::m_interestName), MakeNameChecker())
      .AddAttribute("LifeTime", "LifeTime for interest packet", StringValue("2s"),
                    MakeTimeAccessor(&NTorrentConsumerApp::m_interestLifeTime), MakeTimeChecker())

      .AddAttribute("RetxTimer",
                    "Timeout defining how frequent retransmission timeouts should be checked",
                    StringValue("50ms"),
                    MakeTimeAccessor(&NTorrentConsumerApp::GetRetxTimer, &NTorrentConsumerApp::SetRetxTimer),
                    MakeTimeChecker())

      .AddTraceSource("LastRetransmittedInterestDataDelay",
                      "Delay between last retransmitted Interest and received Data",
                      MakeTraceSourceAccessor(&NTorrentConsumerApp::m_lastRetransmittedInterestDataDelay),
                      "ns3::ndn::NTorrentConsumerApp::LastRetransmittedInterestDataDelayCallback")

      .AddTraceSource("FirstInterestDataDelay",
                      "Delay between first transmitted Interest and received Data",
                      MakeTraceSourceAccessor(&NTorrentConsumerApp::m_firstInterestDataDelay),
					  "ns3::ndn::NTorrentConsumerApp::FirstInterestDataDelayCallback");
    return tid;
}

NTorrentConsumerApp::NTorrentConsumerApp()
{
}

NTorrentConsumerApp::~NTorrentConsumerApp()
{
}

void
NTorrentConsumerApp::StartApplication()
{
    m_instance.reset(new ::ndn::NTorrentConsumer);
    m_instance->run();
}

void
NTorrentConsumerApp::StopApplication()
{
    m_instance.reset();
}

void
NTorrentConsumerApp::OnData(shared_ptr<const Data> data)
{
}

void
NTorrentConsumerApp::OnNack(shared_ptr<const lp::Nack> nack)
{
}

void
NTorrentConsumerApp::OnTimeout(uint32_t sequenceNumber)
{
}

void
NTorrentConsumerApp::WillSendOutInterest(uint32_t sequenceNumber)
{
}

void
NTorrentConsumerApp::SendPacket()
{
    if(!m_active)
        return;
    //NS_LOG_FUNCTION_NOARGS();
    uint32_t seq = std::numeric_limits<uint32_t>::max(); // invalid
    
    while (m_retxSeqs.size()) {
        seq = *m_retxSeqs.begin();
        m_retxSeqs.erase(m_retxSeqs.begin());
        break;
    }
   if (seq == std::numeric_limits<uint32_t>::max()) {
       if (m_seqMax != std::numeric_limits<uint32_t>::max()) {
         if (m_seq >= m_seqMax) {
           return; // we are totally done
         }
       }
   
       seq = m_seq++;
     } 
  
  shared_ptr<Name> nameWithSequence = make_shared<Name>(m_interestName);
  nameWithSequence->appendSequenceNumber(seq);
  //

  // shared_ptr<Interest> interest = make_shared<Interest> ();
  shared_ptr<Interest> interest = make_shared<Interest>();
  interest->setNonce(m_rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
  interest->setName(*nameWithSequence);
  time::milliseconds interestLifeTime(m_interestLifeTime.GetMilliSeconds());
  interest->setInterestLifetime(interestLifeTime);

  // NS_LOG_INFO ("Requesting Interest: \n" << *interest);
  //NS_LOG_INFO("> Interest for " << seq);

  WillSendOutInterest(seq);

  m_transmittedInterests(interest, this, m_face);
  m_appLink->onReceiveInterest(*interest);

  ScheduleNextPacket(); 
}

void
NTorrentConsumerApp::ScheduleNextPacket()
{
    m_sendEvent = Simulator::Schedule(Seconds(0.0), &NTorrentConsumerApp::SendPacket, this);
}

Time
NTorrentConsumerApp::GetRetxTimer() const
{
    return m_retxTimer;
}

void
NTorrentConsumerApp::CheckRetxTimeout()
{
}

void
NTorrentConsumerApp::SetRetxTimer(Time retxTimer)
{

}

} // namespace ndn
} // namespace ns3
