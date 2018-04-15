namespace ns3 {
namespace ndn {


void createAndInstall(ndn::AppHelper x, uint32_t namesPerSegment, 
        uint32_t namesPerManifest, uint32_t dataPacketSize, std::string type, 
        Ptr<Node> n, float start_time)
{
  x.SetAttribute("Prefix", StringValue("/"));
  x.SetAttribute("namesPerSegment", IntegerValue(namesPerSegment));
  x.SetAttribute("namesPerManifest", IntegerValue(namesPerManifest));
  x.SetAttribute("dataPacketSize", IntegerValue(dataPacketSize));
  x.Install(n).Start(Seconds(start_time));
}

}
}
