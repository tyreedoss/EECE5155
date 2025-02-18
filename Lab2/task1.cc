#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ssid.h"
#include "ns3/yans-wifi-helper.h"

// Default Network Topology
//
//   Wifi 10.1.3.0
//                 AP
//  *    *    *    *
//  |    |    |    |    10.1.1.0
// n5   n6   n7   n0 -------------- n1   n2   n3   n4
//                   point-to-point  |    |    |    |
//                                   ================
//                                     LAN 10.1.2.0

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("TaskOne");

int
main(int argc, char* argv[])
{
    // Removed nCsma variable, changed nWifi to 5
    bool verbose = true;
    uint32_t nWifi = 5;
    bool tracing = false;

    CommandLine cmd(__FILE__);
    cmd.AddValue("nWifi", "Number of wifi STA devices", nWifi);
    cmd.AddValue("verbose", "Tell echo applications to log if true", verbose);
    cmd.AddValue("tracing", "Enable pcap tracing", tracing);

    cmd.Parse(argc, argv);

    // The underlying restriction of 18 is due to the grid position
    // allocator's configuration; the grid layout will exceed the
    // bounding box if more than 18 nodes are provided.
    if (nWifi > 18)
    {
        std::cout << "nWifi should be 18 or less; otherwise grid layout exceeds the bounding box"
                  << std::endl;
        return 1;
    }

    if (verbose)
    {
        LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
        LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);
    }

    // Got rid of Csma nodes and p2p nodes, and added wifi nodes
    NodeContainer wifiNodes;
    wifiNodes.Create(nWifi);

    // setup wifi channel and physial layer
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());

    // changed type to AdhocWifiMac
    WifiMacHelper mac;
    mac.SetType("ns3::AdhocWifiMac");

    // changed standard to IEEE 802.11ac
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211ac);

    // setup wifi devices
    NetDeviceContainer devices = wifi.Install(phy, mac, wifiNodes);

    // changed mobility model to specified bounds
    MobilityHelper mobility;
    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                "MinX",
                                DoubleValue(0.0),
                                "MinY",
                                DoubleValue(0.0),
                                "DeltaX",
                                DoubleValue(5.0),
                                "DeltaY",
                                DoubleValue(10.0),
                                "GridWidth",
                                UintegerValue(3),
                                "LayoutType",
                                StringValue("RowFirst"));                            
    mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
                              "Bounds", RectangleValue(Rectangle(-90, 90, -90, 90)));
    mobility.Install(wifiNodes);

    // install internet stack
    InternetStackHelper stack;
    stack.Install(wifiNodes);

    // assign new IP addresses
    Ipv4AddressHelper address;
    address.SetBase("192.168.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign(devices);

    // udp echo server at node 0
    UdpEchoServerHelper echoServer(20);
    ApplicationContainer serverApp = echoServer.Install(wifiNodes.Get(0));
    // start at 0.5
    serverApp.Start(Seconds(0.5));
    serverApp.Stop(Seconds(10.0));

    // UDP Echo Client on Node 3 sends at 1s and 2s
    UdpEchoClientHelper echoClient1(interfaces.GetAddress(0), 20);
    echoClient1.SetAttribute("MaxPackets", UintegerValue(2));
    echoClient1.SetAttribute("Interval", TimeValue(Seconds(1.0)));
    // changed packet size to 512
    echoClient1.SetAttribute("PacketSize", UintegerValue(512));
    ApplicationContainer clientApp1 = echoClient1.Install(wifiNodes.Get(3));
    clientApp1.Start(Seconds(1.0));
    clientApp1.Stop(Seconds(10.0));

    // UDP Echo Client on Node 4 (sends at 2s and 4s)
    UdpEchoClientHelper echoClient2(interfaces.GetAddress(0), 20);
    echoClient2.SetAttribute("MaxPackets", UintegerValue(2));
    echoClient2.SetAttribute("Interval", TimeValue(Seconds(2.0)));
    echoClient2.SetAttribute("PacketSize", UintegerValue(512));
    ApplicationContainer clientApp2 = echoClient2.Install(wifiNodes.Get(4));
    clientApp2.Start(Seconds(2.0));
    clientApp2.Stop(Seconds(10.0));

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();
    
    Simulator::Stop(Seconds(10.0));

    if (tracing)
    {
        phy.SetPcapDataLinkType(YansWifiPhyHelper::DLT_IEEE802_11_RADIO);
        phy.EnablePcap("adhoc-wifi-node1",devices.Get(1),true);
    }
    Simulator::Run();
    Simulator::Destroy();
    return 0;
}
