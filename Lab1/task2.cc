#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("SecondScriptExample");

int
main(int argc, char* argv[])
{
    bool verbose = true;
    uint32_t nCsma = 3;

    CommandLine cmd(__FILE__);
    cmd.AddValue("nCsma", "Number of \"extra\" CSMA nodes/devices", nCsma);
    cmd.AddValue("verbose", "Tell echo applications to log if true", verbose);

    cmd.Parse(argc, argv);

    if (verbose)
    {
        LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
        LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);
    }

    nCsma = nCsma == 0 ? 1 : nCsma;

    // first bus
    NodeContainer csmaBusA;
    csmaBusA.Create(nCsma);

    // second bus
    NodeContainer csmaBusB;
    csmaBusB.Create(nCsma);

    // point-to-point nodes
    NodeContainer p2pNodes;
    p2pNodes.Add(csmaBusA.Get(2));
    p2pNodes.Add(csmaBusB.Get(0));

    // Bus 1 setup
    CsmaHelper csmaA;
    csmaA.SetChannelAttribute("DataRate", StringValue("100Mbps"));
    csmaA.SetChannelAttribute("Delay", TimeValue(MicroSeconds(10)));
    NetDeviceContainer csmaADevices;
    csmaADevices = csmaA.Install(csmaBusA);

    // Bus 2 setup
    CsmaHelper csmaB;
    csmaB.SetChannelAttribute("DataRate", StringValue("100Mbps"));
    csmaB.SetChannelAttribute("Delay", TimeValue(MicroSeconds(10)));
    NetDeviceContainer csmaBDevices;
    csmaBDevices = csmaB.Install(csmaBusB);

    // Point-to-point setup
    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("2ms"));
    NetDeviceContainer p2pDevices;
    p2pDevices = pointToPoint.Install(p2pNodes);

    // Install internet stack
    InternetStackHelper stack;
    stack.Install(csmaBusA);
    stack.Install(csmaBusB);

    // assign IP for bus 1
    Ipv4AddressHelper address;
    address.SetBase("192.168.1.0", "255.255.255.0");
    Ipv4InterfaceContainer csmaAInterfaces;
    csmaAInterfaces = address.Assign(csmaADevices);

    // assign IP for bus 2
    address.SetBase("192.168.2.0", "255.255.255.0");
    Ipv4InterfaceContainer csmaBInterfaces;
    csmaBInterfaces = address.Assign(csmaBDevices);

    // assign IP for point-to-point
    address.SetBase("192.168.3.0", "255.255.255.0");
    Ipv4InterfaceContainer p2pInterfaces;
    p2pInterfaces = address.Assign(p2pDevices);

    // Set up UDP Echo Server on Node 1
    UdpEchoServerHelper echoServer(21);
    ApplicationContainer serverApps = echoServer.Install(csmaBusA.Get(1));
    serverApps.Start(Seconds(1.0));
    serverApps.Stop(Seconds(10.0));

    // Set up UDP Echo Client on Node 5
    UdpEchoClientHelper echoClient(csmaAInterfaces.GetAddress(1), 21);
    echoClient.SetAttribute("MaxPackets", UintegerValue(2));
    echoClient.SetAttribute("Interval", TimeValue(Seconds(3.0)));
    echoClient.SetAttribute("PacketSize", UintegerValue(1024));
    ApplicationContainer clientApps = echoClient.Install(csmaBusB.Get(2));
    clientApps.Start(Seconds(4.0));
    clientApps.Stop(Seconds(10.0));

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // packet tracing for node 2 and 4
    pointToPoint.EnablePcap("p2p_node2", p2pDevices.Get(0), true);
    csmaB.EnablePcap("csma_node4", csmaBDevices.Get(1), true);

    Simulator::Run();
    Simulator::Destroy();
    return 0;
}
