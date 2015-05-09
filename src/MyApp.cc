
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

SocketHandler zebra(CGNAT_SOCK_PATH);


/* Signals catcher */
void catcher(int sig)
{
	if ((sig == SIGINT) || (sig == SIGTERM ) || (sig == SIGKILL))
	{
		// close openning sockets
		std::cout << "close" << std::endl;
		zebra.close();
		exit(1);
	}
	else if (sig == SIGALRM)
	{
		// check vector of Routes
		std::cout << "===== vRoute =====" << std::endl;
		zebra.printRoutes();
		std::cout << "=== End vRoute ===" << std::endl;
		alarm(5);
	}
}

	std::ostream & operator<<(std::ostream & out, const Route & r)
	{
		out << r.prefix % 256;
		uint32_t prefix = r.prefix;
		for (int i = 1; i < 4; ++i)
		{
			prefix /= 256;
			out << '.' << prefix % 256;
		}
		return out << '/' << static_cast<int>(r.prefix_len) << " ---> " << r.gate;
	}


void * routeFromSocket(void * arg)
{
	std::cout << "\"void * routeFromSocket(void * arg)\" was started"  << std::endl;
	SocketHandler * socketHandler = reinterpret_cast<SocketHandler *>(arg);
	std::cout << "status socketHandler->init() == " << socketHandler->init() << std::endl;
	while (true)
		std::cout << "read_status == " << socketHandler->read() << std::endl;
	return nullptr;
}

int init_takingMessages()
{
	signal(SIGINT, catcher);
	signal(SIGTERM, catcher);
	signal(SIGKILL, catcher);
	signal (SIGALRM, catcher);
	alarm(10);

	pthread_t threadZebra;
	int error;
	// int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine)(void*), void *arg);
	error = pthread_create(&threadZebra, NULL, routeFromSocket, &zebra);
	return error;
}

// class SocketHandler 
int SocketHandler::init()
{
/*
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
*/

	struct sockaddr_un addr;
	struct stat buf;
	// TODO: check exist old file
	if (stat(CGNAT_SOCK_PATH, &buf) != -1) {
		std::cerr << "Old socket file exists\n";
		if (remove(CGNAT_SOCK_PATH) == -1)
			std::cerr << "Delete old socket file error" << std::endl;
	}
	// Create socket 
	sock = socket (AF_UNIX, SOCK_STREAM, 0);
	if (sock == -1)
	{
		std::cerr << "Error: can't create socket" << std::endl;
		return -1;
	}
	// Set socket settings 
	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, CGNAT_SOCK_PATH, sizeof(addr.sun_path)-1);
	// bind to socket 
	if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1)
	{
		std::cerr << "Error: can't bind socket (" << errno << ")" << std::endl;
		//std::cerr << perror("ошибка") << std::endl;
		perror("ошибка");
		close();
		return -1;
	}
	is_init = true;
}
int SocketHandler::read()
{
	std::cout << "begin read" << std::endl;
	// check socket init
	if (!is_init)
	{
		std::cerr << "socket wasn\'t init" << std::endl;
		return -1;
	}
	std::cout << "begin listen" << std::endl;
	// listen socket
	if (listen(sock, 1) != 0) 
	{
		std::cerr << "Listen socket failed" << std::endl;
		return -1;
	}
	std::cout << "begin accept" << std::endl;
	// accept socket
	sock_fd = accept(sock, NULL,NULL);
	if (sock_fd < 0)
	{
		std::cerr << "accept failed" << std::endl;
		return -1;
	}
	std::cout << "begin receive data" << std::endl;
	// read
	cgnat_message msg;
	while (::read(sock_fd, (void *)&msg, sizeof(struct cgnat_message)) > 0)
	{
		switch(msg.type)
		{
			case CGNAT_ACTION_CREATE_IPV4:
			{
				vRoute.push_back(Route(msg.prefix_len, msg.prefix, msg.gate));
				msg.type = CGNAT_ACTION_DONE;
				std::cout << "CGNAT_ACTION_CREATE_IPV4" << std::endl;
				std::cout << msg.prefix_len << " : " << msg.prefix << " : " << msg.gate << '\n';
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
				std::cout << "CGNAT_ACTION_DELETE_IPV4" << std::endl;
				break;
			}
			//default:
			//	TODO ?????????????????????
			// maybe ERROR?
		}
		// send replay message
		if (send(sock_fd, (const void *)&msg, sizeof(struct cgnat_message), MSG_NOSIGNAL) != sizeof(struct cgnat_message))
		{
			std::cerr << "cann't send replay message" << std::endl;
			return -1;
		}
	}
	return 0;
}

void SocketHandler::close()
{
	if (sock) 
		::close(sock);
	if (sock_fd)
		::close(sock_fd);
	remove(sockPath);
}
// end of class SocketHandler


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
	// init of receiving messages
	std::cout << "status init_takingMessages() == " << init_takingMessages() << std::endl;
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
