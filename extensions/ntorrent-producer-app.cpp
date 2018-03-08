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

#include "ntorrent-producer-app.hpp"

NS_LOG_COMPONENT_DEFINE("NTorrentProducerApp");
namespace ndn_ntorrent = ndn::ntorrent;

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(NTorrentProducerApp);

TypeId
NTorrentProducerApp::GetTypeId(void)
{
    static TypeId tid = TypeId("NTorrentProducerApp")
      .SetParent<Application>()
      .AddConstructor<NTorrentProducerApp>()
      .AddAttribute("Prefix", "Prefix, for which producer has the data", StringValue("/"),
                    MakeNameAccessor(&NTorrentProducerApp::m_prefix), MakeNameChecker())
      .AddAttribute("nFiles", "Number of files in the torrent", IntegerValue(5),
                    MakeIntegerAccessor(&NTorrentProducerApp::m_nFiles), MakeIntegerChecker<int32_t>())
      .AddAttribute("nSegments", "Number of segments per file", IntegerValue(5),
                    MakeIntegerAccessor(&NTorrentProducerApp::m_nSegmentsPerFile), MakeIntegerChecker<int32_t>())
      //.AddAttribute("Postfix",
      //        "Postfix that is added to the output data (e.g., for adding producer-uniqueness)",
      //        StringValue("/prefix/sub/"), MakeNameAccessor(&NTorrentProducerApp::m_postfix), MakeNameChecker())
      .AddAttribute("PayloadSize", "Virtual payload size for Content packets", IntegerValue(1024),
              MakeIntegerAccessor(&NTorrentProducerApp::m_virtualPayloadSize),
              MakeIntegerChecker<uint32_t>())
      .AddAttribute("Freshness", "Freshness of data packets, if 0, then unlimited freshness",
              TimeValue(Seconds(0)), MakeTimeAccessor(&NTorrentProducerApp::m_freshness),
              MakeTimeChecker())
      .AddAttribute("Signature",
              "Fake signature, 0 valid signature (default), other values application-specific",
              IntegerValue(0), MakeIntegerAccessor(&NTorrentProducerApp::m_signature),
              MakeIntegerChecker<uint32_t>())
      .AddAttribute("KeyLocator",
              "Name to be used for key locator.  If root, then key locator is not used",
              NameValue(), MakeNameAccessor(&NTorrentProducerApp::m_keyLocator), MakeNameChecker());

    return tid;
}

NTorrentProducerApp::NTorrentProducerApp()
{
}

NTorrentProducerApp::~NTorrentProducerApp()
{
}

void
NTorrentProducerApp::StartApplication()
{
    App::StartApplication();
    ndn::FibHelper::AddRoute(GetNode(), "/", m_face, 0);
    generateTorrentFile();
}

void
NTorrentProducerApp::StopApplication()
{
    App::StopApplication();

}

void
NTorrentProducerApp::OnInterest(shared_ptr<const Interest> interest)
{
    ndn::App::OnInterest(interest); 
    NS_LOG_DEBUG("Received interest for" << interest->getName());

    Name dataName(interest->getName());
    auto data = make_shared<Data>();
    data->setName(dataName);
    data->setFreshnessPeriod(::ndn::time::milliseconds(m_freshness.GetMilliSeconds()));
    data->setContent(make_shared< ::ndn::Buffer>(m_virtualPayloadSize));

    Signature signature;
    SignatureInfo signatureInfo(static_cast< ::ndn::tlv::SignatureTypeValue>(255));

    if (m_keyLocator.size() > 0) {
        signatureInfo.setKeyLocator(m_keyLocator);
    }

    signature.setInfo(signatureInfo);
    signature.setValue(::ndn::makeNonNegativeIntegerBlock(::ndn::tlv::SignatureValue, m_signature));

    data->setSignature(signature);
    NS_LOG_INFO("node(" << GetNode()->GetId() << ") responding with Data: " << data->getName());

    data->wireEncode();

    m_transmittedDatas(data, this, m_face);
    m_appLink->onReceiveData(*data);
}
    
void
NTorrentProducerApp::generateTorrentFile()
{
    //TODO use ntorrent code here
    NS_LOG_DEBUG("Creating torrent file!");
    const auto& content = ndn_ntorrent::TorrentFile::generate("/var/tmp/test/", 8, 64, 1024, true);
    
    torrentSegments = content.first;
    
    for (const auto& ms : content.second) {
        manifests.insert(manifests.end(), ms.first.begin(), ms.first.end());
    }
    
    
    for(const auto& t : torrentSegments)
        NS_LOG_DEBUG("Torrent name: " << t.getFullName());
    for(int i=0;i<manifests.size();i++)
        NS_LOG_DEBUG("Manifest name: " << manifests.at(i));

    NS_LOG_DEBUG("Torrent segments: " << torrentSegments.size());
    NS_LOG_DEBUG("Manifests: " << manifests.size());
}

} // namespace ndn
} // namespace ns3
