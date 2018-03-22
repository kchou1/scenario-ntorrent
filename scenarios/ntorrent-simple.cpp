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

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ndnSIM-module.h"
#include "ns3/netanim-module.h"

#include "src/util/shared-constants.hpp"
#include "../extensions/ntorrent-consumer-app.hpp"
#include "../extensions/ntorrent-producer-app.hpp"

namespace ndn_ntorrent = ndn::ntorrent;
namespace ndn{
namespace ntorrent{
const char * ndn_ntorrent::SharedConstants::commonPrefix = "";
}
}

namespace ns3 {
namespace ndn {

/**
 * This scenario simulates a very simple network topology:
 *
 *
 *      +----------+     1Mbps      +--------+     1Mbps      +----------+
 *      | consumer | <------------> | router | <------------> | producer |
 *      +----------+         10ms   +--------+          10ms  +----------+
 *
 *
 * Consumer requests data from producer with frequency 10 interests per second
 * (interests contain constantly increasing sequence number).
 *
 * For every received interest, producer replies with a data packet, containing
 * 1024 bytes of virtual payload.
 *
 * To run scenario and see what is happening, use the following command:
 *
 *     ./waf --run=ntorrent-simple
 */


int
main(int argc, char *argv[])
{

  // setting default parameters for PointToPoint links and channels
  Config::SetDefault("ns3::PointToPointNetDevice::DataRate", StringValue("32kbps"));
  Config::SetDefault("ns3::PointToPointChannel::Delay", StringValue("10ms"));

  //defaults for command line arguments
  uint32_t namesPerSegment = 2;
  uint32_t namesPerManifest = 2;
  uint32_t dataPacketSize = 64;
  
  // Read optional command-line parameters (e.g., enable visualizer with ./waf --run=<> --visualize
  CommandLine cmd;
  cmd.AddValue("namesPerSegment", "Number of names per segment", namesPerSegment);
  cmd.AddValue("namesPerManifest", "Number of names per manifest", namesPerManifest);
  cmd.AddValue("dataPacketSize", "Data Packet size", dataPacketSize);
  cmd.Parse(argc, argv);

  // Creating nodes
  NodeContainer nodes;
  nodes.Create(2);
  
  AnimationInterface::SetConstantPosition (nodes.Get(0), 20, 20);
  AnimationInterface::SetConstantPosition (nodes.Get(1), 60, 60);
  
  //This stuff needs NetAnim to run. 
  //NetAnim takes the xml file as input.
  //AnimationInterface anim ("/var/tmp/test.xml");
  //anim.UpdateNodeDescription (nodes.Get (0), "Producer"); 
  //anim.UpdateNodeDescription (nodes.Get (1), "Consumer");
  //anim.UpdateNodeColor (nodes.Get (1), 255, 0, 0);
  //anim.UpdateNodeColor (nodes.Get (0), 10, 100, 10); 

  // Connecting nodes using two links
  PointToPointHelper p2p;
  p2p.Install(nodes.Get(0), nodes.Get(1));
  
  // Install NDN stack on all nodes
  StackHelper ndnHelper;
  ndnHelper.SetDefaultRoutes(true);
  ndnHelper.InstallAll();

  // Choosing forwarding strategy
  ndn::StrategyChoiceHelper::InstallAll("/", "/localhost/nfd/strategy/multicast");

  // Installing applications
  // Producer
  ndn::AppHelper producerHelper("NTorrentProducerApp");
  producerHelper.SetAttribute("Prefix", StringValue("/"));
  producerHelper.SetAttribute("namesPerSegment", IntegerValue(namesPerSegment));
  producerHelper.SetAttribute("namesPerManifest", IntegerValue(namesPerManifest));
  producerHelper.SetAttribute("dataPacketSize", IntegerValue(dataPacketSize));
  producerHelper.Install(nodes.Get(0)).Start(Seconds(1.0));

  // Consumer
  ndn::AppHelper consumerHelper("NTorrentConsumerApp");
  consumerHelper.SetAttribute("Prefix", StringValue("/"));
  consumerHelper.SetAttribute("namesPerSegment", IntegerValue(namesPerSegment));
  consumerHelper.SetAttribute("namesPerManifest", IntegerValue(namesPerManifest));
  consumerHelper.SetAttribute("dataPacketSize", IntegerValue(dataPacketSize));
  consumerHelper.Install(nodes.Get(1)).Start(Seconds(3.0));

  Simulator::Stop(Seconds(60.0));

  std::cout << "Running with parameters: " << std::endl;
  std::cout << "namesPerSegment: " << namesPerSegment << std::endl;
  std::cout << "namesPerManifest: " << namesPerManifest << std::endl;
  std::cout << "dataPacketSize: " << dataPacketSize << std::endl;
  
  Simulator::Run();
  Simulator::Destroy();
  
  return 0;
}

} // namespace ndn
} // namespace ns3

int
main(int argc, char* argv[])
{
  return ns3::ndn::main(argc, argv);
}
