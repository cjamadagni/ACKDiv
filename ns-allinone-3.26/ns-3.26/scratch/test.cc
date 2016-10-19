#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include <fstream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("TestScriptExample");


int 
main (int argc, char *argv[])
{
 

uint32_t nWifi = 3;

CommandLine cmd;
cmd.AddValue ("nWifi", "Number of wifi STA devices", nWifi);
cmd.Parse (argc,argv);

/* Creating a node container with two nodes */
NodeContainer p2pNodes;
p2pNodes.Create (2);

PointToPointHelper pointToPoint;
pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

NetDeviceContainer p2pDevices;
p2pDevices = pointToPoint.Install (p2pNodes);

NodeContainer wifiStaNodes;
wifiStaNodes.Create (nWifi);
NodeContainer wifiApNode = p2pNodes.Get (0);

/* Configuring MAC and physical layer properties of the WiFi stations */ 
YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();

phy.SetChannel (channel.Create ());

WifiHelper wifi = WifiHelper::Default ();
wifi.SetRemoteStationManager ("ns3::AarfWifiManager");

WifiMacHelper mac;

/* Setting the rate control algorithm */
Ssid ssid = Ssid ("ns-3-ssid");
mac.SetType ("ns3::StaWifiMac","Ssid", SsidValue (ssid), "ActiveProbing", BooleanValue (false));

NetDeviceContainer staDevices;
staDevices = wifi.Install (phy, mac, wifiStaNodes);

/* Configuring an AP */
mac.SetType ("ns3::ApWifiMac","Ssid", SsidValue (ssid));
NetDeviceContainer apDevices;
apDevices = wifi.Install (phy, mac, wifiApNode);

/* Configuring the mobility models */
MobilityHelper mobility;
mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
  "MinX", DoubleValue (0.0),
  "MinY", DoubleValue (0.0),
  "DeltaX", DoubleValue (5.0),
  "DeltaY", DoubleValue (10.0),
  "GridWidth", UintegerValue (3),
  "LayoutType", StringValue ("RowFirst"));

mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel", "Bounds", RectangleValue (Rectangle (-50, 50, -50, 50)));

mobility.Install (wifiStaNodes);
mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
mobility.Install (wifiApNode);

/* Installing Protocol Stacks */
InternetStackHelper stack;
stack.Install (p2pNodes);
//stack.Install (wifiApNode);
stack.Install (wifiStaNodes);

/* Setting IPs */
Ipv4AddressHelper address;

address.SetBase ("10.1.1.0", "255.255.255.0");
Ipv4InterfaceContainer p2pInterfaces;
p2pInterfaces = address.Assign (p2pDevices);

address.SetBase ("10.1.3.0", "255.255.255.0");
Ipv4InterfaceContainer stationInterfaces;
stationInterfaces = address.Assign (staDevices);
Ipv4InterfaceContainer accessPointInterfaces;
accessPointInterfaces = address.Assign (apDevices);

NS_LOG_INFO("Creating Applications...");

uint16_t port = 9090;
BulkSendHelper source ("ns3::TcpSocketFactory", InetSocketAddress (stationInterfaces.GetAddress(2), port));
source.SetAttribute ("MaxBytes", UintegerValue (200000));
ApplicationContainer sourceApps = source.Install (p2pNodes.Get (1));
sourceApps.Start (Seconds (0.0));
sourceApps.Stop (Seconds (10.0));


PacketSinkHelper sink ("ns3::TcpSocketFactory",
                         InetSocketAddress (Ipv4Address::GetAny (), port));
ApplicationContainer sinkApps = sink.Install (wifiStaNodes.Get (2));
sinkApps.Start (Seconds (0.0));
sinkApps.Stop (Seconds (10.0)); 

AsciiTraceHelper ascii;
pointToPoint.EnableAsciiAll(ascii.CreateFileStream("test.tr"));
pointToPoint.EnablePcapAll("test", true);
phy.EnablePcap ("test", apDevices.Get (0));

Ipv4GlobalRoutingHelper::PopulateRoutingTables();

NS_LOG_INFO("Running simulation...");


Simulator::Stop(Seconds(15.00));
Simulator::Run ();
Simulator::Destroy ();

NS_LOG_INFO("Done.");

Ptr<PacketSink> sink1 = DynamicCast<PacketSink>(sinkApps.Get(0));
std::cout<<"Total Bytes Received: "<<sink1->GetTotalRx()<<std::endl;
return 0;
}