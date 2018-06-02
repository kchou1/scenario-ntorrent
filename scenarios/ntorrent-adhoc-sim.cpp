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
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"

#include "src/util/shared-constants.hpp"
#include "../extensions/ntorrent-consumer-app.hpp"
#include "../extensions/ntorrent-producer-app.hpp"

#include "simulation-common.hpp"

#include <vector>

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

  // parameters
  uint32_t numPeers = 5;
  uint32_t wifiRange = 50;
  uint32_t speedMin = 2;
  uint32_t speedMax = 10;

  // Read optional command-line parameters (e.g., enable visualizer with ./waf --run=<> --visualize
  CommandLine cmd;
  cmd.AddValue("numPeers", "number of peers", numPeers);
  cmd.AddValue("wifiRange", "Wifi Range (m)", wifiRange);
  cmd.AddValue("speedMix", "Minimum speed (m/s)", speedMin);
  cmd.AddValue("speedMax", "Maximum speed (m/s)", speedMax);
  cmd.Parse(argc, argv);

  // Creating nodes
  NodeContainer nodes;
  nodes.Create(numPeers);

  WifiHelper wifi = WifiHelper::Default ();
  // wifi.SetRemoteStationManager ("ns3::AarfWifiManager");
  wifi.SetStandard (WIFI_PHY_STANDARD_80211a);
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode", StringValue ("OfdmRate24Mbps"));

  YansWifiChannelHelper wifiChannel;// = YansWifiChannelHelper::Default ();
  wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel.AddPropagationLoss("ns3::RangePropagationLossModel", "MaxRange", DoubleValue(wifiRange));


  //YansWifiPhy wifiPhy = YansWifiPhy::Default();
  YansWifiPhyHelper wifiPhyHelper = YansWifiPhyHelper::Default ();
  wifiPhyHelper.SetChannel (wifiChannel.Create ());
  wifiPhyHelper.Set("TxPowerStart", DoubleValue(5));
  wifiPhyHelper.Set("TxPowerEnd", DoubleValue(5));


  NqosWifiMacHelper wifiMacHelper = NqosWifiMacHelper::Default();
  wifiMacHelper.SetType("ns3::AdhocWifiMac");

  MobilityHelper mobility;

  // Setup mobility model
  Ptr<RandomRectanglePositionAllocator> randomPosAlloc =
    CreateObject<RandomRectanglePositionAllocator>();
  Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable>();
  x->SetAttribute("Min", DoubleValue(0));
  x->SetAttribute("Max", DoubleValue(500));
  randomPosAlloc->SetX(x);
  Ptr<UniformRandomVariable> y = CreateObject<UniformRandomVariable>();
  y->SetAttribute("Min", DoubleValue(0));
  y->SetAttribute("Max", DoubleValue(500));
  randomPosAlloc->SetY(y);

  mobility.SetPositionAllocator(randomPosAlloc);
  std::stringstream ss1;
  ss1 << "ns3::UniformRandomVariable[Min=" << speedMin << "|Max=" << speedMax << "]";

  // std::stringstream ss2;
  // ss2 << "ns3::UniformRandomVariable[Min=" << directionMin << "|Max=" << directionMax << "]";

  mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel", "Bounds",
                            RectangleValue(Rectangle(0, 500, 0, 500)), "Time",
                            ns3::TimeValue(Seconds(20)), "Speed", StringValue(ss1.str()));

  // Make mobile nodes move
  mobility.Install(nodes);

  ////////////////
  // 1. Install Wifi
  NetDeviceContainer wifiNetDevices = wifi.Install(wifiPhyHelper, wifiMacHelper, nodes);

  // 2. Install Mobility model
  mobility.Install(nodes);

  // 3. Install NDN stack on all nodes
  StackHelper ndnHelper;
  ndnHelper.SetDefaultRoutes(true);
  ndnHelper.InstallAll();

  // Installing applications
  ndn::AppHelper p1("NTorrentAdHocAppNaive");
  p1.SetAttribute("TorrentPrefix", StringValue("movie1"));
  p1.SetAttribute("NumberOfPackets", StringValue("10"));
  p1.SetAttribute("BeaconTimer", StringValue("1s"));
  p1.SetAttribute("RandomTimerRange", StringValue("20ms"));
  p1.SetAttribute("TorrentProducer", BooleanValue(false));
  // Install the app stack on all the peers except for the original torrent producer
  for (int i = 0; i < numPeers - 1; i++) {
    p1.SetAttribute("NodeId", IntegerValue(i));
    ApplicationContainer peer1 = p1.Install(nodes.Get(i));
    peer1.Start(Seconds(0));
    FibHelper::AddRoute(nodes.Get(i), "/beacon", std::numeric_limits<int32_t>::max());
    FibHelper::AddRoute(nodes.Get(i), "/bitmap", std::numeric_limits<int32_t>::max());
    FibHelper::AddRoute(nodes.Get(i), "/movie1", std::numeric_limits<int32_t>::max());
  }

  // original torrent producer
  ndn::AppHelper p2("NTorrentAdHocAppNaive");
  p2.SetAttribute("TorrentPrefix", StringValue("movie1"));
  p2.SetAttribute("NumberOfPackets", StringValue("10"));
  p2.SetAttribute("NodeId", IntegerValue(numPeers - 1));
  p2.SetAttribute("BeaconTimer", StringValue("1s"));
  p2.SetAttribute("RandomTimerRange", StringValue("20ms"));
  p2.SetAttribute("TorrentProducer", BooleanValue(true));
  ApplicationContainer peer2 = p2.Install(nodes.Get(numPeers - 1));
  peer2.Start(Seconds(0));
  FibHelper::AddRoute(nodes.Get(numPeers - 1), "/beacon", std::numeric_limits<int32_t>::max());
  FibHelper::AddRoute(nodes.Get(numPeers - 1), "/bitmap", std::numeric_limits<int32_t>::max());
  FibHelper::AddRoute(nodes.Get(numPeers - 1), "/movie1", std::numeric_limits<int32_t>::max());

  Simulator::Stop(Seconds(20.0));

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
