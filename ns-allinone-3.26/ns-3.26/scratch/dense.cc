/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 */

#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/propagation-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include <fstream>
#include "ns3/gnuplot.h"
using namespace std;
using namespace ns3;

// Default Network Topology
//
//   Wifi 10.1.3.0
//                 AP
//  *    *    *    *
//  |    |    |    |    10.1.1.0
// n4   n3   n2   n0 -------------- n1
//                   point-to-point


NS_LOG_COMPONENT_DEFINE ("ThirdScriptExample");
Ptr<PacketSink> sink1;
const uint32_t nWifi = 30;
uint64_t lastTotalRx[nWifi] = {};    /* The value of the last total received bytes */
uint32_t MacTxDropCount, PhyTxDropCount, PhyRxDropCount;
ApplicationContainer sinkApps;
std::fstream file;

void initialize ()
{
  for (uint32_t i = 0; i < nWifi; i++)
    lastTotalRx[i] = 0;
}

void
CalculateThroughput ()
{
  Time now = Simulator::Now ();
  double sum = 0, avg = 0;
  sum = 0;
  for(uint32_t i = 0; i< nWifi; i++)
  {
    sink1 = DynamicCast<PacketSink>(sinkApps.Get(i));
    double cur = (sink1->GetTotalRx() - lastTotalRx[i]) * (double) 8/1e5;     /* Convert Application RX Packets to MBits. */
    lastTotalRx[i] = sink1->GetTotalRx ();
    sum += cur;
  }
  avg = sum/nWifi;
  //Write the value of avg Throughput to a file.
  file << now.GetSeconds () << "\t" <<avg << std::endl;
  std::cout << now.GetSeconds () << "s: \t" << " " << avg << " Mbit/s" << std::endl;
  Simulator::Schedule (MilliSeconds (100), &CalculateThroughput);
}

int
experiment (std::string rate)
{
  double simulationTime = 10.0;

  /* Setting threshold=2200 disables RTS-CTS */
  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", UintegerValue(2200));
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
  NodeContainer ServerNode = p2pNodes.Get (1);

  YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
  MobilityHelper mobility;

  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (10.0),
                                 "MinY", DoubleValue (10.0),
                                 "DeltaX", DoubleValue (10.0),
                                 "DeltaY", DoubleValue (-10.0),
                                 "GridWidth", UintegerValue (6),
                                 "LayoutType", StringValue ("RowFirst"));

  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");


  mobility.Install (wifiStaNodes);

  mobility.Install (wifiApNode);

  Ptr<YansWifiChannel> wifiChannel = CreateObject <YansWifiChannel> ();
  Ptr<MatrixPropagationLossModel> lossModel = CreateObject<MatrixPropagationLossModel> ();
  lossModel->SetDefaultLoss (400); // set default loss to 200 dB (no link)
  for (size_t i = 0; i < nWifi; ++i) {
    lossModel->SetLoss (wifiStaNodes.Get (i)->GetObject<MobilityModel>(), p2pNodes.Get (0)->GetObject<MobilityModel>(), 10);
  }
  wifiChannel->SetPropagationLossModel (lossModel);
  wifiChannel->SetPropagationDelayModel (CreateObject <ConstantSpeedPropagationDelayModel> ());
  phy.SetChannel (wifiChannel);
  //phy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO);

  WifiHelper wifi;
  /*Uncomment to enable RAAs*/
  //wifi.SetRemoteStationManager (rate);
  wifi.SetStandard(WIFI_PHY_STANDARD_80211b);

  //Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/MacTxDrop", MakeCallback(&MacTxDrop));
  //Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyRxDrop", MakeCallback(&PhyRxDrop));
  //Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyTxDrop", MakeCallback(&PhyTxDrop));

  WifiMacHelper mac;
  Ssid ssid = Ssid ("ns-3-ssid");
  mac.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid),
               "ActiveProbing", BooleanValue (false));

  NetDeviceContainer staDevices;
  staDevices = wifi.Install (phy, mac, wifiStaNodes);

  mac.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid));

  NetDeviceContainer apDevices;
  apDevices = wifi.Install (phy, mac, wifiApNode);



  InternetStackHelper stack;
  stack.Install (ServerNode);
  stack.Install (wifiApNode);
  stack.Install (wifiStaNodes);

  Ipv4AddressHelper address;

  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer p2pInterfaces;
  p2pInterfaces = address.Assign (p2pDevices);

  address.SetBase ("10.1.3.0", "255.255.255.0");
  address.Assign (apDevices);
  Ipv4InterfaceContainer staInterfaces;
  staInterfaces = address.Assign (staDevices);

  for(uint32_t i = 0; i< nWifi; i++)
  {
    BulkSendHelper source ("ns3::TcpSocketFactory",
                          InetSocketAddress(staInterfaces.GetAddress (i), 5555));
    source.SetAttribute ("MaxBytes", UintegerValue (0));
    ApplicationContainer sourceApps = source.Install (p2pNodes.Get(1));
    sourceApps.Start(Seconds (0.0));
    sourceApps.Stop(Seconds (10.0));
  }
  PacketSinkHelper sink ("ns3::TcpSocketFactory",
                        InetSocketAddress (Ipv4Address::GetAny (), 5555));
  sinkApps = sink.Install (wifiStaNodes);
  sinkApps.Start (Seconds (0.0));
  sinkApps.Stop (Seconds (10.0));

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  Simulator::Schedule (MilliSeconds(100), CalculateThroughput);

  Simulator::Stop (Seconds (10.0));
  //phy.EnablePcapAll(rate);
  Simulator::Run ();
  Simulator::Destroy ();

  for(uint32_t i = 0; i< nWifi; i++)
  {
    sink1 = DynamicCast<PacketSink>(sinkApps.Get(i));
    //std::cout << rate <<" Node "<<i+1<<std::endl;
    std::cout << "\nNode "<<i+1<<std::endl;
    std::cout << "Total Bytes Received: " << sink1->GetTotalRx() << std::endl;
    std::cout << "Average Throughput: " << ((sink1->GetTotalRx() * 8) / (1e6  * simulationTime)) << " Mbit/s" << std::endl;
  }
  return 0;
}

int
main (int argc, char *argv[])
{
  bool verbose = true;

  CommandLine cmd;
  cmd.AddValue ("verbose", "Tell application to log if true", verbose);

  cmd.Parse (argc,argv);

  /* ... */
  string raas[] = { "ns3::ConstantRateWifiManager",
  				   "ns3::ArfWifiManager",
                   "ns3::AarfWifiManager",
                   "ns3::AarfcdWifiManager",
                   "ns3::AmrrWifiManager",
                   "ns3::CaraWifiManager",
                   "ns3::IdealWifiManager",
                   "ns3::MinstrelWifiManager",
                   "ns3::ParfWifiManager",
                   "ns3::AparfWifiManager",
                   "ns3::OnoeWifiManager",
                   "ns3::RraaWifiManager"};
  /*for (unsigned int i = 0; i < sizeof(raas)/sizeof(raas[0]); i++ )
  {
    file.open (raas[i] + " Average Throughput.txt", std::fstream::out);
    initialize ();
    experiment(raas[i]);
    file.close();
  }*/
  experiment(raas[0]);
  //PlotGraph();
  return 0;
}
