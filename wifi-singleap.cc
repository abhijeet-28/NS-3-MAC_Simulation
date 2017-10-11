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
 #include<math.h>
#include "ns3/point-to-point-module.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;


int getindex(int i)
{
  if(i<10)
  {
    return i-2;
  }
  else
  {
    return i-3;
  }
}

int main (int argc, char *argv[])
{
  for(double percent=0.1;percent<=0.9;percent+=0.1){
  uint32_t nWifis = 1;
  uint32_t nStas[1]={8};
  //uint32_t nStas = 2;
  bool sendIp = true;
  bool writeMobility = false;
  
  double totalrate=11.0*percent;

  uint32_t payloadSize = 1472;                       /* Transport layer payload size in bytes. */
  std::string dataRate = "1Mbps";                  /* Application layer datarate. */
  std::string tcpVariant = "ns3::TcpNewReno";        /* TCP variant type. */
  std::string phyRate = "HtMcs7";                    /* Physical layer bitrate. */
  double simulationTime = 5;                        /* Simulation time in seconds. */
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
  
  // sink = StaticCast<PacketSink> (sinkApp.Get(0));

  /* Install TCP/UDP Transmitter on the station */  
  ApplicationContainer serverApp;//1,serverApp2;
  srand(time(NULL));

  
  double arr[56];
  double sum=0.0;
  for(int i=0;i<56;i++)
  {
    arr[i]=rand()%20+10;
    sum+=arr[i];
  }
  // int count=0;
  double anss=0.0;
  for(int sender=0;sender<8;sender++)
  {

     
    for(int rcv=0;rcv<8;rcv++)
    {
        
        if(sender==rcv) continue;
        double valself=(totalrate*1.0)/56.0;
        anss+=valself;
        OnOffHelper server ("ns3::TcpSocketFactory", (InetSocketAddress (staInterfaces[0].GetAddress (rcv), 9)));
        server.SetAttribute ("PacketSize", UintegerValue (payloadSize));
        server.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
        server.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
        server.SetAttribute ("DataRate", DataRateValue (DataRate (std::to_string(valself)+"Mbps")));
        serverApp.Add(server.Install(staNodes[0].Get(sender)));    
    }
    
    
  }
std::cout<<"fasfas "<<anss<<std::endl;


  
    
  


  sinkApp.Start (Seconds (0.0));
  serverApp.Start (Seconds (1.0));




  FlowMonitorHelper flowHelper;
  
   
       

  Ptr<FlowMonitor> flowMonitor = flowHelper.InstallAll();
  
  

  wifiPhy.EnablePcap ("wifi-wired-bridging", apDevices[0]);
  

  
  Simulator::Stop (Seconds (simulationTime + 1));
  Simulator::Run ();


  double matrix[8][8];
  for(int i=0;i<8;i++)
  {
    for(int j=0;j<8;j++)
    {
      matrix[i][j]=0.0;
    }
  }
  
  flowMonitor->CheckForLostPackets ();
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowHelper.GetClassifier ());
  std::map<FlowId, FlowMonitor::FlowStats> stats = flowMonitor->GetFlowStats ();
  // double tput_1=0;
  // double tput_2=0;

  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator iter = stats.begin (); iter != stats.end (); ++iter){
    // cout<<iter->first;


      Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (iter->first);

      
      uint32_t saddress=t.sourceAddress.Get();
      int index1=getindex(saddress%3232235520);
      uint32_t daddress=t.destinationAddress.Get();
      int index2=getindex(daddress%3232235520);

      

      // NS_LOG_UNCOND("Flow ID: " << iter->first << " Src Addr " << t.sourceAddress << " Dst Addr " << t.destinationAddress);
      // NS_LOG_UNCOND("Tx Packets = " << iter->second.txPackets);
      // NS_LOG_UNCOND("Rx Packets = " << iter->second.rxPackets);
      // NS_LOG_UNCOND("Jitter Sum = " << iter->second.jitterSum);

      
      
      // NS_LOG_UNCOND("DelaySum = " << iter->second.delaySum);

      double tput=iter->second.rxBytes * 8.0 / (iter->second.timeLastRxPacket.GetSeconds()-iter->second.timeFirstTxPacket.GetSeconds()) / 1024 ;
      matrix[index1][index2]+=tput;
      // NS_LOG_UNCOND("Throughput: " << tput << " Kbps");
     
      // th_put[index]=tput;

      

     
        
  }
  

  // FlowProbe::Stats stats1=flowMonitor.GetStatus();



NS_LOG_UNCOND("\n\nReport\n\n");


      
double totalsum=0.0;

for(int i=0;i<8;i++)
{
  for(int j=0;j<8;j++)
  {
    totalsum+=matrix[i][j];
    std::cout<<matrix[i][j]<<" ";
  }
  std::cout<<std::endl;
}



double ans=(totalsum*1.0)/1024.0;
std::cout<<"network1 : "<<percent*100<<" "<<ans<< std::endl;





// flowMonitor->SerializeToXmlFile("report1.xml", true, true);
  
double throughput = 0;
          uint32_t totalPacketsThrough = 0;
          for(int ii = 0; ii<8; ii++){
            totalPacketsThrough  =  totalPacketsThrough +  DynamicCast<PacketSink> (sinkApp.Get (ii))->GetTotalRx ();
          }

          throughput = totalPacketsThrough * 8 / (simulationTime * 1000000.0); //Mbit/s
          std::cout << throughput << " Mbit/s" <<std::endl;
  Simulator::Destroy ();
}
  








}
