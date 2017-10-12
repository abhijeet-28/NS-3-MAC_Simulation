/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015, IMDEA Networks Institute
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
 * Author: Hany Assasa <hany.assasa@gmail.com>
.*
 * This is a simple example to test TCP over 802.11n (with MPDU aggregation enabled).
 *
 * Network topology:
 *
 *   Ap    STA
 *   *      *
 *   |      |
 *   n1     n2
 *
 * In this example, an HT station sends TCP packets to the access point. 
 * We report the total throughput received during a window of 100ms. 
 * The user can specify the application data rate and choose the variant
 * of TCP i.e. congestion control algorithm to use.
 */

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/wifi-module.h"
#include "ns3/flow-monitor-module.h"
#include<iostream>
#include<fstream>

NS_LOG_COMPONENT_DEFINE ("wifi-tcp-b");

using namespace ns3;
using std::ofstream;

Ptr<PacketSink> sink;                         /* Pointer to the packet sink application */
uint64_t lastTotalRx = 0;                     /* The value of the last total received bytes */

void
CalculateThroughput ()
{
  Time now = Simulator::Now ();                                         /* Return the simulator's virtual time. */
  double cur = (sink->GetTotalRx() - lastTotalRx) * (double) 8/1e5;     /* Convert Application RX Packets to MBits. */
  std::cout << now.GetSeconds () << "s: \t" << cur << " Mbit/s" << std::endl;
  lastTotalRx = sink->GetTotalRx ();
  Simulator::Schedule (MilliSeconds (100), &CalculateThroughput);
}

int
main(int argc, char *argv[])
{

  ofstream out;
  out.open("report.csv");
  
  out<<"percentage,"<<"Aver. Throughput,"<<"Total packets sent,"<<"Total packets received,"<<"Traffic dropped(%),"<<"Traffic Dropped(Rate),"<<"Aver. delay,"<<"Delay Std. Dev,"<<"Aver. Jitters,"<<"Jitters Std. Dev\n";


  double percentage=0.10;
  while(percentage<=0.90)
  {

    out<<percentage*100<<",";

  uint32_t payloadSize = 1472;                       /* Transport layer payload size in bytes. */
  std::string dataRate = "1Mbps";                  /* Application layer datarate. */
  std::string tcpVariant = "ns3::TcpNewReno";        /* TCP variant type. */
  std::string phyRate = "HtMcs7";                    /* Physical layer bitrate. */
  double simulationTime = 10;                        /* Simulation time in seconds. */
  bool pcapTracing = true;                          /* PCAP Tracing is enabled or not. */


  /* Command line argument parser setup. */
  CommandLine cmd;
  cmd.AddValue ("payloadSize", "Payload size in bytes", payloadSize);
  cmd.AddValue ("dataRate", "Application data ate", dataRate);
  cmd.AddValue ("tcpVariant", "Transport protocol to use: TcpTahoe, TcpReno, TcpNewReno, TcpWestwood, TcpWestwoodPlus ", tcpVariant);
  cmd.AddValue ("phyRate", "Physical layer bitrate", phyRate);
  cmd.AddValue ("simulationTime", "Simulation time in seconds", simulationTime);
  cmd.AddValue ("pcap", "Enable/disable PCAP Tracing", pcapTracing);
  cmd.Parse (argc, argv);

  /* No fragmentation and no RTS/CTS */
  Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("999999"));
  // Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("1000"));
  // ns3::FlowMonitorHelper

  /* Configure TCP Options */
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (payloadSize));

  // WifiAddPropagationLoss ("ns3::LogDistancePropagationLossModel", "Exponent", DoubleValue (3.0), "ReferenceLoss", DoubleValue (40.0459));


  std::string phyMode ("DsssRate11Mbps");
  NodeContainer ap;
  ap.Create(1);
  NodeContainer sta;
  sta.Create(8);
 
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
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode", StringValue(phyMode), "ControlMode", StringValue(phyMode));

  //setup the rest of the upper mac
  Ssid ssid = Ssid("wifi-default");
  // setup AP
  wifiMac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));
  
  NetDeviceContainer apDevice = wifi.Install(wifiPhy, wifiMac, ap);
  NetDeviceContainer devices = apDevice;

  //setup station
  wifiMac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid), "ActiveProbing", BooleanValue(false));
  NetDeviceContainer staDevice = wifi.Install(wifiPhy, wifiMac, sta);
  devices.Add(staDevice);

  //Configure mobility
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (0.0, 0.0, 0.0));
  positionAlloc->Add (Vector (5.0, 0.0, 0.0));
  positionAlloc->Add (Vector (0.0, 5.0, 0.0));


  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (ap);
  mobility.Install (sta);
  
  /* Internet stack */
  InternetStackHelper stack;
  stack.Install (ap);
  stack.Install (sta);

  Ipv4AddressHelper address;
  address.SetBase ("10.0.0.0", "255.255.255.0");
  Ipv4InterfaceContainer apInterface;
  apInterface = address.Assign (devices);
  
  /* Populate routing table */
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  /* Install TCP Receiver on the access point */
  PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), 9));
  ApplicationContainer sinkApp = sinkHelper.Install (ap);
  sink = StaticCast<PacketSink> (sinkApp.Get (0));

  /* Install TCP/UDP Transmitter on the station */
  OnOffHelper server ("ns3::TcpSocketFactory", (InetSocketAddress (apInterface.GetAddress (0), 9)));
  server.SetAttribute ("PacketSize", UintegerValue (payloadSize));
  server.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
  server.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  ApplicationContainer serverApp;
  double totalDataRate=11.0*percentage;
  srand (time(NULL));
  double arr[8];
  double sum=0;
  for(int i=0;i<8;i++)
  {
    arr[i]=rand()%20+10;
    sum+=arr[i];
  }
  for(int i=0;i<8;i++)
  {
    arr[i]=(arr[i]*totalDataRate)/sum;
    // NS_LOG_UNCOND(arr[i]);
    server.SetAttribute ("DataRate", DataRateValue (DataRate (std::to_string(arr[i])+"Mbps")));
    // NS_LOG_UNCOND("Yo");

    // ApplicationContainer appcont=;
    serverApp.Add(server.Install (sta.Get(i)));
  }
  

  // server.SetAttribute ("DataRate", DataRateValue (DataRate (dataRate)));
  //  ApplicationContainer serverApp=server.Install(sta);
 
  /* Start Applications */
  sinkApp.Start (Seconds (0.0));
  serverApp.Start (Seconds (1.0));
  Simulator::Schedule (Seconds (1.1), &CalculateThroughput);





  //Installing flow monitor
  FlowMonitorHelper flowHelper;
  
   
       

  Ptr<FlowMonitor> flowMonitor = flowHelper.InstallAll();
  Ptr<FlowMonitor> flowMonitor2 = flowHelper.Install(ap);
  // Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowHelper.GetClassifier()); 
  // Ptr<FlowMonitor> monitor1 = flowHelper.GetMonitor();
  // flowMonitor->SetFlowClassifier(classifier);
  // Ptr<FlowMonitor> fm=flowHelper.InstallAll(ap.get(0));
  

   // Enable Traces 
  if (pcapTracing)
    {
      wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO);
      wifiPhy.EnablePcap ("AccessPoint", apDevice);
      wifiPhy.EnablePcap ("Station", staDevice);
    }
  
  /* Start Simulation */

  Simulator::Stop (Seconds (simulationTime + 1));
  Simulator::Run ();

  double total=0;
  flowMonitor->CheckForLostPackets ();
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowHelper.GetClassifier ());
  std::map<FlowId, FlowMonitor::FlowStats> stats = flowMonitor->GetFlowStats ();

  double Jitters[16];
  double RxPackets[16];
  double TxPackets[16];
  double DelaySum[16];
  Ipv4Address source[16];
  Ipv4Address destination[16];
  
  // double th_put[16];

  double sum_delay=0;
  double sum_rxpackets=0;
  double sum_txpackets=0;
  double sum_jitters=0;


  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator iter = stats.begin (); iter != stats.end (); ++iter){
    // cout<<iter->first;


      Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (iter->first);

      int index=iter->first-1;
      source[index]=t.sourceAddress;
      destination[index]=t.destinationAddress;
      TxPackets[index]=iter->second.txPackets;
      RxPackets[index]=iter->second.rxPackets;
      Jitters[index]=iter->second.jitterSum.GetNanoSeconds();
      DelaySum[index]=iter->second.delaySum.GetNanoSeconds();

      NS_LOG_UNCOND("Flow ID: " << iter->first << " Src Addr " << t.sourceAddress << " Dst Addr " << t.destinationAddress);
      NS_LOG_UNCOND("Tx Packets = " << iter->second.txPackets);
      NS_LOG_UNCOND("Rx Packets = " << iter->second.rxPackets);
      NS_LOG_UNCOND("Jitter Sum = " << iter->second.jitterSum);

      double tput=iter->second.rxBytes * 8.0 / (iter->second.timeLastRxPacket.GetSeconds()-iter->second.timeFirstTxPacket.GetSeconds()) / 1024 ;
      NS_LOG_UNCOND("Throughput: " << tput << " Kbps");
      NS_LOG_UNCOND("DelaySum = " << iter->second.delaySum);
      // th_put[index]=tput;

      sum_delay+=DelaySum[index];
      sum_rxpackets+=RxPackets[index];
      sum_txpackets+=TxPackets[index];
      sum_jitters+=Jitters[index];

      total+=tput;
        
  }
  NS_LOG_UNCOND("Total throughput"<<total);

  // FlowProbe::Stats stats1=flowMonitor.GetStatus();



NS_LOG_UNCOND("\n\nReport\n\n");

double mean_jitters=(sum_jitters)/(sum_rxpackets-1);
NS_LOG_UNCOND("mean jitters="<<mean_jitters);

double mean_delay=(sum_delay)/(sum_rxpackets);


double variance_delay=0;
double variance_jitters=0;
for(int i=0;i<16;i++)
{
  if(RxPackets[i]>=2)
    {
      double temp=abs((Jitters[i])/(RxPackets[i]-1)-mean_jitters);
      // NS_LOG_UNCOND("temp="<<temp<<" "<<"index"<<i<<" "<<Jitters[i]);
      variance_jitters+=temp*temp;

      
    }
    if(RxPackets[i]>0)
    {
      double temp2=abs((DelaySum[i])/(RxPackets[i])-mean_delay);
      variance_delay+=temp2*temp2;

    }
    
  

}
variance_jitters=variance_jitters/16;
variance_delay=variance_delay/16;

NS_LOG_UNCOND("variance jitters="<<variance_jitters);
double deviation_jitters=sqrt(variance_jitters);
NS_LOG_UNCOND("deviation jitters="<<deviation_jitters);

NS_LOG_UNCOND("\n");
NS_LOG_UNCOND("mean delay="<<mean_delay);
NS_LOG_UNCOND("variance delay="<<variance_delay);
double deviation_delay=sqrt(variance_delay);
NS_LOG_UNCOND("deviation delay="<<deviation_delay);


NS_LOG_UNCOND("Total Packets Transmitted="<<sum_txpackets);
NS_LOG_UNCOND("Total Packets Received="<<sum_rxpackets);
double packets_lost=sum_txpackets-sum_rxpackets;
double percentage_loss=(packets_lost*100.0)/sum_txpackets;
double rate_packet_loss=(packets_lost)/simulationTime;
NS_LOG_UNCOND("Total Packets Lost="<<packets_lost);
NS_LOG_UNCOND("Percentage of Total Packets Lost="<<percentage_loss<<"%");
NS_LOG_UNCOND("Rate of Packets Lost="<<rate_packet_loss<<" packets per second");












flowMonitor->SerializeToXmlFile("report1.xml", true, true);
  flowMonitor2->SerializeToXmlFile("report2.xml", true, true);

  Simulator::Destroy ();

  
  double averageThroughput = ((sink->GetTotalRx() * 8) / (1e6  * simulationTime));
  std::cout << "\nAverage throughtput: " << averageThroughput << " Mbit/s" << std::endl;



double to_div=1000000.0;
  out<<averageThroughput<<",";
  out<<sum_txpackets<<",";
  out<<sum_rxpackets<<",";
  out<<percentage_loss<<",";
  out<<rate_packet_loss<<",";
  out<<mean_delay/to_div<<",";
  out<<deviation_delay/to_div<<",";
  out<<mean_jitters/to_div<<",";
  out<<deviation_jitters/to_div<<",";
  out<<"\n";












  percentage+=0.05;
}
  return 0;
}
