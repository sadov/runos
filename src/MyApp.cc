
#include "MyApp.hh"
#include "Controller.hh"
//#include "OFMsgUnion.hh"
//#include <fluid/OFServer.hh>
//#include <fluid/of13msg.hh>


std::ostream & operator<< (std::ostream & out, const EthAddress & address)
{
	out << address.to_string();
	return out;
}

std::ostream & operator<< (std::ostream & out, const IPAddress & address)
{
	IPAddress addressV4 = address;
	uint32_t number = addressV4.getIPv4();
	out << number % 256;
	for (unsigned i = 1; i < 4; ++i)
		out << '.' << ((number >> (i * 8)) % 256);
	return out;
}

 REGISTER_APPLICATION(MyApp, {"controller", ""})

void MyApp::onSwitchUp(OFConnection* ofconn, of13::FeaturesReply fr)
{
    LOG(INFO) << "Look! This is a switch " << FORMAT_DPID;// << fr.xid();
    LOG(INFO) << "Hello, world!";
}

void MyApp::init(Loader *loader, const Config& config)
{
	//std::cout << "init" << std::endl;
    Controller* ctrl = Controller::get(loader);
    QObject::connect(ctrl, &Controller::switchUp, this, &MyApp::onSwitchUp);
    ctrl->registerHandler(this);
}


OFMessageHandler::Action MyApp::Handler::processMiss(OFConnection* ofconn, Flow* flow)
{
	//std::cout << EthAddress("00:11:22:33:44:55") << std::endl;
	//std::cout << IPAddress("192.168.1.2") << std::endl;
	//std::cout << "IPSrc = " << flow->pkt()->readIPv4Src() << std::endl;
	//std::cout << "IPDst = " << flow->pkt()->readIPv4Dst() << std::endl;
	//std::cout << "port = " << flow->pkt()->readInPort() << std::endl;

	
	of13::FlowMod fm;
	// Creates in_port oxm_field with value=1
	//of13::InPort *port = new of13::InPort(1);
	// Add in_port to Flow Mod
	//fm.add_oxm_field(port);
	// Create eth_type oxm_field with value=0x0800 and add to Flow Mod
	//fm.add_oxm_field(new of13::EthType(0x0800));
	// add match IPsrc = 192.168.0.0
	// add match IPdst = 255.255.255.255

	IPAddress mask("255.255.255.0");
	IPAddress ipSrc("192.168.2.0");
	fm.add_oxm_field(new of13::IPv4Src(ipSrc, mask));
	IPAddress ipDst("192.168.1.0");
	fm.add_oxm_field(new of13::IPv4Dst(ipDst, mask));

	// Create eth_type oxm_field with value=0x0800 and add to Flow Mod
	fm.add_oxm_field(new of13::EthType(0x0800));


	//fm.add_oxm_field(new of13::IPv4Dst(ip));
	// Creates Output Action with in_port=1 and send_len=1024
	of13::OutputAction act(1, 1024);

	// Creates ApplyAction and add action
	of13::ApplyActions inst; 
	inst.add_action(act);
	//Add the ApplyAction to FlowMod
	fm.add_instruction(inst);

	uint8_t *buff = fm.pack();
	
	ofconn->send(buff, fm.length());
	std::cout << "-----" << " MESSAGE " << "-----" << std:: endl;
	std::cout << "size = " << fm.length() << std::endl;
	of13::Match m = fm.match();
	std::cout << "src = " << m.ipv4_src()->value() << " / " << m.ipv4_src()->mask() << std::endl;
	std::cout << "dst = " << m.ipv4_dst()->value() << " / " << m.ipv4_dst()->mask() << std::endl;
	std::cout << "-----" << " ! END ! " << "-----" << std::endl;
	delete[] buff;

	return Continue;
	/*
    if (flow->match(of13::EthSrc("00:11:22:33:44:55")))
    {
    	return Stop;
    } 
    else 
    	return Continue;
    */
}


/*
#include "MyApp.hh"

REGISTER_APPLICATION(MyApp, {""})

void MyApp::init(Loader *loader, const Config& config)
{
    LOG(INFO) << "Hello, world!";
    //std::cout << "lalala" << std::endl;
}*/