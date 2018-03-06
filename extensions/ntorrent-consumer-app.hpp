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
#include "utils/ndn-rtt-estimator.hpp"

#include "ntorrent-consumer.hpp"

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
  OnData(shared_ptr<const Data> data);

  virtual void
  OnNack(shared_ptr<const lp::Nack> nack);

  virtual void
  OnTimeout(uint32_t sequenceNumber);

  void
  SendPacket();

  virtual void
  WillSendOutInterest(uint32_t sequenceNumber);

protected:
  virtual void
  StartApplication();

  virtual void
  StopApplication();

  virtual void
  ScheduleNextPacket();

  void
  CheckRetxTimeout();

  void
  SetRetxTimer(Time retxTimer);

  Time
  GetRetxTimer() const;

private:
  std::unique_ptr<::ndn::NTorrentConsumer> m_instance;
  
  Ptr<UniformRandomVariable> m_rand; 

uint32_t m_seq; 
uint32_t m_seqMax;
EventId m_sendEvent;
Time m_retxTimer;
EventId m_retxEvent;

Ptr<RttEstimator> m_rtt;

Time m_offTime; 
Name m_interestName;
Time m_interestLifeTime;

struct RetxSeqsContainer : public std::set<uint32_t> {
}; 

RetxSeqsContainer m_retxSeqs; 

struct SeqTimeout {
    SeqTimeout(uint32_t _seq, Time _time)
      : seq(_seq)
      , time(_time)
    {
    }

    uint32_t seq;
    Time time;
};

class i_seq {
  };
  class i_timestamp {
};

struct SeqTimeoutsContainer
    : public boost::multi_index::
        multi_index_container<SeqTimeout,
                              boost::multi_index::
                                indexed_by<boost::multi_index::
                                             ordered_unique<boost::multi_index::tag<i_seq>,
                                                            boost::multi_index::
                                                              member<SeqTimeout, uint32_t,
                                                                     &SeqTimeout::seq>>,
                                           boost::multi_index::
                                             ordered_non_unique<boost::multi_index::
                                                                  tag<i_timestamp>,
                                                                boost::multi_index::
                                                                  member<SeqTimeout, Time,
                                                                         &SeqTimeout::time>>>> {
};

SeqTimeoutsContainer m_seqTimeouts; ///< \brief multi-index for the set of SeqTimeout structs

  SeqTimeoutsContainer m_seqLastDelay;
  SeqTimeoutsContainer m_seqFullDelay;
  std::map<uint32_t, uint32_t> m_seqRetxCounts;

  TracedCallback<Ptr<App> /* app */, uint32_t /* seqno */, Time /* delay */, int32_t /*hop count*/>
    m_lastRetransmittedInterestDataDelay;
  TracedCallback<Ptr<App> /* app */, uint32_t /* seqno */, Time /* delay */,
uint32_t /*retx count*/, int32_t /*hop count*/> m_firstInterestDataDelay;

};

} // namespace ndn
} // namespace ns3
