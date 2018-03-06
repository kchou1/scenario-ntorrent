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
      .AddAttribute("Prefix", "Prefix, for which producer has the data", StringValue("/"),
                    MakeNameAccessor(&NTorrentConsumerApp::m_prefix), MakeNameChecker())
      .AddAttribute("Postfix",
              "Postfix that is added to the output data (e.g., for adding producer-uniqueness)",
              StringValue("/"), MakeNameAccessor(&NTorrentConsumerApp::m_postfix), MakeNameChecker())
      .AddAttribute("PayloadSize", "Virtual payload size for Content packets", IntegerValue(1024),
              MakeIntegerAccessor(&NTorrentConsumerApp::m_virtualPayloadSize),
              MakeIntegerChecker<uint32_t>())
      .AddAttribute("Freshness", "Freshness of data packets, if 0, then unlimited freshness",
              TimeValue(Seconds(0)), MakeTimeAccessor(&NTorrentConsumerApp::m_freshness),
              MakeTimeChecker())
      .AddAttribute("Signature",
              "Fake signature, 0 valid signature (default), other values application-specific",
              IntegerValue(0), MakeIntegerAccessor(&NTorrentConsumerApp::m_signature),
              MakeIntegerChecker<uint32_t>())
      .AddAttribute("KeyLocator",
              "Name to be used for key locator.  If root, then key locator is not used",
              NameValue(), MakeNameAccessor(&NTorrentConsumerApp::m_keyLocator), MakeNameChecker());

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
    m_instance->setPrefix(m_prefix);
    m_instance->run();
}

void
NTorrentConsumerApp::StopApplication()
{
    m_instance.reset();
}

void
NTorrentConsumerApp::OnInterest(shared_ptr<const Interest> interest)
{
    Name dataName(interest->getName());
    // dataName.append(m_postfix);
    // dataName.appendVersion();

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
    std::cout << "node(" << GetNode()->GetId() << ") responding with Data: " << data->getName() << std::endl;
    data->wireEncode();

    m_transmittedDatas(data, this, m_face);
    m_appLink->onReceiveData(*data);
}

void
NTorrentConsumerApp::OnData(shared_ptr<const Data> data)
{
}

void
NTorrentConsumerApp::OnNack(shared_ptr<const lp::Nack> nack)
{
}


} // namespace ndn
} // namespace ns3
