/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
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
 *
 * Authors: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

// This example is used to validate 802.11n MIMO.
//
// It outputs plots of the throughput versus the distance
// for every HT MCS value and from 1 to 4 MIMO streams.
//
// The simulation assumes a single station in an infrastructure network:
//
//  STA     AP
//    *     *
//    |     |
//   n1     n2
//
// The user can choose whether UDP or TCP should be used and can configure
// some 802.11n parameters (frequency, channel width and guard interval).

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"
#include "ns3/gnuplot.h"
#include <fstream>
#include <vector>
#include <cmath>

using namespace ns3;

int main (int argc, char *argv[])
{
  std::ofstream file ("80211n-mimo-throughput.plt");
  
  std::vector <std::string> modes;
  modes.push_back ("HtMcs0");
  // modes.push_back ("HtMcs1");
  // modes.push_back ("HtMcs2");
  // modes.push_back ("HtMcs3");
  // modes.push_back ("HtMcs4");
  // modes.push_back ("HtMcs5");
  modes.push_back ("HtMcs6");
  // modes.push_back ("HtMcs7");
  // modes.push_back ("HtMcs8");
  // modes.push_back ("HtMcs9");
  // modes.push_back ("HtMcs10");
  // modes.push_back ("HtMcs11");
  // modes.push_back ("HtMcs12");
  // modes.push_back ("HtMcs13");
  // modes.push_back ("HtMcs14");
  // modes.push_back ("HtMcs15");
  // modes.push_back ("HtMcs16");
  // modes.push_back ("HtMcs17");
  // modes.push_back ("HtMcs18");
  // modes.push_back ("HtMcs19");
  // modes.push_back ("HtMcs20");
  // modes.push_back ("HtMcs21");
  // modes.push_back ("HtMcs22");
  // modes.push_back ("HtMcs23");
  // modes.push_back ("HtMcs24");
  // modes.push_back ("HtMcs25");
  // modes.push_back ("HtMcs26");
  // modes.push_back ("HtMcs27");
  // modes.push_back ("HtMcs28");
  // modes.push_back ("HtMcs29");
  // modes.push_back ("HtMcs30");
  // modes.push_back ("HtMcs31");
  
  double simulationTime = 5; //seconds
  double frequency = 5.0; //whether 2.4 or 5.0 GHz
  double step = 5; //meters
  bool shortGuardInterval = false;
  bool channelBonding = false;

  CommandLine cmd;
  cmd.AddValue ("step", "Granularity of the results to be plotted in meters", step);
  cmd.AddValue ("channelBonding", "Enable/disable channel bonding (channel width = 20 MHz if false, channel width = 40 MHz if true)", channelBonding);
  cmd.AddValue ("shortGuardInterval", "Enable/disable short guard interval", shortGuardInterval);
  cmd.AddValue ("frequency", "Whether working in the 2.4 or 5.0 GHz band (other values gets rejected)", frequency);
  cmd.Parse (argc,argv);


  for (uint32_t i = 0; i < modes.size (); i++) //MCS
    {
      std::cout << modes[i] << std::endl;
      for (int d = 20; d <= 20; ) //distance
        {
          std::cout << "Distance = " << d << "m: "<< std::endl;
          uint32_t payloadSize = 1448; //bytes
          Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (payloadSize));
              
          uint8_t nStreams = 1 + (i / 8); //number of MIMO streams

          NodeContainer wifiStaNode;
          wifiStaNode.Create (8);
          NodeContainer wifiApNode;
          wifiApNode.Create (1);

          YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
          YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
          phy.SetChannel (channel.Create ());

          // Set guard interval
          phy.Set ("ShortGuardEnabled", BooleanValue (shortGuardInterval));
          // Set MIMO capabilities
          phy.Set ("TxAntennas", UintegerValue (nStreams));
          phy.Set ("RxAntennas", UintegerValue (nStreams));

          WifiMacHelper mac;
          WifiHelper wifi;
          if (frequency == 5.0)
            {
              wifi.SetStandard (WIFI_PHY_STANDARD_80211n_5GHZ);
            }
          else if (frequency == 2.4)
            {
              wifi.SetStandard (WIFI_PHY_STANDARD_80211n_2_4GHZ);
              Config::SetDefault ("ns3::LogDistancePropagationLossModel::ReferenceLoss", DoubleValue (40.046));
            }
          else
            {
              std::cout<<"Wrong frequency value!"<<std::endl;
              return 0;
            }

          wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager","DataMode", StringValue (modes[i]),
                                        "ControlMode", StringValue (modes[i]));
                
          Ssid ssid = Ssid ("ns3-80211n");

          mac.SetType ("ns3::StaWifiMac",
                       "Ssid", SsidValue (ssid));

          NetDeviceContainer staDevice;
          staDevice = wifi.Install (phy, mac, wifiStaNode);

          mac.SetType ("ns3::ApWifiMac",
                       "Ssid", SsidValue (ssid));

          NetDeviceContainer apDevice;
          apDevice = wifi.Install (phy, mac, wifiApNode);

          // Set channel width
          if (channelBonding)
            {
              Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/ChannelWidth", UintegerValue (40));
            }

          // mobility.
          MobilityHelper mobility;
          Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();

          positionAlloc->Add (Vector (0.0, 0.0, 0.0));
          positionAlloc->Add (Vector (d, 0.0, 0.0));
          mobility.SetPositionAllocator (positionAlloc);

          mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

          mobility.Install (wifiApNode);
          mobility.Install (wifiStaNode);

          /* Internet stack*/
          InternetStackHelper stack;
          stack.Install (wifiApNode);
          stack.Install (wifiStaNode);

          Ipv4AddressHelper address;

          address.SetBase ("192.168.1.0", "255.255.255.0");
          Ipv4InterfaceContainer staNodeInterface;
          Ipv4InterfaceContainer apNodeInterface;
          
          staNodeInterface = address.Assign (staDevice);
          apNodeInterface = address.Assign (apDevice);

        
          /* Install TCP Receiver on the access point */
          PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), 9));
          ApplicationContainer sinkApp = sinkHelper.Install (wifiStaNode);  
          // sink = StaticCast<PacketSink> (sinkApp.Get(0));

          /* Install TCP/UDP Transmitter on the station */  
          ApplicationContainer serverApp;
          // srand(time(NULL));
                  
          for(int sender=0;sender<8;sender++)
          {

            double valself=0.65/56.0;
            for(int rcv=0;rcv<8;rcv++)
            {                
                if(sender==rcv) continue;
                OnOffHelper server ("ns3::TcpSocketFactory", (InetSocketAddress (staNodeInterface.GetAddress (rcv), 9)));
                server.SetAttribute ("PacketSize", UintegerValue (payloadSize));
                server.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
                server.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
                server.SetAttribute ("DataRate", DataRateValue (DataRate (std::to_string(valself)+"Mbps")));
                serverApp.Add(server.Install(wifiStaNode.Get(sender)));    
            }            
          }


          sinkApp.Start (Seconds (0.0));
          sinkApp.Stop (Seconds (simulationTime + 1));
          serverApp.Start (Seconds (1.0));
          serverApp.Stop (Seconds (simulationTime + 1));


          Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
          
          Simulator::Stop (Seconds (simulationTime + 1));
          Simulator::Run ();
          Simulator::Destroy ();

          double throughput = 0;
          uint32_t totalPacketsThrough = 0;
          for(int ii = 0; ii<8; ii++){
            totalPacketsThrough  =  totalPacketsThrough +  DynamicCast<PacketSink> (sinkApp.Get (ii))->GetTotalRx ();
          }

          throughput = totalPacketsThrough * 8 / (simulationTime * 1000000.0); //Mbit/s
          std::cout << throughput << " Mbit/s" <<std::endl;
          d += step;
        }
    }
  return 0;
}

