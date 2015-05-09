#pragma once

#include "Application.hh"
#include "Loader.hh"
#include "Common.hh"
#include "OFMessageHandler.hh"

#include <sys/un.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <pthread.h>
#include <pwd.h>
#include <signal.h>
#include <iostream>

//#include "common.h"
//#include "route.h"
//#include "route_zebra.h"
//#include "zebra_cgnat.h"

#define QUAGGA "quagga"
#define CGNAT_SOCK_PATH "/var/run/cgnat.sock"

struct Route
{
	u_char prefix_len;
	uint32_t prefix;
	uint32_t gate;
	
	Route(const u_char prefix_length, const uint32_t pref, const uint32_t postfix) : prefix_len(prefix_length), prefix(pref), gate(postfix)
	{}
	bool operator==(const Route & r)
	{ return prefix_len == r.prefix_len && prefix == r.prefix && gate == r.gate; }
	bool operator!=(const Route & r)
	{ return !(*this == r); }
};

	std::ostream & operator<<(std::ostream & out, const Route & r);

/*
class ARPservice
{
	struct Record
	{
		EthAddress mac;
		IPAddress ip;

		Record(const EthAddress & Ethadd, const IPAddress & IPadd) : mac(Ethadd), ip(IPadd) {}
	};
	std::vector< Record > ARPtable;
public:
	void addRecord(const EthAddress & mac, const IPAddress & ip)
	{ ARPtable.push_back(Record(mac, ip)); }

	EthAddress * find(const IPAddress & ip);
	IPAddress * find(const EthAddress & mac);
	bool find(const EthAddress & mac, const IPAddress & ip);
	OFMessageHandler::Action process(OFConnection* ofconn, Flow* flow);  
};
*/

class SocketHandler
{
	bool is_init;
	const char * sockPath;
	int sock; // socket
	int sock_fd; // socket from accept
	int8_t * data;
	int dataSize;
	std::vector<Route> vRoute;
public:
	SocketHandler(const char * path) : is_init(false), sockPath(path), sock(-1), sock_fd(-1), data(nullptr), dataSize(0)
	{}
	const std::vector<Route> & getRoutes() { return vRoute; }
	int init();
	//bool isinit() { return is_init; }
	int read();
	void close();
	void printRoutes() 
	{ for (unsigned i = 0; i < vRoute.size(); ++i) std::cout << vRoute[i] << std::endl; }
	~SocketHandler() {}
	friend void catcher(int sig);
};


class MyApp : public Application, public OFMessageHandlerFactory
{
	SIMPLE_APPLICATION(MyApp, "myapp")
public:
	void init(Loader *loader, const Config& config);
	std::string orderingName() const override { return "mac-filtering"; }
	bool isPostreq(const std::string &name) const override { return (name == "forwarding"); }
	OFMessageHandler* makeOFMessageHandler() override { new Handler(); }
public slots:
	void onSwitchUp(OFConnection* ofconn, of13::FeaturesReply fr);


	OFMessageHandler::Action BGPprocess(OFConnection* ofconn, Flow* flow);
private:
	class Handler: public OFMessageHandler {
	public:
		Action processMiss(OFConnection* ofconn, Flow* flow) override;
	};
	
	//ARPservice ARPhandler;
};

/*
#include "Application.hh"
#include "Loader.hh"
#include "Common.hh"

class MyApp : public Application {
SIMPLE_APPLICATION(MyApp, "myapp")
public:
	void init(Loader* loader, const Config& config) override;
};
*/
