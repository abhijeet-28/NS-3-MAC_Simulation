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

//
// Default network topology includes some number of AP nodes specified by
// the variable nWifis (defaults to two).  Off of each AP node, there are some
// number of STA nodes specified by the variable nStas (defaults to two).
// Each AP talks to its associated STA nodes.  There are bridge net devices
// on each AP node that bridge the whole thing into one network.
//
//      +-----+      +-----+            +-----+      +-----+
//      | STA |      | STA |            | STA |      | STA | 
//      +-----+      +-----+            +-----+      +-----+
//    192.168.0.2  192.168.0.3        192.168.0.5  192.168.0.6
//      --------     --------           --------     --------
//      WIFI STA     WIFI STA           WIFI STA     WIFI STA
//      --------     --------           --------     --------
//        ((*))       ((*))       |      ((*))        ((*))
//                                |
//              ((*))             |             ((*))
//             -------                         -------
//             WIFI AP   CSMA ========= CSMA   WIFI AP 
//             -------   ----           ----   -------
//             ##############           ##############
//                 BRIDGE                   BRIDGE
//             ##############           ############## 
//               192.168.0.1              192.168.0.4
//               +---------+              +---------+
//               | AP Node |              | AP Node |
//               +---------+              +---------+
//

#include "ns3/core-module.h"
#include "ns3/mobility-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/network-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/bridge-helper.h"
#include <vector>
#include <stdint.h>
#include <sstream>
#include <fstream>
#include<time.h>
#include "ns3/point-to-point-module.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;

int main (int argc, char *argv[])
{
  uint32_t nWifis = 2;
  uint32_t nStas[2]={8,4};
  //uint32_t nStas = 2;
  bool sendIp = true;
  bool writeMobility = false;
  int totalrate=11;

  uint32_t payloadSize = 1472;                       /* Transport layer payload size in bytes. */
  std::string dataRate = "1Mbps";                  /* Application layer datarate. */
  std::string tcpVariant = "ns3::TcpNewReno";        /* TCP variant type. */
  std::string phyRate = "HtMcs7";                    /* Physical layer bitrate. */
  double simulationTime = 2;                        /* Simulation time in seconds. */
  // bool pcapTracing = true;                          /* PCAP Tracing is enabled or not. */

  /* No fragmentation and no RTS/CTS */
  Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("999999"));
  // Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("1000"));
  // ns3::FlowMonitorHelper

  /* Configure TCP Options */
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (payloadSize));

  // WifiAddPropagationLoss ("ns3::LogDistancePropagationLossModel", "Exponent", DoubleValue (3.0), "ReferenceLoss", DoubleValue (40.0459));

  std::string phyMode ("DsssRate11Mbps");

  CommandLine cmd;
  cmd.AddValue ("nWifis", "Number of wifi networks", nWifis);
  // cmd.AddValue ("nStas", "Number of stations per wifi network", nStas);
  cmd.AddValue ("SendIp", "Send Ipv4 or raw packets", sendIp);
  cmd.AddValue ("writeMobility", "Write mobility trace", writeMobility);
  cmd.Parse (argc, argv);

  NodeContainer backboneNodes;
  NetDeviceContainer backboneDevices;
  Ipv4InterfaceContainer backboneInterfaces;
  std::vector<NodeContainer> staNodes;
  std::vector<NetDeviceContainer> staDevices;
  std::vector<NetDeviceContainer> apDevices;
  std::vector<Ipv4InterfaceContainer> staInterfaces;
  std::vector<Ipv4InterfaceContainer> apInterfaces;

  InternetStackHelper stack;
  CsmaHelper csma;
  Ipv4AddressHelper ip;
  ip.SetBase ("192.168.0.0", "255.255.255.0");

  backboneNodes.Create (nWifis);
  stack.Install (backboneNodes);

  backboneDevices = csma.Install (backboneNodes);

  double wifiX = 0.0;

  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
  wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO); 

  for (uint32_t i = 0; i < nWifis; ++i)
    {
      // calculate ssid for wifi subnetwork
      std::ostringstream oss;
      oss << "wifi-default-" << i;
      Ssid ssid = Ssid (oss.str ());

      NodeContainer sta;
      NetDeviceContainer staDev;
      NetDeviceContainer apDev;
      Ipv4InterfaceContainer staInterface;
      Ipv4InterfaceContainer apInterface;
      MobilityHelper mobility;
      BridgeHelper bridge;

      WifiHelper wifi;
      wifi.SetStandard(WIFI_PHY_STANDARD_80211b);

      YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default();
      // ns-3 supports RadioTap and Prism tracing extensions for 802.11
      wifiPhy.SetPcapDataLinkType(YansWifiPhyHelper::DLT_IEEE802_11_RADIO);

      YansWifiChannelHelper wifiChannel;
      // reference loss must be changed since 802.11b is operating at 2.4GHz
      wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
      wifiChannel.AddPropagationLoss("ns3::LogDistancePropagationLossModel", "ReferenceLoss", DoubleValue (40.0459));

      wifiPhy.SetChannel(wifiChannel.Create());
      //Add a non-Qos upper mac, and disable the rate control
      NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default();


      sta.Create (nStas[i]);
      mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                     "MinX", DoubleValue (wifiX),
                                     "MinY", DoubleValue (0.0),
                                     "DeltaX", DoubleValue (5.0),
                                     "DeltaY", DoubleValue (5.0),
                                     "GridWidth", UintegerValue (1),
                                     "LayoutType", StringValue ("RowFirst"));


      // setup the AP.
      mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
      mobility.Install (backboneNodes.Get (i));
      wifiMac.SetType ("ns3::ApWifiMac",
                       "Ssid", SsidValue (ssid));
      apDev = wifi.Install (wifiPhy, wifiMac, backboneNodes.Get (i));

      NetDeviceContainer bridgeDev;
      bridgeDev = bridge.Install (backboneNodes.Get (i), NetDeviceContainer (apDev, backboneDevices.Get (i)));

      // assign AP IP address to bridge, not wifi
      apInterface = ip.Assign (bridgeDev);

      // setup the STAs
      stack.Install (sta);
      mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                                 "Mode", StringValue ("Time"),
                                 "Time", StringValue ("2s"),
                                 "Speed", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"),
                                 "Bounds", RectangleValue (Rectangle (wifiX, wifiX+5.0,0.0, (nStas[i]+1)*5.0)));
      mobility.Install (sta);
      wifiMac.SetType ("ns3::StaWifiMac",
                       "Ssid", SsidValue (ssid));
      staDev = wifi.Install (wifiPhy, wifiMac, sta);
      staInterface = ip.Assign (staDev);

      // save everything in containers.
      staNodes.push_back (sta);
      apDevices.push_back (apDev);
      apInterfaces.push_back (apInterface);
      staDevices.push_back (staDev);
      staInterfaces.push_back (staInterface);

      wifiX += 20.0;
    }




  /* Populate routing table */
  // Ipv4GlobalRoutingHelper::PopulateRoutingTables ();


  /* Install TCP Receiver on the access point */
  PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), 9));
  ApplicationContainer sinkApp = sinkHelper.Install (staNodes[0]);  
  sinkApp.Add(sinkHelper.Install (staNodes[1]));
  // sink = StaticCast<PacketSink> (sinkApp.Get(0));

  /* Install TCP/UDP Transmitter on the station */  
  ApplicationContainer serverApp;//1,serverApp2;
  srand(time(NULL));

  int x=rand()%totalrate;
  int y=rand()%(totalrate-x);
  
  
  for(int sender=0;sender<8;sender++)
  {

     double valself=(x*1.0)/56.0;
    for(int rcv=0;rcv<8;rcv++)
    {
        
        if(sender==rcv) continue;
        OnOffHelper server ("ns3::TcpSocketFactory", (InetSocketAddress (staInterfaces[0].GetAddress (rcv), 9)));
        server.SetAttribute ("PacketSize", UintegerValue (payloadSize));
        server.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
        server.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
        server.SetAttribute ("DataRate", DataRateValue (DataRate (std::to_string(valself)+"Mbps")));
        serverApp.Add(server.Install(staNodes[0].Get(sender)));    
    }
    double valcross=(y*1.0)/32.0;
    for(int rcv=0;rcv<4;rcv++)
    {
        
        OnOffHelper server ("ns3::TcpSocketFactory", (InetSocketAddress (staInterfaces[1].GetAddress (rcv), 9)));
        server.SetAttribute ("PacketSize", UintegerValue (payloadSize));
        server.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
        server.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
        server.SetAttribute ("DataRate", DataRateValue (DataRate (std::to_string(valcross)+"Mbps")));
        serverApp.Add(server.Install(staNodes[0].Get(sender)));    
    }
    
  }



  for(int sender=0;sender<4;sender++)
  {     
    double valself=(x*1.0)/(12.0);
    for(int rcv=0;rcv<4;rcv++)
    {
        if(sender==rcv) continue;
        OnOffHelper server ("ns3::TcpSocketFactory", (InetSocketAddress (staInterfaces[1].GetAddress (rcv), 9)));
        server.SetAttribute ("PacketSize", UintegerValue (payloadSize));
        server.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
        server.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
        server.SetAttribute ("DataRate", DataRateValue (DataRate (std::to_string(valself)+"Mbps")));
        serverApp.Add(server.Install(staNodes[1].Get(sender)));
    }
    double valcross=((11-x-y)*1.0)/(32.0);
    for(int rcv=0;rcv<8;rcv++)
    {
        if(sender==rcv) continue;
        OnOffHelper server ("ns3::TcpSocketFactory", (InetSocketAddress (staInterfaces[0].GetAddress (rcv), 9)));
        server.SetAttribute ("PacketSize", UintegerValue (payloadSize));
        server.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
        server.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
        server.SetAttribute ("DataRate", DataRateValue (DataRate (std::to_string(valcross)+"Mbps")));
        serverApp.Add(server.Install(staNodes[1].Get(sender)));
    }
    
  }


  sinkApp.Start (Seconds (0.0));
  serverApp.Start (Seconds (1.0));


  // Address dest;
  // std::string protocol;
  // if (sendIp)
  //   {
  //     dest = InetSocketAddress (staInterfaces[1].GetAddress (1), 1025);
  //     protocol = "ns3::TcpSocketFactory";
  //   }
  // else
  //   {
  //     PacketSocketAddress tmp;
  //     tmp.SetSingleDevice (staDevices[0].Get (0)->GetIfIndex ());
  //     tmp.SetPhysicalAddress (staDevices[1].Get (0)->GetAddress ());
  //     tmp.SetProtocol (0x807);
  //     dest = tmp;
  //     protocol = "ns3::PacketSocketFactory";
  //   }

  // OnOffHelper onoff = OnOffHelper (protocol, dest);
  // onoff.SetConstantRate (DataRate ("500kb/s"));
  // ApplicationContainer apps = onoff.Install (staNodes[0].Get (0));
  // apps.Start (Seconds (0.5));
  // apps.Stop (Seconds (3.0));


  FlowMonitorHelper flowHelper;
  
   
       

  Ptr<FlowMonitor> flowMonitor = flowHelper.InstallAll();
  
  // Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowHelper.GetClassifier()); 
  // Ptr<FlowMonitor> monitor1 = flowHelper.GetMonitor();
  // flowMonitor->SetFlowClassifier(classifier);
  // Ptr<FlowMonitor> fm=flowHelper.InstallAll(ap.get(0));
  

  wifiPhy.EnablePcap ("wifi-wired-bridging", apDevices[0]);
  wifiPhy.EnablePcap ("wifi-wired-bridging", apDevices[1]);

  // if (writeMobility)
  //   {
  //     AsciiTraceHelper ascii;
  //     MobilityHelper::EnableAsciiAll (ascii.CreateFileStream ("wifi-wired-bridging.mob"));
  //   }

  Simulator::Stop (Seconds (simulationTime + 1));
  Simulator::Run ();



  
  flowMonitor->CheckForLostPackets ();
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowHelper.GetClassifier ());
  std::map<FlowId, FlowMonitor::FlowStats> stats = flowMonitor->GetFlowStats ();


  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator iter = stats.begin (); iter != stats.end (); ++iter){
    // cout<<iter->first;


      Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (iter->first);

      

      NS_LOG_UNCOND("Flow ID: " << iter->first << " Src Addr " << t.sourceAddress << " Dst Addr " << t.destinationAddress);
      NS_LOG_UNCOND("Tx Packets = " << iter->second.txPackets);
      NS_LOG_UNCOND("Rx Packets = " << iter->second.rxPackets);
      NS_LOG_UNCOND("Jitter Sum = " << iter->second.jitterSum);

      
      
      NS_LOG_UNCOND("DelaySum = " << iter->second.delaySum);

      double tput=iter->second.rxBytes * 8.0 / (iter->second.timeLastRxPacket.GetSeconds()-iter->second.timeFirstTxPacket.GetSeconds()) / 1024 ;
      NS_LOG_UNCOND("Throughput: " << tput << " Kbps");
      NS_LOG_UNCOND("x: " << x << " Kbps");
      NS_LOG_UNCOND("y: " << y << " Kbps");
      // th_put[index]=tput;

      

     
        
  }
  

  // FlowProbe::Stats stats1=flowMonitor.GetStatus();



NS_LOG_UNCOND("\n\nReport\n\n");












flowMonitor->SerializeToXmlFile("report1.xml", true, true);
  

  Simulator::Destroy ();

  








}
