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
                    MakeBooleanAccessor(&NTorrentAdHocAppNaive::m_isTorrentProducer), MakeBooleanChecker());
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

    if (m_torrentPrefix == "movie1") {
      ndn::FibHelper::AddRoute(GetNode(), "movie2", m_face, 0);
    } else {
      ndn::FibHelper::AddRoute(GetNode(), "movie1", m_face, 0);
    }
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

    m_beaconSent = Simulator::Schedule(ns3::MilliSeconds(m_randomBeacon->GetValue() + 2000), &NTorrentAdHocAppNaive::SendBeacon, this);

    m_expireTime = 200000000; // ns
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
  Name interestName = interest->getName();
  shared_ptr<Interest> interestForwarded = make_shared<Interest>(interestName);
  NS_LOG_DEBUG("Forwarding Interest. (" + interestName.toUri() + ")");

  m_transmittedInterests(interestForwarded, this, m_face);
  m_appLink->onReceiveInterest(*interestForwarded);
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

    if (interestName.get(0).toUri() == "beacon") {
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
      // TODO: Check if the bitmap is for the desired torrent file
      // Decide what to do with received Interest
      bool overheardInterest = false;
      for (auto it = m_overheard.begin(); it != m_overheard.end(); it++) {
        if (it->first == interestName.get(1).toUri()) {
          NS_LOG_DEBUG("Interest Desires Other Torrent File (seen before): " << interestName.get(1).toUri());
          if (it->second > Simulator::Now()) { // entry hasn't expired, refresh expire time
            overheardInterest = true;
            // Update Expire Time for torrent file if already in overheardInterest
            it->second = Simulator::Now() + m_expireTime;
            break;
          } else { // only one entry per prefix in overheard at a time
            m_overheard.erase(it);
            break;
          }
        }
      }

      if ('/' + interestName.get(1).toUri() != m_torrentPrefix.toUri()) {
        if (overheardInterest) {
          // std::string bitmap = DecodeBitmap(interest);
          Simulator::Schedule(ns3::MilliSeconds(m_random->GetValue()), &NTorrentAdHocAppNaive::ForwardInterest, this, interest);
        } else { // Haven't overheard
          // Only input if torrent prefix does not match node's desired Torrent File
          m_overheard.push_back(std::make_pair(interestName.get(1).toUri(), Simulator::Now() + m_expireTime));
          NS_LOG_DEBUG("Time Entry Expires: " << m_overheard.back().second);
        }

        // Go back to sending beacons
        if (!m_beaconSent.IsRunning()) {
          m_beaconSent = Simulator::Schedule(ns3::MilliSeconds(m_randomBeacon->GetValue() + 2000), &NTorrentAdHocAppNaive::SendBeacon, this);
        }
      } else {
        // Decode the bitmap of the neighbor
        std::string bitmap = DecodeBitmap(interest);
        Simulator::Schedule(ns3::MilliSeconds(m_random->GetValue()), &NTorrentAdHocAppNaive::CreateAndSendBitmap, this, interest);
        if (!m_scarcity.empty())
          Simulator::Schedule(ns3::MilliSeconds(m_random->GetValue()), &NTorrentAdHocAppNaive::SendInterestForData, this, interestName.get(2).toUri(), bitmap);
      }
    }
    else {
      // TODO: Check if the Interest for torrent data is for the desired torrent file
      // Decide what to do with received Interest
      bool overheardInterest = false;
      for (auto it = m_overheard.begin(); it != m_overheard.end(); it++) {
        if (it->first == interestName.get(0).toUri()) {
          NS_LOG_DEBUG("Interest Desires Other Torrent File: " << interestName.get(0).toUri());
          if (it->second > Simulator::Now()) {
            overheardInterest = true;
            // Update Expire Time for torrent file if already in overheardInterest
            it->second = Simulator::Now() + m_expireTime;
            break;
          } else {
            m_overheard.erase(it);
            break;
          }
        }
      }

      if ('/' + interestName.get(0).toUri() != m_torrentPrefix.toUri()) {
        // If overheard other nodes wanting this torrent prefix
        if (overheardInterest) {
          Simulator::Schedule(ns3::MilliSeconds(m_random->GetValue()), &NTorrentAdHocAppNaive::ForwardInterest, this, interest);
        } else {
          // Only input if torrent prefix does not match node's desired Torrent File
          m_overheard.push_back(std::make_pair(interestName.get(0).toUri(), Simulator::Now() + m_expireTime));
          NS_LOG_DEBUG("Time Entry Expires: " << m_overheard.back().second << " for " << interestName.toUri());
        }
      // this is an Interest for torrent data
      } else if (std::get<1>(m_downloadedData[interestName.get(-1).toSequenceNumber()]) == 1) {
        Simulator::Schedule(ns3::MilliSeconds(m_random->GetValue()), &NTorrentAdHocAppNaive::SendData, this, interestName);
      }
    }

}

void
NTorrentAdHocAppNaive::OnData(shared_ptr<const Data> data)
{
    // TODO: Check if bitmap or data packet is for the desired torrent file
    // Decide what to do with received Data

    // 2 cases: 1. this is an IBF packet, or 2. this is torent data packet
    if (data->getName().get(0).toUri() == "bitmap") {
    // received a bitmap
    if (m_bitmapSent.IsRunning()) {
      Simulator::Cancel(m_bitmapSent);
    }
    NS_LOG_DEBUG("Received Bitmap in data packet: " << data->getName().toUri());
    std::string bitmap = DecodeBitmap(data);

    bool overheardInterest = false;
    for (auto it = m_overheard.begin(); it != m_overheard.end(); it++) {
        if (it->first == data->getName().get(1).toUri()) {
            NS_LOG_DEBUG("Received Bitmap Data Wants Other File: " << data->getName().get(1).toUri());
          if (it->second > Simulator::Now()) { // check if still not expired
            overheardInterest = true;
            // Update Expire Time for torrent file if already in overheardInterest
            it->second = Simulator::Now() + m_expireTime;
            break;
          } else {
            m_overheard.erase(it);
            break;
          }
        }
    }

    // Request for torrent data if incoming bitmap shares same torrent file prefix
    if (!m_scarcity.empty() && '/' + data->getName().get(1).toUri() == m_torrentPrefix.toUri()) {
      Simulator::Schedule(ns3::MilliSeconds(m_random->GetValue()), &NTorrentAdHocAppNaive::SendInterestForData, this, data->getName().get(2).toUri(), bitmap);
    } else {
      if (overheardInterest) {
        // Forward since already heard bitmap interest for that torrent file
        NS_LOG_DEBUG("Forwarding Data Bitmap (overheard previously)." << data->getName().toUri());
        Simulator::Schedule(ns3::MilliSeconds(m_random->GetValue()), &NTorrentAdHocAppNaive::ForwardData, this, data);
      }
      // if no other beacon event is running, schedule one
      if (!m_beaconSent.IsRunning())
        m_beaconSent = Simulator::Schedule(ns3::MilliSeconds(m_randomBeacon->GetValue() + 2000), &NTorrentAdHocAppNaive::SendBeacon, this);
    }
  }
  else {
    // Logic for receiving a data packet
    NS_LOG_DEBUG("Received torrent data: " << data->getName().toUri());
    if (m_scarcity.empty() || '/' + data->getName().get(0).toUri() != m_torrentPrefix.toUri()) {
      // if we have downloaded all the torrent data or not the corrent torrent file
      // avoid doing all the rest
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
