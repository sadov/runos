#pragma once

#include "Application.hh"
#include "Loader.hh"
#include "Common.hh"
#include "OFMessageHandler.hh"


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