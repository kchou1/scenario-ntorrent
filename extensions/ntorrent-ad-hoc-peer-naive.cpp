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

#include "ntorrent-ad-hoc-peer-naive.hpp"

NS_LOG_COMPONENT_DEFINE("NTorrentAdHocAppNaive");

// custom comparator for scarcity pairs
bool compare_pairs (const std::pair<uint32_t, uint32_t> first, const std::pair<uint32_t, uint32_t> second)
{
  return (std::get<1>(first) > std::get<1>(second));
}

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(NTorrentAdHocAppNaive);

TypeId
NTorrentAdHocAppNaive::GetTypeId(void)
{
    static TypeId tid = TypeId("NTorrentAdHocAppNaive")
      .SetParent<Application>()
      .AddConstructor<NTorrentAdHocAppNaive>()
      // Making things simple: right away I specify the total number of torrent packets
      .AddAttribute("NumberOfPackets", "Number of torrent packets (in total)", IntegerValue(0),
                    MakeIntegerAccessor(&NTorrentAdHocAppNaive::m_torrentPacketNum), MakeIntegerChecker<int32_t>())
      // Prefix of torrent packets
      .AddAttribute("TorrentPrefix", "Torrent prefix", StringValue("/"),
                    MakeNameAccessor(&NTorrentAdHocAppNaive::m_torrentPrefix), MakeNameChecker())
      // Node id
      .AddAttribute("NodeId", "Id of the current node", IntegerValue(-1),
                    MakeIntegerAccessor(&NTorrentAdHocAppNaive::m_nodeId), MakeIntegerChecker<int32_t>())
      // Send a new beacon every BeaconTimer seconds + random timer between 0 and RandomTimerRange
      .AddAttribute("BeaconTimer", "Periodic timer for beacons", StringValue("1s"),
                    MakeTimeAccessor(&NTorrentAdHocAppNaive::m_beaconTimer), MakeTimeChecker())
      // Random timer between 0 and RandomTimerRange
      .AddAttribute("RandomTimerRange", "Random Timer Range", StringValue("1s"),
                    MakeTimeAccessor(&NTorrentAdHocAppNaive::m_randomTimerRange), MakeTimeChecker())
                    // Random timer between 0 and RandomTimerRange
      .AddAttribute("ExpirationTimer", "Timer for an outstanding Interest to expire", StringValue("30ms"),
                    MakeTimeAccessor(&NTorrentAdHocAppNaive::m_expirationTimer), MakeTimeChecker())
      // Is this node the original torrent producer or just a peer?
      .AddAttribute("TorrentProducer", "Has this node generated the torrent?", BooleanValue(false),
                    MakeBooleanAccessor(&NTorrentAdHocAppNaive::m_isTorrentProducer), MakeBooleanChecker())
      .AddAttribute("PureForwarder", "Is this node a pure forwarding node?", BooleanValue(false),
                    MakeBooleanAccessor(&NTorrentAdHocAppNaive::m_isPureForwarder), MakeBooleanChecker())
      .AddAttribute("ForwardProbability", "Probability packet is forwarded.", IntegerValue(0),
                    MakeIntegerAccessor(&NTorrentAdHocAppNaive::m_forwardProbability), MakeIntegerChecker<int32_t>());
    return tid;
}

NTorrentAdHocAppNaive::NTorrentAdHocAppNaive()
{
  NS_ASSERT(m_nodeId != -1);
  m_seq = 0;
  m_beaconSeq = 0;
  m_downloadedAllData = false;
}

NTorrentAdHocAppNaive::~NTorrentAdHocAppNaive()
{
}

void
NTorrentAdHocAppNaive::StartApplication()
{
    NS_LOG_DEBUG("Starting application for node: " << m_nodeId);
    App::StartApplication();
    ndn::FibHelper::AddRoute(GetNode(), m_torrentPrefix, m_face, 0);
    ndn::FibHelper::AddRoute(GetNode(), "bitmap", m_face, 0);
    ndn::FibHelper::AddRoute(GetNode(), "beacon", m_face, 0);

    // malloc to dynamically allocate the bitmap memory based on the
    // number of torrent pieces
    // The array will be initialized in PopulateBitmap
    m_bitmap = (char*) malloc(sizeof(char) * m_torrentPacketNum);

    PopulateBitmap();

    m_random = CreateObject<UniformRandomVariable>();
    m_random->SetAttribute("Min", DoubleValue(0.0));
    m_random->SetAttribute("Max", DoubleValue(m_randomTimerRange.GetMilliSeconds()));


    m_randomBeacon = CreateObject<UniformRandomVariable>();
    m_randomBeacon->SetAttribute("Min", DoubleValue(0.0));
    m_randomBeacon->SetAttribute("Max", DoubleValue(m_beaconTimer.GetMilliSeconds()));

    if (!m_isPureForwarder) {
        m_beaconSent = Simulator::Schedule(ns3::MilliSeconds(m_randomBeacon->GetValue() + 2000), &NTorrentAdHocAppNaive::SendBeacon, this);
    }
}

void
NTorrentAdHocAppNaive::StopApplication()
{
    App::StopApplication();

    // delete the bitmap memory
    delete(m_bitmap);
}

void
NTorrentAdHocAppNaive::SendInterest()
{
}

void
NTorrentAdHocAppNaive::SendInterest(const string& interestName)
{

}

void
NTorrentAdHocAppNaive::ForwardInterest(shared_ptr<const Interest> interest)
{
  m_transmittedInterests(interest, this, m_face);
  m_appLink->onReceiveInterest(*interest);
}

void
NTorrentAdHocAppNaive::ForwardData(shared_ptr<const Data> data)
{
  m_transmittedDatas(data, this, m_face);
  m_appLink->onReceiveData(*data);
}

void
NTorrentAdHocAppNaive::OnInterest(shared_ptr<const Interest> interest)
{
    ndn::App::OnInterest(interest);
    // Check what kind of Interest this is: 1) Beacon, 2) Bitmap, or
    // 3) Interest for torrent data
    Name interestName = interest->getName();

    if (m_isPureForwarder) {
      // Decide with some probability. If forward, choose random delay else nothing.
      NS_LOG_DEBUG("Deciding whether to forward or not. (Interest)");
      unsigned int prob = rand() % 100 + 1;
      // NS_LOG_DEBUG("Chosen Probability: " + std::to_string(prob));
      if (prob <= m_forwardProbability) { // Forward
        // Forward the Interest with random delay
        NS_LOG_DEBUG("Forwarding Interest. (" + interestName.toUri() + ")");
        Simulator::Schedule(ns3::MilliSeconds(m_random->GetValue()), &NTorrentAdHocAppNaive::ForwardInterest, this, interest);
      } else { // Don't Forward
        NS_LOG_DEBUG("Not Forwarding Interest. (" + interestName.get(0).toUri() + ")");
      }
    } 
    else if (interestName.get(0).toUri() == "beacon") {
      NS_LOG_DEBUG("Received beacon: " << interestName.toUri());
      // if a beacon sending event has been scheduled, cancel it
      if (m_beaconSent.IsRunning() && !m_downloadedAllData) {
        Simulator::Cancel(m_beaconSent);
      }
      // if the interest is a beacon, send your bitmap
      m_bitmapSent = Simulator::Schedule(ns3::MilliSeconds(m_random->GetValue()), &NTorrentAdHocAppNaive::SendBitmap, this, 0);
    }
    else if (interestName.get(0).toUri() == "bitmap") {
      // this is a bitmap
      NS_LOG_DEBUG("Received bitmap: " << interestName.toUri());
      // if a beacon sending event has been scheduled, cancel it
      if (m_beaconSent.IsRunning() && !m_downloadedAllData) {
        Simulator::Cancel(m_beaconSent);
      }
      // Decode the bitmap of the neighbor
      std::string bitmap = DecodeBitmap(interest);
      Simulator::Schedule(ns3::MilliSeconds(m_random->GetValue()), &NTorrentAdHocAppNaive::CreateAndSendBitmap, this, interest);
      if (!m_scarcity.empty())
        Simulator::Schedule(ns3::MilliSeconds(m_random->GetValue()), &NTorrentAdHocAppNaive::SendInterestForData, this, interestName.get(2).toUri(), bitmap);
    }
    else {
      // this is an Interest for torrent data
      if (std::get<1>(m_downloadedData[interestName.get(-1).toSequenceNumber()]) == 1) {
        Simulator::Schedule(ns3::MilliSeconds(m_random->GetValue()), &NTorrentAdHocAppNaive::SendData, this, interestName);
      }
    }

}

void
NTorrentAdHocAppNaive::OnData(shared_ptr<const Data> data)
{
    if (m_isPureForwarder) {
      // Decide with some probability. If forward, choose random delay else nothing.
      NS_LOG_DEBUG("Deciding whether to forward or not. (Data)");
      unsigned int prob = rand() % 100 + 1;
      // NS_LOG_DEBUG("Chosen Probability: " + std::to_string(prob));
      if (prob <= m_forwardProbability) { // Forward
        // Forward the Interest with random delay
        NS_LOG_DEBUG("Forwarding Data. (" + data->getName().toUri() + ")");
        Simulator::Schedule(ns3::MilliSeconds(m_random->GetValue()), &NTorrentAdHocAppNaive::ForwardData, this, data);
      } else { // Don't Forward
        NS_LOG_DEBUG("Not Forwarding Data. (" + data->getName().toUri() + ")");
      }
    // 2 cases: 1. this is an IBF packet, or 2. this is torent data packet
    } else if (data->getName().get(0).toUri() == "bitmap") {
    // received a bitmap
    if (m_bitmapSent.IsRunning()) {
      Simulator::Cancel(m_bitmapSent);
    }
    NS_LOG_DEBUG("Received Bitmap in data packet: " << data->getName().toUri());
    std::string bitmap = DecodeBitmap(data);
    if (!m_scarcity.empty())
      Simulator::Schedule(ns3::MilliSeconds(m_random->GetValue()), &NTorrentAdHocAppNaive::SendInterestForData, this, data->getName().get(2).toUri(), bitmap);
    else {
      // if no other beacon event is running, schedule one
      if (!m_beaconSent.IsRunning())
        m_beaconSent = Simulator::Schedule(ns3::MilliSeconds(m_randomBeacon->GetValue() + 2000), &NTorrentAdHocAppNaive::SendBeacon, this);
    }
  }
  else {
    // Logic for receiving a data packet
    NS_LOG_DEBUG("Received torrent data: " << data->getName().toUri());
    if (m_scarcity.empty()) {
      // if we have downloaded all the torrent data, avoid doing all the rest
      return;
    }
    if (std::get<1>(m_downloadedData[data->getName().get(-1).toSequenceNumber()]) == 0) {
      m_downloadedData[data->getName().get(-1).toSequenceNumber()].second = 1;
    }
    // cancel retransmission, erase scarcity entry
    std::string nodeId;
    std::string bitmap = "\0";
    for (auto it = m_outstandingInterests.begin(); it != m_outstandingInterests.end(); it++) {
      if (data->getName().get(-1).toSequenceNumber() == std::get<0>(*it)) {
        nodeId = std::get<1>(*it);
        bitmap = std::get<2>(*it);
        Simulator::Cancel(std::get<3>(*it));

        m_outstandingInterests.erase(it);
        break;
      }
    }

    // erase scarcity entry
    for (auto it = m_scarcity.begin(); it != m_scarcity.end(); it++) {
      if (it->first == data->getName().get(-1).toSequenceNumber()) {
        m_scarcity.erase(it);
        break;
      }
    }

    if (m_scarcity.empty()) {
      // we just finished downloading all the data
      m_downloadedAllData = true;
      // NS_LOG_DEBUG("Finished downloading torrent data: " << Simulator::Now().GetMilliSeconds() / 1000.0 << " sec");
      std::cerr << "Finished downloading torrent data: " << Simulator::Now().GetMilliSeconds() / 1000.0 << " sec" << std::endl;
    }

    // update your own bitmap
    m_bitmap[data->getName().get(-1).toSequenceNumber()] = 1;

    // Send next Interest for data
    if (bitmap != "\0")
      Simulator::Schedule(ns3::MilliSeconds(m_random->GetValue()), &NTorrentAdHocAppNaive::SendInterestForData, this, nodeId, bitmap);
  }
}

void
NTorrentAdHocAppNaive::SendBitmap(uint32_t retransmissions)
{
  // if we have sent the bitmap 3 times, but still no response
  // then fall back to beacon mode again
  if (retransmissions == 3) {
    if (!m_beaconSent.IsRunning())
      m_beaconSent = Simulator::Schedule(ns3::MilliSeconds(m_randomBeacon->GetValue() + 2000), &NTorrentAdHocAppNaive::SendBeacon, this);
    return;
  }
  // Send a bitmap
  Name beaconName = Name("bitmap" + m_torrentPrefix.toUri() + "/node" + std::to_string(m_nodeId));
  beaconName.append((uint8_t*) m_bitmap, m_torrentPacketNum);
  beaconName.appendSequenceNumber(m_seq);
  m_seq++;
  NS_LOG_DEBUG("Sending bitmap, time " << retransmissions + 1 << " name: " << beaconName.toUri());

  shared_ptr<Interest> bitmap = make_shared<Interest>(beaconName);

  m_transmittedInterests(bitmap, this, m_face);
  m_appLink->onReceiveInterest(*bitmap);

  m_bitmapSent = Simulator::Schedule(ns3::MilliSeconds(m_random->GetValue()), &NTorrentAdHocAppNaive::SendBitmap, this, retransmissions + 1);
}

void
NTorrentAdHocAppNaive::PopulateBitmap()
{
  NS_LOG_DEBUG("Populate Bitmap with data");
  // loop through all the data packets of the torrent
  for (auto i = 0; i < m_torrentPacketNum; i++) {
    // create something like a bitmap for each data packet that I have
    // each character (1-byte) will be either 1 if I have the data packet,
    // or 0 if I do not
    if (m_isTorrentProducer) {
      // if this is the original torrent producer
      m_bitmap[i] = 1;
      m_downloadedData.push_back(std::make_pair(i, 1));
    }
    else {
      // if this is not the original producer, it does not have data initially
      m_bitmap[i] = 0;
      m_scarcity.push_back(std::make_pair(i, 1));
      m_downloadedData.push_back(std::make_pair(i, 0));
    }
  }
}

void
NTorrentAdHocAppNaive::CreateAndSendBitmap(shared_ptr<const Interest> interest)
{
  shared_ptr<Data> data = make_shared<Data>(interest->getName());

  data->setContentType(::ndn::tlv::ContentType_Blob);
  data->setContent((uint8_t*) m_bitmap, m_torrentPacketNum);

  Signature signature;
  SignatureInfo signatureInfo(static_cast< ::ndn::tlv::SignatureTypeValue>(255));

  signature.setInfo(signatureInfo);
  signature.setValue(::ndn::makeNonNegativeIntegerBlock(::ndn::tlv::SignatureValue, 0));

  data->setSignature(signature);

  NS_LOG_INFO("Sending back Bitmap in Data packet: " << interest->getName().toUri());

  // to create real wire encoding
  data->wireEncode();

  m_transmittedDatas(data, this, m_face);
  m_appLink->onReceiveData(*data);

}

std::string
NTorrentAdHocAppNaive::DecodeBitmap(shared_ptr<const Data> data)
{
  // First, decode the received bitmap
  std::string bitmap;
  ::ndn::Block contentBlock = data->getContent();
  for (auto it = contentBlock.value_begin(); it != contentBlock.value_end(); it++) {
    bitmap.append(std::to_string(*it));
  }

  // Then, update the local piece scarcity knowledge
  // search for all the pieces the peer does not have in the received
  // bitmap, and add 1 to the piece counter if the peer that sent the bitmap
  // does not have the piece as well
  for (auto i = 0; i < m_torrentPacketNum; i++) {
    if (m_downloadedData[i].second == 0) {
      if (bitmap[i] == 0) {
        for (auto it = m_scarcity.begin(); it != m_scarcity.end(); it++) {
          if (it->first == i)
            it->second = it->second + 1;
        }
      }
    }
  }
  return (std::string(bitmap));
}

std::string
NTorrentAdHocAppNaive::DecodeBitmap(shared_ptr<const Interest> interest)
{
  // First, decode the received bitmap
  char bitmap[m_torrentPacketNum];
  std::string bitmapNameComp = interest->getName().get(3).toUri();
  int j = 0;
  for (auto i = 0; i < m_torrentPacketNum * 3; i = i + 3) {
    bitmap[j] = (bitmapNameComp[i + 1] << 4) | (bitmapNameComp[i + 2]);
    // std::cout << "Element " << j << " is " << bitmap[j] << std::endl;
    j++;
  }

  // Then, update the local piece scarcity knowledge
  // search for all the pieces the peer does not have in the received
  // bitmap, and add 1 to the piece counter if the peer that sent the bitmap
  // does not have the piece as well
  for (auto i = 0; i < m_torrentPacketNum; i++) {
    if (m_downloadedData[i].second == 0) {
      if (bitmap[i] == 0) {
        for (auto it = m_scarcity.begin(); it != m_scarcity.end(); it++) {
          if (it->first == i)
            it->second = it->second + 1;
        }
      }
    }
  }
  return (std::string(bitmap));
}

void
NTorrentAdHocAppNaive::SendInterestForData(std::string nodeId, std::string bitmap)
{
  // sort the scarcity list
  m_scarcity.sort(compare_pairs);
  uint32_t seqNum = -1;
  // find the rarest piece that the other peer has and there is
  // no outstanding Interest
  for (auto it = m_scarcity.begin(); it != m_scarcity.end(); it++) {
    if (bitmap[std::get<0>(*it)] == '1') {
      // check if we have already sent an outstanding Interest
      bool outstadingInterestFound = false;
      for (auto it2 = m_outstandingInterests.begin(); it2 != m_outstandingInterests.end(); it2++) {
        if (std::get<0>(*it2) == std::get<0>(*it)) {
          outstadingInterestFound = true;
          break;
        }
      }
      if (outstadingInterestFound) {
        continue;
      }
      seqNum = std::get<0>(*it);
      break;
    }
  }
  if (seqNum == -1) {
    NS_LOG_INFO("Could not find a missing piece to fetch from: " << nodeId);
    if (!m_beaconSent.IsRunning())
      m_beaconSent = Simulator::Schedule(ns3::MilliSeconds(m_randomBeacon->GetValue() + 2000), &NTorrentAdHocAppNaive::SendBeacon, this);
    return;
  }

  // create the Interest
  Name interestName = Name(m_torrentPrefix).appendSequenceNumber(seqNum);
  shared_ptr<Interest> interest = make_shared<Interest>(interestName);
  NS_LOG_INFO("Sending Interest for Torrent Data Packet: " << interestName.toUri());

  // schedule the Interest retansmission event
  ns3::EventId retransmission = Simulator::Schedule(m_expirationTimer, &NTorrentAdHocAppNaive::ResendInterestForData, this, interestName, 0);

  m_outstandingInterests.push_back(std::make_tuple(seqNum, nodeId, bitmap, retransmission));

  m_transmittedInterests(interest, this, m_face);
  m_appLink->onReceiveInterest(*interest);
}

void
NTorrentAdHocAppNaive::SendData(Name interestName)
{
  // Send out the torrent data
  shared_ptr<Data> data = make_shared<Data>(interestName);

  data->setContent(make_shared< ::ndn::Buffer>(1024));

  Signature signature;
  SignatureInfo signatureInfo(static_cast< ::ndn::tlv::SignatureTypeValue>(255));

  signature.setInfo(signatureInfo);
  signature.setValue(::ndn::makeNonNegativeIntegerBlock(::ndn::tlv::SignatureValue, 0));

  data->setSignature(signature);

  NS_LOG_INFO("Sending Torrent Data Packet: " << data->getName().toUri());

  // to create real wire encoding
  data->wireEncode();

  m_transmittedDatas(data, this, m_face);
  m_appLink->onReceiveData(*data);
}

void
NTorrentAdHocAppNaive::SendBeacon()
{
  // Send a beacon
  Name beaconName = Name("beacon");
  beaconName.append("/node" + std::to_string(m_nodeId));
  beaconName.appendSequenceNumber(m_beaconSeq);
  m_beaconSeq++;
  NS_LOG_DEBUG("Sending beacon: " << beaconName.toUri());

  shared_ptr<Interest> beacon = make_shared<Interest>(beaconName);

  m_transmittedInterests(beacon, this, m_face);
  m_appLink->onReceiveInterest(*beacon);

  m_beaconSent = Simulator::Schedule(ns3::MilliSeconds(m_randomBeacon->GetValue() + 2000), &NTorrentAdHocAppNaive::SendBeacon, this);
}

void
NTorrentAdHocAppNaive::ResendInterestForData(Name interestName, uint8_t numberOfRetransmissions)
{
  uint32_t seqNum = interestName.get(-1).toSequenceNumber();
  if (numberOfRetransmissions == 3) {
    // if we have done already 3 retransmissions, then just erase outstanding
    // Interest entry and schedule the next beacon trasmission
    NS_LOG_INFO("Reached maximum number of retransmissions for: " << interestName.toUri());
    for (auto it = m_outstandingInterests.begin(); it != m_outstandingInterests.end(); it++) {
      if (seqNum == std::get<0>(*it)) {
        m_outstandingInterests.erase(it);
        break;
      }
    }
    if (!m_beaconSent.IsRunning())
      m_beaconSent = Simulator::Schedule(ns3::MilliSeconds(m_randomBeacon->GetValue() + 2000), &NTorrentAdHocAppNaive::SendBeacon, this);
    return;
  }

  // if we have not done 3 retransmissions, retransmit
  shared_ptr<Interest> interest = make_shared<Interest>(interestName);
  NS_LOG_INFO("Retransmitting Interest for Torrent Data Packet: " << interestName.toUri());

  // schedule the Interest retansmission event
  ns3::EventId retransmission = Simulator::Schedule(m_expirationTimer, &NTorrentAdHocAppNaive::ResendInterestForData, this, interestName, numberOfRetransmissions + 1);

  for (auto it = m_outstandingInterests.begin(); it != m_outstandingInterests.end(); it++) {
    if (seqNum == std::get<0>(*it)) {
      std::get<3>(*it) = retransmission;
      break;
    }
  }

  m_transmittedInterests(interest, this, m_face);
  m_appLink->onReceiveInterest(*interest);
}

} // namespace ndn
} // namespace ns3
