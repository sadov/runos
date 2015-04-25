#pragma once

#include "Application.hh"
#include "Loader.hh"
#include "Common.hh"
#include "OFMessageHandler.hh"

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


    OFMessageHandler::Action BGPprocess(OFConnection* ofconn, of13::FeaturesReply fr);
private:
    class Handler: public OFMessageHandler {
    public:
        Action processMiss(OFConnection* ofconn, Flow* flow) override;
    };

    ARPservice ARPhandler;
};


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
    void addRecord(EthAddress & mac, IPAddress & ip)
    { ARPtable.push_back(Record(mac, ip)); }

    EthAddress * find(IPAddress & ip);
    IPAddress * find(EthAddress & mac);
    bool find(EthAddress & mac, IPAddress & ip);
    OFMessageHandler::Action process(OFConnection* ofconn, Flow* flow);  
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