
#include "MyApp.hh"
#include "Controller.hh"
#include <sys/types.h>
 #include <sys/socket.h>
 #include <netinet/in.h> 
//#include "OFMsgUnion.hh"
//#include <fluid/OFServer.hh>
//#include <fluid/of13msg.hh>





enum cgnat_action_type {
		CGNAT_ACTION_CREATE_IPV4,
		CGNAT_ACTION_DELETE_IPV4,
		CGNAT_ACTION_DONE,
		CGNAT_ACTION_NON,
};

struct cgnat_message {
	enum cgnat_action_type type;
	u_char prefix_len;
	uint32_t prefix;
	uint32_t gate;
};

int SocketHandler::init()
{
	struct sockaddr_in addr;
	// TODO: check exist old file
	// Create socket
	sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == -1)
	{
		std::cerr << "Error: can't create socket" << std::endl;
		return -1;
	}
	// Set socket settings
	memset(&addr, 0, sizeof (addr));
	addr.sin_family = PF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	// bind to socket
	if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1)
	{
	std::cerr << "Error: cann't bind" << std::endl;
		close(sock);
		return -1;
	}
	// TODO:  Check: if user quagga exist set socket permitions ????
	is_init = true;
	
/*
	struct sockaddr_un addr;
	// TODO: check exist old file
	// Create socket 
	sock = socket (AF_INET, SOCK_STREAM, 0);
	if (sock == -1)
	{
		std::cerr << "Error: can't create socket" << std::endl;
		return -1;
	}
	// Set socket settings 
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, CGNAT_SOCK_PATH, sizeof(addr.sun_path)-1);
    // bind to socket 
	if ((bind(sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_un))) == -1)
	{
		close(sock);
		return -1;
	}
*/
}
int SocketHandler::read()
{
	// check socket init
	if (!is_init)
	{
		std::cerr << "socket wasn\'t init" << std::endl;
		return -1;
	}
	// listen socket
	if (listen(sock, 1) != 0) 
	{
		std::cerr << "Listen socket failed" << std::endl;
		return -1;
	}
	// accept socket
	int tmpfd = accept(sock, NULL,NULL);
	if (tmpfd < 0)
	{
		std::cerr << "accept failed" << std::endl;
		return -1;
	}
	// read messages
	cgnat_message msg;
	while (::read(tmpfd, (void *)&msg, sizeof(struct cgnat_message)) > 0)
	{
		switch(msg.type)
		{
			case CGNAT_ACTION_CREATE_IPV4:
			{
				vRoute.push_back(Route(msg.prefix_len, msg.prefix, msg.gate));
				msg.type = CGNAT_ACTION_DONE;
				break;
			}
			case CGNAT_ACTION_DELETE_IPV4:
			{
				Route r(msg.prefix_len, msg.prefix, msg.gate);
				unsigned i = 0;
				for (; i < vRoute.size() && (vRoute[i] != r); ++i)
					;
				if (i < vRoute.size())
				{
					vRoute.erase(vRoute.begin() + i);
					msg.type = CGNAT_ACTION_DONE;
				}
				else
					msg.type = CGNAT_ACTION_NON;
				break;
			}
			//default:
			//	TODO ?????????????????????
			// maybe ERROR?
		}
		// send replay message
		if (send(tmpfd, (const void *)&msg, sizeof(struct cgnat_message), MSG_NOSIGNAL) != sizeof(struct cgnat_message))
		{
			std::cerr << "cann't send replay message" << std::endl;
			return -1;
		}
	}
}


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


OFMessageHandler::Action MyApp::BGPprocess(OFConnection* ofconn, Flow* flow)
{
	Packet * packet = flow->pkt();
	// if IP-packet and TCP-segment
	if (packet->readEthType() != 0x0800 || packet->readIPProto() != 6) 
		return OFMessageHandler::Continue;
	of13::FlowMod fm;
	// Creates in_port oxm_field with value=1
	of13::InPort *port = new of13::InPort(1);
	// Add in_port to Flow Mod
	fm.add_oxm_field(new of13::InPort(packet->readInPort()));
	// Create eth_type oxm_field with value=0x0800 (IPv4) and add to Flow Mod
	fm.add_oxm_field(new of13::EthType(0x0800));
	// Create ip_proto oxm_field with value=6 (TCP) and add to Flow Mod
	fm.add_oxm_field(new of13::IPProto(6));

	fm.add_oxm_field(new of13::IPv4Src(packet->readIPv4Src()));
	fm.add_oxm_field(new of13::IPv4Dst(packet->readIPv4Dst()));


	// Creates Output Action with in_port=1 and send_len=1024
	// of13::OutputAction act(1, 1024);
	of13::OutputAction act(2605, 1024); // port of BGP?

	// Creates ApplyAction and add action
	of13::ApplyActions inst; 
	inst.add_action(act);
	//Add the ApplyAction to FlowMod
	fm.add_instruction(inst);

	uint8_t *buff = fm.pack();
	ofconn->send(buff, fm.length());
	delete[] buff;
	return OFMessageHandler::Continue;
}
/*
unsigned ARP_SIZE = 2 + 2 + 1 + 1 + 2 + 6 + 4 + 6 + 4;

uint8_t * getARP(const uint16_t operation, EthAddress srcEth, IPAddress srcIP = IPAddress("0.0.0.0"), 
		EthAddress dstEth = EthAddress("ff:ff:ff:ff:ff:ff"), IPAddress dstIP = IPAddress("255.255.255.255"))
{
	uint8_t * data = new uint8_t[ARP_SIZE];
	unsigned j = 0;

	*reinterpret_cast<uint16_t*>(data + j) = 0x0001; j += 2; // HTYPE
	*reinterpret_cast<uint16_t*>(data + j) = 0x0800; j += 2; // PTYPE
	data[4] = uint8_t(6); ++j;// HLEN
	data[5] = uint8_t(4); ++j; // PLEN
	*reinterpret_cast<uint16_t*>(data + j) = operation; j += 2; // OPER
	// SHA
	for (unsigned i = 0; i < 6; ++i)
		data[j + i] = *srcEth.get_data();
	j += 6;
	*reinterpret_cast<uint32_t*>(data + j) = srcIP.getIPv4(); j += 4;// SPA
	// THA
	for (unsigned i = 0; i < 6; ++i)
		data[j + i] = *dstEth.get_data();
	*reinterpret_cast<uint32_t*>(data + j) = srcIP.getIPv4(); j += 4; // TPA
	return data;
}
*/

/*
EthAddress * ARPservice::find(const IPAddress & ip)
{
	for (unsigned i = 0; i < ARPtable.size(); ++i)
		if (ARPtable[i].ip == ip)
			return &ARPtable[i].mac;
	return nullptr;
}

IPAddress * ARPservice::find(const EthAddress & mac)
{
	for (unsigned i = 0; i < ARPtable.size(); ++i)
		if (ARPtable[i].mac == mac)
			return &ARPtable[i].ip;
	return nullptr;
}


bool ARPservice::find(const EthAddress & mac, const IPAddress & ip)
{
	for (unsigned i = 0; i < ARPtable.size(); ++i)
		if (ARPtable[i].mac == mac && ARPtable[i].ip == ip)
			return true;
	return false;
}


OFMessageHandler::Action ARPservice::process(OFConnection* ofconn, Flow* flow)
{
	if (flow->pkt()->readEthType() != 0x0806)
		return OFMessageHandler::Continue;
	Packet * packet = flow->pkt();
	uint16_t op = packet->readARPOp();
	switch (op)
	{
	case 0: 
		return OFMessageHandler::Stop;
	case 1:
		{
		if (!find(packet->readARPSHA(), packet->readARPSPA()))
			addRecord(packet->readARPSHA(), packet->readARPSPA());
		EthAddress * addr = find(packet->readARPTPA());
		if (!addr)
			return OFMessageHandler::Stop;
		// TODO: sending ARP-reply
		return OFMessageHandler::Continue;
		}
	case 2:
		if (find(packet->readARPSHA(), packet->readARPSPA()))
			addRecord(packet->readARPSHA(), packet->readARPSPA());
		if (find(packet->readARPTHA(), packet->readARPTPA()))
			addRecord(packet->readARPTHA(), packet->readARPTPA());
		return OFMessageHandler::Continue;
	default:
		// TODO: ERROR ?
		return OFMessageHandler::Continue;
	}

}
*/

OFMessageHandler::Action MyApp::Handler::processMiss(OFConnection* ofconn, Flow* flow)
{
	//if (ARPhandler.process(ofconn, flow) == Stop)
	//	return Stop;
	//if ()
	return Continue;

	/*
	int EthType = flow->pkt()->readEthType();
	IPAddress srcIP, dstIP;
	EthAddress srcEth, dstEth;
	if (EthType == 0x0800)
	{
		srcIP = flow->pkt()->readIPv4Src();
		dstIP = flow->pkt()->readIPv4Dst();
	}
	srcEth = flow->pkt()->readEthSrc();
	dstEth = flow->pkt()->readEthDst();
	std::cout << "-----" << " MESSAGE " << "-----" << std:: endl;
	printf("type = %x\n", EthType);
	std::cout << srcEth << " ---> " << dstEth << std::endl;
	std::cout << srcIP << " ---> " << dstIP << std::endl;
	std::cout << "---" << " MESSAGE END " << "---" << std:: endl;

	uint8_t * data = getARP(1, EthAddress("01:80:c2:00:00:0e"));
	ofconn->send(data, ARP_SIZE);
	delete[] data;

	return Continue;
*/

	//std::cout << EthAddress("00:11:22:33:44:55") << std::endl;
	//std::cout << IPAddress("192.168.1.2") << std::endl;
	//std::cout << "IPSrc = " << flow->pkt()->readIPv4Src() << std::endl;
	//std::cout << "IPDst = " << flow->pkt()->readIPv4Dst() << std::endl;
	//std::cout << "port = " << flow->pkt()->readInPort() << std::endl;

	/*
	of13::FlowMod fm;
	// Creates in_port oxm_field with value=1
	//of13::InPort *port = new of13::InPort(1);
	// Add in_port to Flow Mod
	//fm.add_oxm_field(port);
	// Create eth_type oxm_field with value=0x0800 and add to Flow Mod
	fm.add_oxm_field(new of13::EthType(0x0800));
	// add match IPsrc = 192.168.0.0
	// add match IPdst = 255.255.255.255

	IPAddress mask("255.255.255.0");
	IPAddress ipSrc("192.168.2.0");
	fm.add_oxm_field(new of13::IPv4Src(ipSrc, mask));
	IPAddress ipDst("192.168.1.0");
	fm.add_oxm_field(new of13::IPv4Dst(ipDst, mask));

	// Create eth_type oxm_field with value=0x0800 and add to Flow Mod
	//fm.add_oxm_field(new of13::EthType(0x0800));


	//fm.add_oxm_field(new of13::IPv4Dst(ip));
	// Creates Output Action with in_port=1 and send_len=1024
	of13::OutputAction act(1, 1024);

	// Creates ApplyAction and add action
	of13::ApplyActions inst; 
	inst.add_action(act);
	//Add the ApplyAction to FlowMod
	fm.add_instruction(inst);

	uint8_t *buff = fm.pack();



	//uint8_t *buff;
	//std::cout << "length = " << of13::ARPTHA( EthAddress("00:00:00:00:00:01") ).pack(buff) << std::endl;
	
	//ofconn->send(buff, fm.length());



	std::cout << "-----" << " MESSAGE " << "-----" << std:: endl;
	std::cout << "size = " << fm.length() << std::endl;
	of13::Match m = fm.match();
	std::cout << "src = " << m.ipv4_src()->value() << " / " << m.ipv4_src()->mask() << std::endl;
	std::cout << "dst = " << m.ipv4_dst()->value() << " / " << m.ipv4_dst()->mask() << std::endl;
	std::cout << "-----" << " ! END ! " << "-----" << std::endl;
	delete[] buff;

	return Continue;
	*/

	/*
    if (flow != nullptr && flow->match(of13::EthSrc("00:00:00:00:00:00")))
    {
    	std::cout << "catch and drop" << std::endl;
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
