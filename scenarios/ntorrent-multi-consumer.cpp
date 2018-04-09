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

#include <cmath>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ndnSIM-module.h"
#include "ns3/netanim-module.h"

#include "src/util/shared-constants.hpp"
#include "../extensions/ntorrent-consumer-app.hpp"
#include "../extensions/ntorrent-producer-app.hpp"

#define PI 3.14159

namespace ndn_ntorrent = ndn::ntorrent;
namespace ndn{
namespace ntorrent{
const char * ndn_ntorrent::SharedConstants::commonPrefix = "";
}
}

namespace ns3 {
namespace ndn {


int
main(int argc, char *argv[])
{

  // setting default parameters for PointToPoint links and channels
  Config::SetDefault("ns3::PointToPointNetDevice::DataRate", StringValue("32kbps"));
  Config::SetDefault("ns3::PointToPointChannel::Delay", StringValue("10ms"));

  //defaults for command line arguments
  uint32_t namesPerSegment = 2;
  uint32_t namesPerManifest = 2;
  uint32_t dataPacketSize = 128;
  
  // Read optional command-line parameters (e.g., enable visualizer with ./waf --run=<> --visualize
  CommandLine cmd;
  cmd.AddValue("namesPerSegment", "Number of names per segment", namesPerSegment);
  cmd.AddValue("namesPerManifest", "Number of names per manifest", namesPerManifest);
  cmd.AddValue("dataPacketSize", "Data Packet size", dataPacketSize);
  cmd.Parse(argc, argv);

  int nodeCount = 3;
  int radius = 50;
  
  // Creating nodes
  NodeContainer nodes;
  nodes.Create(nodeCount);
  
  // Connecting nodes using two links
  PointToPointHelper p2p;
  
  //Make producer be at (0,0)
  int diff_x = radius * cos(0);
  int diff_y = radius * sin(0);
  
  for (int i=0; i<nodeCount; i++) {
        
        int x = radius * cos(2 * PI * i / nodeCount) - diff_x;
        int y = radius * sin(2 * PI * i / nodeCount) - diff_y;
        AnimationInterface::SetConstantPosition (nodes.Get(i), x, y);
        
        for(int j=0;j<i;j++)
            p2p.Install(nodes.Get(i), nodes.Get(j));
  }
  
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
  for(int i=1; i<nodeCount;i++)
  {
      ndn::AppHelper consumerHelper("NTorrentConsumerApp");
      consumerHelper.SetAttribute("Prefix", StringValue("/"));
      consumerHelper.SetAttribute("namesPerSegment", IntegerValue(namesPerSegment));
      consumerHelper.SetAttribute("namesPerManifest", IntegerValue(namesPerManifest));
      consumerHelper.SetAttribute("dataPacketSize", IntegerValue(dataPacketSize));
      consumerHelper.Install(nodes.Get(i)).Start(Seconds(3.0+i*5));
  }

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
