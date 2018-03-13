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

NS_LOG_COMPONENT_DEFINE("NTorrentConsumerApp");
namespace ndn_ntorrent = ndn::ntorrent;

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
      .AddAttribute("namesPerSegment", "Number of names (files) per segment", IntegerValue(2),
                    MakeIntegerAccessor(&NTorrentConsumerApp::m_namesPerSegment), MakeIntegerChecker<int32_t>())
      .AddAttribute("namesPerManifest", "Number of names per manifest", IntegerValue(64),
                    MakeIntegerAccessor(&NTorrentConsumerApp::m_namesPerManifest), MakeIntegerChecker<int32_t>())
      .AddAttribute("dataPacketSize", "Size of each data packet", IntegerValue(64),
                    MakeIntegerAccessor(&NTorrentConsumerApp::m_dataPacketSize), MakeIntegerChecker<int32_t>())
      .AddAttribute("LifeTime", "LifeTime for interest packet", StringValue("1s"),
                    MakeTimeAccessor(&NTorrentConsumerApp::m_interestLifeTime), MakeTimeChecker());
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
    ndn::App::StartApplication();
    ndn::FibHelper::AddRoute(GetNode(), "/", m_face, 0);
    
    /*for(int i=0;i<5;i++)
        Simulator::Schedule(Seconds(i+1.0), &NTorrentConsumerApp::SendInterest, this);*/

    copyTorrentFile();

    //std::string torrentName = "/NTORRENT/"+ndn_ntorrent::DUMMY_FILE_PATH+"torrent-file/sha256digest=16dcb0d2de642941c0522515144c69300f50834fcba78fb5f4c54bd6ed8254ec";
    SendInterest(m_initialSegment.getFullName().toUri());
    m_interestQueue = make_shared<ndn_ntorrent::InterestQueue>();
}

void
NTorrentConsumerApp::StopApplication()
{
    ndn::App::StopApplication();
}

void
NTorrentConsumerApp::SendInterest()
{
  //Just for testing, not really going to be used.. 
  auto interest = std::make_shared<Interest>(std::string(ndn_ntorrent::SharedConstants::commonPrefix) + "/NTORRENT/" + to_string(m_seq++));
  Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable>();
  interest->setNonce(rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
  interest->setInterestLifetime(ndn::time::seconds(1));
  NS_LOG_DEBUG("Sending Interest packet for " << *interest);
  m_transmittedInterests(interest, this, m_face);
  m_appLink->onReceiveInterest(*interest);
}

void
NTorrentConsumerApp::SendInterest(const string& interestName)
{
  auto interest = std::make_shared<Interest>(interestName);
  Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable>();
  interest->setNonce(rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
  interest->setInterestLifetime(ndn::time::seconds(1));
  NS_LOG_DEBUG("Sending Interest packet for " << *interest);
  m_transmittedInterests(interest, this, m_face);
  m_appLink->onReceiveInterest(*interest);
}

void
NTorrentConsumerApp::OnInterest(std::shared_ptr<const Interest> interest)
{
  //We don't have to process interests on the consumer-end for now
}

void
NTorrentConsumerApp::copyTorrentFile()
{
    //This is "technically" the same as what the producer does
    //In a real world application, both parties will have a torrent file
    //Since that isn't possible here, just generate the same torrent file on producer and consumer
    
    NS_LOG_DEBUG("Copying torrent file!");
    const auto& content = ndn_ntorrent::TorrentFile::generate(ndn_ntorrent::DUMMY_FILE_PATH, 
            m_namesPerSegment, m_namesPerManifest, m_dataPacketSize, true);
   
    //Copy only initial segment...
    //This will be used to make future requests
    m_initialSegment = content.first.at(0);
}

void
NTorrentConsumerApp::OnData(std::shared_ptr<const Data> data)
{
    NS_LOG_DEBUG("Received: " << data->getFullName());
    ndn_ntorrent::IoUtil::NAME_TYPE interestType = ndn_ntorrent::IoUtil::findType(data->getFullName());
    switch(interestType)
    {
        case ndn_ntorrent::IoUtil::TORRENT_FILE:
        {
            ndn_ntorrent::TorrentFile file(data->wireEncode());
            std::vector<Name> manifestCatalog = file.getCatalog();
            manifests.insert(manifests.end(), manifestCatalog.begin(), manifestCatalog.end());
            shared_ptr<Name> nextSegmentPtr = file.getTorrentFilePtr();
            if(nextSegmentPtr!=nullptr){
                NS_LOG_DEBUG("Wait.. There are more torrent segments remaining!");
                SendInterest(nextSegmentPtr.get()->toUri());
            }
            else
            {
                NS_LOG_DEBUG("W00t! Torrent file is done!");
                NS_LOG_DEBUG("Total number of file manifests: " << manifests.size());
                NS_LOG_DEBUG("These are the manifests:");
                for(uint32_t i=0;i<manifests.size();i++)
                    NS_LOG_DEBUG(i << " " << manifests.at(i));
            }
            
            //TODO: Need to queue interests...
            //This currently works for just one manifest
            for(uint8_t i=0; i<manifestCatalog.size(); i++)
            {
                SendInterest(manifestCatalog.at(i).toUri());
            }
            break;
        }
        case ndn_ntorrent::IoUtil::FILE_MANIFEST:
        {
            ndn_ntorrent::FileManifest fm(data->wireEncode());
            
            std::vector<Name> subManifestCatalog = fm.catalog();
            shared_ptr<Name> nextSegmentPtr = fm.submanifest_ptr();
            if(nextSegmentPtr!=nullptr){
                NS_LOG_DEBUG("Wait.. There are more manifests remaining!");
                SendInterest(nextSegmentPtr.get()->toUri());
            }
            
            //TODO: Need to queue interests...
            //This currently works for just one submanifest
            for(uint8_t i=0; i<subManifestCatalog.size(); i++)
            {
                SendInterest(subManifestCatalog.at(i).toUri());
            }
            break;
        }
        case ndn_ntorrent::IoUtil::DATA_PACKET:
        {
            //TODO: Handle incoming data packets
            Data d(data->wireEncode());
            const Block& content = d.getContent();
            break;
        }
        case ndn_ntorrent::IoUtil::UNKNOWN:
        {
            //This should never happen
            break;
        }
    }
}

} // namespace ndn
} // namespace ns3
