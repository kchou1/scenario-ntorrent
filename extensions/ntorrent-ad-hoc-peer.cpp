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

#include "ntorrent-ad-hoc-peer.hpp"

NS_LOG_COMPONENT_DEFINE("NTorrentAdHocApp");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(NTorrentAdHocApp);

TypeId
NTorrentAdHocApp::GetTypeId(void)
{
    static TypeId tid = TypeId("NTorrentAdHocApp")
      .SetParent<Application>()
      .AddConstructor<NTorrentAdHocApp>()
      // Making things simple: right away I specify the total number of torrent packets
      .AddAttribute("NumberOfPackets", "Number of torrent packets (in total)", IntegerValue(0),
                    MakeIntegerAccessor(&NTorrentAdHocApp::m_torrentPacketNum), MakeIntegerChecker<int32_t>())
      // Prefix of torrent packets
      .AddAttribute("TorrentPrefix", "Torrent prefix", StringValue("/"),
                    MakeNameAccessor(&NTorrentAdHocApp::m_torrentPrefix), MakeNameChecker())
      // Node id
      .AddAttribute("NodeId", "Id of the current node", IntegerValue(-1),
                    MakeIntegerAccessor(&NTorrentAdHocApp::m_nodeId), MakeIntegerChecker<int32_t>())
      // Send a new beacon every BeaconTimer seconds + random timer between 0 and RandomTimerRange
      .AddAttribute("BeaconTimer", "Periodic timer for beacons", StringValue("1s"),
                    MakeTimeAccessor(&NTorrentAdHocApp::m_beaconTimer), MakeTimeChecker())
      // Random timer between 0 and RandomTimerRange
      .AddAttribute("RandomTimerRange", "Random Timer Range", StringValue("1s"),
                    MakeTimeAccessor(&NTorrentAdHocApp::m_randomTimerRange), MakeTimeChecker())
      // Is this node the original torrent producer or just a peer?
      .AddAttribute("TorrentProducer", "Has this node generated the torrent?", BooleanValue(false),
                    MakeBooleanAccessor(&NTorrentAdHocApp::m_isTorrentProducer), MakeBooleanChecker());
    return tid;
}

NTorrentAdHocApp::NTorrentAdHocApp()
{
  NS_ASSERT(m_nodeId != -1);
  m_seq = 0;
  m_IBFhasChanged = true;
}

NTorrentAdHocApp::~NTorrentAdHocApp()
{
}

void
NTorrentAdHocApp::StartApplication()
{
    NS_LOG_DEBUG("Starting application for node: " << m_nodeId);
    App::StartApplication();
    ndn::FibHelper::AddRoute(GetNode(), m_torrentPrefix, m_face, 0);
    ndn::FibHelper::AddRoute(GetNode(), "beacon", m_face, 0);

    PopulateIBF();

    m_random = CreateObject<UniformRandomVariable>();
    m_random->SetAttribute("Min", DoubleValue(0.0));
    m_random->SetAttribute("Max", DoubleValue(m_randomTimerRange.GetSeconds() * 1000));

    Simulator::Schedule(ns3::MilliSeconds(m_beaconTimer.GetMilliSeconds() + m_random->GetValue()), &NTorrentAdHocApp::SendBeacon, this);
}

void
NTorrentAdHocApp::StopApplication()
{
    App::StopApplication();
}

void
NTorrentAdHocApp::SendInterest()
{
}

void
NTorrentAdHocApp::SendInterest(const string& interestName)
{

}

void
NTorrentAdHocApp::OnInterest(shared_ptr<const Interest> interest)
{
    ndn::App::OnInterest(interest);
    // Check what kind of Interest this is: 1) Beacon, or 2) Interest for torrent data
    Name interestName = interest->getName();

    if (interestName.get(0).toUri() == "beacon") {
      // this is a beacon
      NS_LOG_DEBUG("Received beacon: " << interestName.toUri());
      Simulator::Schedule(ns3::MilliSeconds(m_random->GetValue()), &NTorrentAdHocApp::CreateAndSendIBF, this, interest);
    }
    else {
      // this is an Interest for torrent data
      // TODO: Send torrent data
      if (std::get<1>(m_downloadedData[interestName.get(-1).toSequenceNumber()]) == 1) {
        Simulator::Schedule(ns3::MilliSeconds(m_random->GetValue()), &NTorrentAdHocApp::SendData, this, interestName);
      }
    }

}

void
NTorrentAdHocApp::copyTorrentFile()
{

}

void
NTorrentAdHocApp::OnData(shared_ptr<const Data> data)
{
  // 2 cases: 1. this is an IBF packet, or 2. this is torent data packet
  if (data->getName().get(0).toUri() == "beacon") {
    // received an IBF
    NS_LOG_DEBUG("Received IBF: " << data->getName().toUri());
    std::vector<std::tuple<Name, uint32_t, uint32_t>> receivedIBF;
    DecodeIBFAndComputeRearestPiece(data, &receivedIBF);
    // TODO: Send Interests for rarest pieces first
    // For now, send Interests sequentially
    SendInterestForData(receivedIBF);
  }
  else {
    // Logic for receiving a data packet
    NS_LOG_DEBUG("Received torrent data: " << data->getName().toUri());
    if (std::get<1>(m_downloadedData[data->getName().get(-1).toSequenceNumber()]) == 0) {
      m_downloadedData[data->getName().get(-1).toSequenceNumber()].second = 1;
    }
  }
}

void
NTorrentAdHocApp::SendBeacon()
{
  // Send a beacon
  Name beaconName = Name("beacon" + m_torrentPrefix.toUri() + "/node" + std::to_string(m_nodeId));
  beaconName.appendSequenceNumber(m_seq);
  m_seq++;
  NS_LOG_DEBUG("Sending beacon: " << beaconName.toUri());

  shared_ptr<Interest> beacon = make_shared<Interest>(beaconName);

  m_transmittedInterests(beacon, this, m_face);
  m_appLink->onReceiveInterest(*beacon);

  Simulator::Schedule(ns3::MilliSeconds(m_beaconTimer.GetMilliSeconds() + m_random->GetValue()), &NTorrentAdHocApp::SendBeacon, this);
}

void
NTorrentAdHocApp::PopulateIBF()
{
  NS_LOG_DEBUG("Populate IBF with data");
  // loop through all the data packets of the torrent
  for (auto i = 0; i < m_torrentPacketNum; i++) {
    // create the ibf tuple and push it back to the vector
    Name torrentPacketName = Name(m_torrentPrefix.toUri()).appendSequenceNumber(i);
    std::tuple<Name, uint32_t, uint32_t> ibfTuple;
    std::pair<Name, uint32_t> dataPair;
    if (m_isTorrentProducer) {
      // if this is the original torrent producer
      ibfTuple = std::make_tuple(torrentPacketName, m_nodeId, 1);
      dataPair = std::make_pair(torrentPacketName, 1);
    }
    else {
      // if this is not the original torrent producer
      ibfTuple = std::make_tuple(torrentPacketName, m_nodeId, 0);
      dataPair = std::make_pair(torrentPacketName, 0);
    }
    m_IBF.push_back(ibfTuple);
    m_downloadedData.push_back(dataPair);
  }
}

void
NTorrentAdHocApp::CreateAndSendIBF(shared_ptr<const Interest> interest)
{
  shared_ptr<Data> data = make_shared<Data>(interest->getName());

  // minor optimization: encode the IBF again only if its content has changed
  // since the last time that it was encoded
  if (m_IBFhasChanged) {
    ::ndn::EncodingEstimator estimator;
    size_t estimatedSize = EncodeContent(estimator);

    ::ndn::EncodingBuffer buffer(estimatedSize, 0);
    EncodeContent(buffer);

    m_encodedIBF = buffer.block();

    m_IBFhasChanged = false;
  }

  data->setContentType(::ndn::tlv::ContentType_Blob);
  data->setContent(m_encodedIBF);

  Signature signature;
  SignatureInfo signatureInfo(static_cast< ::ndn::tlv::SignatureTypeValue>(255));

  signature.setInfo(signatureInfo);
  signature.setValue(::ndn::makeNonNegativeIntegerBlock(::ndn::tlv::SignatureValue, 0));

  data->setSignature(signature);

  NS_LOG_INFO("Sending IBF: " << interest->getName().toUri());

  // to create real wire encoding
  data->wireEncode();

  m_transmittedDatas(data, this, m_face);
  m_appLink->onReceiveData(*data);

}

template<::ndn::encoding::Tag TAG>
size_t
NTorrentAdHocApp::EncodeContent(::ndn::EncodingImpl<TAG>& encoder) const
{
  size_t totalLength = 0;

  for (auto it = m_IBF.rbegin(); it != m_IBF.rend(); it++) {
    // send only the entries for which the node has downloaded the data
    uint32_t seq = std::get<0>(*it).get(-1).toSequenceNumber();
    if (std::get<1>(m_downloadedData[seq]) == 0)
      continue;
    // encode the last element of an entry first (counter)
    uint32_t counter = std::get<2>(*it);
    totalLength += encoder.prependByteArray(reinterpret_cast<uint8_t*>(&counter), sizeof(counter));
    totalLength += encoder.prependVarNumber(sizeof(counter));
    totalLength += encoder.prependVarNumber(128);
    // encode the second element (nonce)
    uint32_t nonce = std::get<1>(*it);
    totalLength += encoder.prependByteArray(reinterpret_cast<uint8_t*>(&nonce), sizeof(nonce));
    totalLength += encoder.prependVarNumber(sizeof(nonce));
    totalLength += encoder.prependVarNumber(129);
    // encode the first element (torrent packet name)
    totalLength += std::get<0>(*it).wireEncode(encoder);
  }

  totalLength += encoder.prependVarNumber(totalLength);
  totalLength += encoder.prependVarNumber(::ndn::tlv::Content);
  return totalLength;
}

void
NTorrentAdHocApp::DecodeIBFAndComputeRearestPiece(shared_ptr<const Data> data,
                                                  std::vector<std::tuple<Name, uint32_t, uint32_t>>* receivedIBF)
{
  // decode the received IBF content
  if (data->getContentType() != ::ndn::tlv::ContentType_Blob) {
    NS_LOG_ERROR("Expected Content Type Blob");
    return;
  }

  const Block& content = data->getContent();
  content.parse();

  auto element = content.elements_begin();
  if (content.elements_end() == element) {
    NS_LOG_ERROR("IBF with empty content");
    return;
  }

  // construct the received IBF data structure
  for (; element != content.elements_end(); ++element) {
    // decode the data packet name first
    element->parse();
    Name packetName(*element);
    if (packetName.empty()) {
      NS_LOG_ERROR("Empty data packet name in the IBF");
      return;
    }

    // decode the nonce
    element++;
    uint32_t nonce = 0;
    if (element->type() == 129) {
      if (element->value_size() != sizeof(nonce)) {
        NS_LOG_ERROR("Malformed nonce in the IBF");
        return;
      }
      std::memcpy(&nonce, element->value(), sizeof(nonce));
    }
    else {
      NS_LOG_ERROR("Nonce with malformed TLV number in the IBF");
      return;
    }

    // decode the counter
    // decode the nonce
    element++;
    uint32_t counter = 0;
    if (element->type() == 128) {
      if (element->value_size() != sizeof(counter)) {
        NS_LOG_ERROR("Malformed counter in the IBF");
        return;
      }
      std::memcpy(&counter, element->value(), sizeof(counter));
    }
    else {
      NS_LOG_ERROR("Counter with malformed TLV number in the IBF");
      return;
    }

    receivedIBF->push_back(std::make_tuple(packetName, nonce, counter));

    // Making sure we encode and decode in the right way
    // std::cerr << "Name: " << packetName.toUri() << std::endl;
    // std::cerr << "Counter: " << counter << std::endl;
    // std::cerr << "Nonce: " << nonce << std::endl;
  }
  NS_LOG_DEBUG("IBF decoded successfully: " << data->getName().toUri());
  // TODO: Apply the data scarcity estimation logic

}

void
NTorrentAdHocApp::SendInterestForData(std::vector<std::tuple<Name, uint32_t, uint32_t>> receivedIBF,
                                      uint32_t interestsSent)
{
  // For now, just find the next missing data piece that the node that sent
  // its IBF has already downloaded. For each received IBF, try to fetch
  // MAX_PACKETS_TO_FETCH packets
  // TODO: Fetch the rarest piece
  if (interestsSent == MAX_PACKETS_TO_FETCH) {
    return;
  }

  // Find the first missing data packet that the other node has
  for (auto it1 = m_downloadedData.begin(); it1 != m_downloadedData.end(); it1++) {
    // find the first missing data packet
    if (std::get<1>(*it1) == 1) {
      continue;
    }
    for (auto it2 = receivedIBF.begin(); it2 != receivedIBF.end(); it2++) {
      // check if the other node has this data packet
      if (std::get<0>(*it1).toUri() != std::get<0>(*it2).toUri()) {
        continue;
      }
      shared_ptr<Interest> interest = make_shared<Interest>(std::get<0>(*it1));

      NS_LOG_DEBUG("Sending Interest for torrent data: " << std::get<0>(*it1));

      m_transmittedInterests(interest, this, m_face);
      m_appLink->onReceiveInterest(*interest);

      Simulator::Schedule(ns3::MilliSeconds(m_random->GetValue()), &NTorrentAdHocApp::SendInterestForData, this, receivedIBF, interestsSent + 1);

      return;
    }
  }
}

void
NTorrentAdHocApp::SendData(Name interestName)
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

} // namespace ndn
} // namespace ns3
