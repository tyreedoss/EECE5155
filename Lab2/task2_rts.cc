#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/yans-wifi-helper.h"

using namespace ns3;

int main(int argc, char *argv[]) {
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
    
    // Enable RTS/CTS Threshold to 500
    Config::SetDefault("ns3::WifiRemoteStationManager::RtsCtsThreshold", UintegerValue(500));

    // Create 5 WiFi station nodes and 1 access point (AP) node
    NodeContainer wifiStaNodes;
    wifiStaNodes.Create(nWifi);
    NodeContainer wifiApNode;
    wifiApNode.Create(1);

    // Configure the WiFi channel and physical layer
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());

    // Configure MAC and WiFi settings
    WifiMacHelper mac;
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211ac);

    Ssid ssid = Ssid("EECE5155");

    // Configure station nodes to use infrastructure mode (AP-managed)
    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid), "ActiveProbing", BooleanValue(false));
    NetDeviceContainer staDevices = wifi.Install(phy, mac, wifiStaNodes);

    // Configure AP node
    mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));
    NetDeviceContainer apDevice = wifi.Install(phy, mac, wifiApNode);

    // Set mobility models
    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    positionAlloc->Add(Vector(0.0, 0.0, 0.0)); // AP fixed at (0,0)
    mobility.SetPositionAllocator(positionAlloc);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(wifiApNode);

    // Set station nodes to move randomly within a defined rectangular area
    MobilityHelper mobility1;
    mobility1.SetPositionAllocator("ns3::GridPositionAllocator",
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
    mobility1.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
                              "Bounds", RectangleValue(Rectangle(-90, 90, -90, 90)));
    mobility1.Install(wifiStaNodes);

    // Install Internet stack on all nodes
    InternetStackHelper stack;
    stack.Install(wifiApNode);
    stack.Install(wifiStaNodes);

    // Assign IPv4 addresses
    Ipv4AddressHelper address;
    address.SetBase("192.168.2.0", "255.255.255.0");
    Ipv4InterfaceContainer staInterfaces = address.Assign(staDevices);
    Ipv4InterfaceContainer apInterface = address.Assign(apDevice);

    // Set up UDP Echo Server on Node 0
    UdpEchoServerHelper echoServer(20);
    ApplicationContainer serverApp = echoServer.Install(wifiStaNodes.Get(0));
    serverApp.Start(Seconds(1.0));
    serverApp.Stop(Seconds(10.0));

    // Configure UDP Echo Clients
    UdpEchoClientHelper echoClient(staInterfaces.GetAddress(0), 20);
    echoClient.SetAttribute("MaxPackets", UintegerValue(2));
    echoClient.SetAttribute("Interval", TimeValue(Seconds(2.0)));
    echoClient.SetAttribute("PacketSize", UintegerValue(512)); // set packet size

    // Client on Node 3 sends packets at 3s and 5s
    ApplicationContainer clientApp3 = echoClient.Install(wifiStaNodes.Get(3));
    clientApp3.Start(Seconds(3.0));
    clientApp3.Stop(Seconds(10.0));

    // Client on Node 4 sends packets at 2s and 5s
    echoClient.SetAttribute("Interval", TimeValue(Seconds(3.0)));
    ApplicationContainer clientApp4 = echoClient.Install(wifiStaNodes.Get(4));
    clientApp4.Start(Seconds(2.0));
    clientApp4.Stop(Seconds(10.0));

    // Enable IPv4 routing
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();
    Simulator::Stop(Seconds(10.0));
    
    // Enable tracing on Node 4 and the AP
    if (tracing)
    {
        phy.SetPcapDataLinkType(YansWifiPhyHelper::DLT_IEEE802_11_RADIO);
        phy.EnablePcap("wifi-ap", apDevice, true);
        phy.EnablePcap("wifi-node4", staDevices.Get(4), true);
    }
    
    // Run the simulation
    Simulator::Run();
    Simulator::Destroy();
    return 0;
}
