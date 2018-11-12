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

int
main(int argc, char *argv[])
{

  // setting default parameters for PointToPoint links and channels
  Config::SetDefault("ns3::PointToPointNetDevice::DataRate", StringValue("32kbps"));
  Config::SetDefault("ns3::PointToPointChannel::Delay", StringValue("10ms"));

  // Read optional command-line parameters (e.g., enable visualizer with ./waf --run=<> --visualize
  CommandLine cmd;
  cmd.Parse(argc, argv);

  // Creating nodes
  NodeContainer nodes;
  nodes.Create(3);

  AnimationInterface::SetConstantPosition (nodes.Get(0), 20, 20);
  AnimationInterface::SetConstantPosition (nodes.Get(1), 60, 60);

  WifiHelper wifi = WifiHelper::Default ();
  // wifi.SetRemoteStationManager ("ns3::AarfWifiManager");
  wifi.SetStandard (WIFI_PHY_STANDARD_80211a);
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode", StringValue ("OfdmRate24Mbps"));

  YansWifiChannelHelper wifiChannel;// = YansWifiChannelHelper::Default ();
  wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel.AddPropagationLoss("ns3::RangePropagationLossModel", "MaxRange", DoubleValue(150));


  //YansWifiPhy wifiPhy = YansWifiPhy::Default();
  YansWifiPhyHelper wifiPhyHelper = YansWifiPhyHelper::Default ();
  wifiPhyHelper.SetChannel (wifiChannel.Create ());
  wifiPhyHelper.Set("TxPowerStart", DoubleValue(5));
  wifiPhyHelper.Set("TxPowerEnd", DoubleValue(5));


  NqosWifiMacHelper wifiMacHelper = NqosWifiMacHelper::Default();
  wifiMacHelper.SetType("ns3::AdhocWifiMac");

  MobilityHelper mobility;
  Ptr<ListPositionAllocator> posAlloc = CreateObject<ListPositionAllocator>();
  posAlloc->Add(Vector(0.0, 0.0, 0.0));
  posAlloc->Add(Vector(0.0, 50.0, 0.0));
  posAlloc->Add(Vector(50.0, 150.0, 0.0));

  mobility.SetPositionAllocator(posAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

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
  // ndn::AppHelper p1("NTorrentAdHocNewPrioritization");
  ndn::AppHelper p1("NTorrentAdHocAppNaive");
  p1.SetAttribute("TorrentPrefix", StringValue("movie1"));
  p1.SetAttribute("NumberOfPackets", StringValue("10"));
  p1.SetAttribute("NodeId", StringValue("0"));
  p1.SetAttribute("BeaconTimer", StringValue("1s"));
  p1.SetAttribute("RandomTimerRange", StringValue("20ms"));
  p1.SetAttribute("TorrentProducer", BooleanValue(true));
  p1.SetAttribute("PureForwarder", BooleanValue(false));
  p1.SetAttribute("ForwardProbability", StringValue("10"));
  //p1.SetAttribute("DataUsefulToAll", BooleanValue(true));
  ApplicationContainer peer1 = p1.Install(nodes.Get(0));
  peer1.Start(Seconds(0));
  FibHelper::AddRoute(nodes.Get(0), "/beacon", std::numeric_limits<int32_t>::max());
  FibHelper::AddRoute(nodes.Get(0), "/bitmap", std::numeric_limits<int32_t>::max());
  FibHelper::AddRoute(nodes.Get(0), "/movie1", std::numeric_limits<int32_t>::max());

  // ndn::AppHelper p2("NTorrentAdHocNewPrioritization");
  ndn::AppHelper p2("NTorrentAdHocAppNaive");
  p2.SetAttribute("TorrentPrefix", StringValue("movie1"));
  p2.SetAttribute("NumberOfPackets", StringValue("10"));
  p2.SetAttribute("NodeId", StringValue("1"));
  p2.SetAttribute("BeaconTimer", StringValue("1s"));
  p2.SetAttribute("RandomTimerRange", StringValue("5ms"));
  p2.SetAttribute("TorrentProducer", BooleanValue(false));
  p2.SetAttribute("PureForwarder", BooleanValue(true));
  p2.SetAttribute("ForwardProbability", StringValue("50"));
  //p1.SetAttribute("DataUsefulToAll", BooleanValue(true));
  //p2.SetAttribute("DataUsefulToAll", BooleanValue(true));
  //p2.SetAttribute("HasHalfData", BooleanValue(true));
  ApplicationContainer peer2 = p2.Install(nodes.Get(1));
  peer2.Start(Seconds(0));
  FibHelper::AddRoute(nodes.Get(1), "/beacon", std::numeric_limits<int32_t>::max());
  FibHelper::AddRoute(nodes.Get(1), "/bitmap", std::numeric_limits<int32_t>::max());
  FibHelper::AddRoute(nodes.Get(1), "/movie1", std::numeric_limits<int32_t>::max());

  // Installing applications
  // ndn::AppHelper p3("NTorrentAdHocNewPrioritization");
  ndn::AppHelper p3("NTorrentAdHocAppNaive");
  p3.SetAttribute("TorrentPrefix", StringValue("movie1"));
  p3.SetAttribute("NumberOfPackets", StringValue("10"));
  p3.SetAttribute("NodeId", StringValue("2"));
  p3.SetAttribute("BeaconTimer", StringValue("1s"));
  p3.SetAttribute("RandomTimerRange", StringValue("20ms"));
  p3.SetAttribute("TorrentProducer", BooleanValue(false));
  p3.SetAttribute("PureForwarder", BooleanValue(false));
  p3.SetAttribute("ForwardProbability", StringValue("10"));
  //p3.SetAttribute("DataUsefulToAll", BooleanValue(true));
  ApplicationContainer peer3 = p3.Install(nodes.Get(2));
  peer3.Start(Seconds(0));
  FibHelper::AddRoute(nodes.Get(2), "/beacon", std::numeric_limits<int32_t>::max());
  FibHelper::AddRoute(nodes.Get(2), "/bitmap", std::numeric_limits<int32_t>::max());
  FibHelper::AddRoute(nodes.Get(2), "/movie1", std::numeric_limits<int32_t>::max());

  Simulator::Stop(Seconds(5.0));

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
