// $Id: SMPSWebPageHelper.cc,v 1.1.2.5 2011/02/09 11:47:04 mommsen Exp $
/// @file: SMPSWebPageHelper.cc

#include "EventFilter/SMProxyServer/interface/SMPSWebPageHelper.h"
#include "EventFilter/StorageManager/interface/XHTMLMonitor.h"
#include "EventFilter/StorageManager/src/ConsumerWebPageHelper.icc"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Utilities/interface/EDMException.h"


namespace smproxy
{
  SMPSWebPageHelper::SMPSWebPageHelper
  (
    xdaq::ApplicationDescriptor* appDesc,
    StateMachinePtr stateMachine
  ) :
  stor::WebPageHelper<SMPSWebPageHelper>(appDesc, "$Name:  $", this, &smproxy::SMPSWebPageHelper::addDOMforHyperLinks),
  _stateMachine(stateMachine),
  _consumerWebPageHelper(appDesc, "$Name:  $", this, &smproxy::SMPSWebPageHelper::addDOMforHyperLinks)
  { }
  
  
  void SMPSWebPageHelper::defaultWebPage(xgi::Output* out)
  {
    stor::XHTMLMonitor theMonitor;
    stor::XHTMLMaker maker;

    stor::XHTMLMaker::Node* body = createWebPageBody(maker,
      "Main",
      _stateMachine->getExternallyVisibleStateName(),
      _stateMachine->getStateName(),
      _stateMachine->getReasonForFailed()
    );
    
    addDOMforHyperLinks(maker, body);
    
    // Dump the webpage to the output stream
    maker.out(*out);
  }
  
  
  void SMPSWebPageHelper::dataRetrieverWebPage(xgi::Output* out)
  {
    stor::XHTMLMonitor theMonitor;
    stor::XHTMLMaker maker;

    stor::XHTMLMaker::Node* body = createWebPageBody(maker,
      "Data Retrieval",
      _stateMachine->getExternallyVisibleStateName(),
      _stateMachine->getStateName(),
      _stateMachine->getReasonForFailed()
    );

    addDOMforEventServers(maker, body);
    
    maker.addNode("hr", body);
    
    addDOMforDQMEventServers(maker, body);
    
    addDOMforHyperLinks(maker, body);
    
    // Dump the webpage to the output stream
    maker.out(*out);    
  } 
  
  
  void SMPSWebPageHelper::consumerStatisticsWebPage(xgi::Output* out)
  {
    _consumerWebPageHelper.consumerStatistics(out,
      _stateMachine->getExternallyVisibleStateName(),
      _stateMachine->getStateName(),
      _stateMachine->getReasonForFailed(),
      _stateMachine->getStatisticsReporter(),
      _stateMachine->getRegistrationCollection(),
      _stateMachine->getEventQueueCollection(),
      _stateMachine->getDQMEventQueueCollection()
    );
  }
  
  
  void SMPSWebPageHelper::dqmEventStatisticsWebPage(xgi::Output* out)
  {
    stor::XHTMLMonitor theMonitor;
    stor::XHTMLMaker maker;

    stor::XHTMLMaker::Node* body = createWebPageBody(maker,
      "DQM Event Processor",
      _stateMachine->getExternallyVisibleStateName(),
      _stateMachine->getStateName(),
      _stateMachine->getReasonForFailed()
    );
    
    addDOMforHyperLinks(maker, body);
    
    // Dump the webpage to the output stream
    maker.out(*out);    
  } 
  
  
  void SMPSWebPageHelper::addDOMforHyperLinks
  (
    stor::XHTMLMaker& maker,
    stor::XHTMLMaker::Node *parent
  )
  {
    std::string url = _appDescriptor->getContextDescriptor()->getURL()
      + "/" + _appDescriptor->getURN();
    
    stor::XHTMLMaker::AttrMap linkAttr;
    stor::XHTMLMaker::Node *link;
    
    maker.addNode("hr", parent);
    
    linkAttr[ "href" ] = url;
    link = maker.addNode("a", parent, linkAttr);
    maker.addText(link, "Main web page");
    
    maker.addNode("hr", parent);
    
    linkAttr[ "href" ] = url + "/dataRetriever";
    link = maker.addNode("a", parent, linkAttr);
    maker.addText(link, "Data retriever web page");
    
    maker.addNode("hr", parent);
    
    linkAttr[ "href" ] = url + "/dqmEventStatistics";
    link = maker.addNode("a", parent, linkAttr);
    maker.addText(link, "DQM event processor statistics");
    
    maker.addNode("hr", parent);
    
    linkAttr[ "href" ] = url + "/consumerStatistics";
    link = maker.addNode("a", parent, linkAttr);
    maker.addText(link, "Consumer Statistics");
    
    maker.addNode("hr", parent);
  }
  
  
  void SMPSWebPageHelper::addDOMforEventServers
  (
    stor::XHTMLMaker& maker,
    stor::XHTMLMaker::Node* parent
  )
  {
    stor::XHTMLMaker::AttrMap colspanAttr;
    colspanAttr[ "colspan" ] = "15";
    
    stor::XHTMLMaker::Node* table = maker.addNode("table", parent, _tableAttr);
    
    stor::XHTMLMaker::Node* tableRow = maker.addNode("tr", table, _rowAttr);
    stor::XHTMLMaker::Node* tableDiv = maker.addNode("th", tableRow, colspanAttr);
    maker.addText(tableDiv, "Event Servers");
    
    stor::XHTMLMaker::AttrMap rowspanAttr;
    rowspanAttr[ "rowspan" ] = "2";
    
    stor::XHTMLMaker::AttrMap subColspanAttr;
    subColspanAttr[ "colspan" ] = "2";
   
    // Header
    tableRow = maker.addNode("tr", table, _specialRowAttr);
    tableDiv = maker.addNode("th", tableRow, rowspanAttr);
    maker.addText(tableDiv, "Hostname");
    tableDiv = maker.addNode("th", tableRow, rowspanAttr);
    maker.addText(tableDiv, "Status");
    tableDiv = maker.addNode("th", tableRow, rowspanAttr);
    maker.addText(tableDiv, "HLT Output Module");
    tableDiv = maker.addNode("th", tableRow, rowspanAttr);
    maker.addText(tableDiv, "Filters");
    tableDiv = maker.addNode("th", tableRow, rowspanAttr);
    maker.addText(tableDiv, "Prescale");
    tableDiv = maker.addNode("th", tableRow, rowspanAttr);
    maker.addText(tableDiv, "Unique Events");
    tableDiv = maker.addNode("th", tableRow, rowspanAttr);
    maker.addText(tableDiv, "Enquing Policy");
    tableDiv = maker.addNode("th", tableRow, rowspanAttr);
    maker.addText(tableDiv, "Queue Size");
    tableDiv = maker.addNode("th", tableRow, rowspanAttr);
    maker.addText(tableDiv, "Max Request Rate (Hz)");
    tableDiv = maker.addNode("th", tableRow, subColspanAttr);
    maker.addText(tableDiv, "Event Rate (Hz)");
    tableDiv = maker.addNode("th", tableRow, subColspanAttr);
    maker.addText(tableDiv, "Average Event Size (kB)");
    tableDiv = maker.addNode("th", tableRow, subColspanAttr);
    maker.addText(tableDiv, "Bandwidth (kB/s)");

    tableRow = maker.addNode("tr", table, _specialRowAttr);
    tableDiv = maker.addNode("th", tableRow);
    maker.addText(tableDiv, "overall");
    tableDiv = maker.addNode("th", tableRow);
    maker.addText(tableDiv, "recent");
    tableDiv = maker.addNode("th", tableRow);
    maker.addText(tableDiv, "overall");
    tableDiv = maker.addNode("th", tableRow);
    maker.addText(tableDiv, "recent");
    tableDiv = maker.addNode("th", tableRow);
    maker.addText(tableDiv, "overall");
    tableDiv = maker.addNode("th", tableRow);
    maker.addText(tableDiv, "recent");

    DataRetrieverMonitorCollection::DataRetrieverStatList dataRetrieverStats;
    _stateMachine->getStatisticsReporter()->getDataRetrieverMonitorCollection()
      .getStatsByConnection(dataRetrieverStats);

    if ( dataRetrieverStats.empty() )
    {
      stor::XHTMLMaker::AttrMap messageAttr = colspanAttr;
      messageAttr[ "align" ] = "center";

      tableRow = maker.addNode("tr", table, _rowAttr);
      tableDiv = maker.addNode("td", tableRow, messageAttr);
      maker.addText(tableDiv, "Not registered to any event servers yet");
      return;
    }

    bool evenRow = false;

    for (DataRetrieverMonitorCollection::DataRetrieverStatList::const_iterator
           it = dataRetrieverStats.begin(), itEnd = dataRetrieverStats.end();
         it != itEnd; ++it)
    {
      stor::XHTMLMaker::AttrMap rowAttr = _rowAttr;
      if( evenRow )
      {
        rowAttr[ "style" ] = "background-color:#e0e0e0;";
        evenRow = false;
      }
      else
      {
        evenRow = true;
      }
      stor::XHTMLMaker::Node* tableRow = maker.addNode("tr", table, rowAttr);
      addRowForEventServer(maker, tableRow, *it);
    }
  }
  
  
  void SMPSWebPageHelper::addRowForEventServer
  (
    stor::XHTMLMaker& maker,
    stor::XHTMLMaker::Node* tableRow,
    DataRetrieverMonitorCollection::DataRetrieverStats const& stats
  )
  {
    // Hostname
    stor::XHTMLMaker::Node* tableDiv = maker.addNode("td", tableRow, _tableLabelAttr);
    const std::string sourceURL = stats.pset.getParameter<std::string>("sourceURL");
    std::string::size_type startPos = sourceURL.find("//");
    if ( startPos == std::string::npos )
      startPos = 0; 
    else
      startPos += 2;
    const std::string::size_type endPos = sourceURL.find('.');
    const std::string hostname = sourceURL.substr(startPos,(endPos-startPos));
    stor::XHTMLMaker::AttrMap linkAttr;
    linkAttr[ "href" ] = sourceURL + "/consumerStatistics";
    stor::XHTMLMaker::Node* link = maker.addNode("a", tableDiv, linkAttr);
    maker.addText(link, hostname);

    // Status
    if ( stats.connectionStatus == DataRetrieverMonitorCollection::CONNECTED )
    {
      tableDiv = maker.addNode("td", tableRow, _tableLabelAttr);
      maker.addText(tableDiv, "Connected");
    }
    else
    {
      stor::XHTMLMaker::AttrMap statusAttr = _tableLabelAttr;
      statusAttr[ "style" ] = "color:brown;";
      tableDiv = maker.addNode("td", tableRow, statusAttr);
      std::ostringstream status;
      status << stats.connectionStatus;
      maker.addText(tableDiv, status.str());
    }

    // HLT output module
    tableDiv = maker.addNode("td", tableRow, _tableLabelAttr);
    const std::string outputModule = stats.pset.getUntrackedParameter<std::string>("SelectHLTOutput");
    maker.addText(tableDiv, outputModule);

    // Filter list:
    std::string filters = stats.pset.getUntrackedParameter<std::string>("TriggerSelector");
    if ( filters.empty() )
    {
      const Strings fl = stats.pset.getParameter<Strings>("TrackedEventSelection");
      for( Strings::const_iterator
             lit = fl.begin(), litEnd = fl.end();
           lit != litEnd; ++lit )
      {
        if( lit != fl.begin() ) filters += "  ";
        filters += *lit;
      }
    }
    tableDiv = maker.addNode("td", tableRow, _tableLabelAttr);
    maker.addText(tableDiv, filters);
  
    // Prescale:
    tableDiv = maker.addNode("td", tableRow, _tableValueAttr);
    const int prescale = stats.pset.getUntrackedParameter<int>("prescale");
    maker.addInt(tableDiv, prescale);
    
    // Unique events
    tableDiv = maker.addNode("td", tableRow, _tableLabelAttr);
    const bool uniqueEvents = stats.pset.getUntrackedParameter<bool>("uniqueEvents");
    maker.addBool(tableDiv, uniqueEvents);
    
    // Policy
    tableDiv = maker.addNode("td", tableRow, _tableLabelAttr);
    const std::string policy = stats.pset.getUntrackedParameter<std::string>("queuePolicy");
    maker.addText(tableDiv, policy);
    
    // Queue size
    tableDiv = maker.addNode("td", tableRow, _tableValueAttr);
    const int queueSize = stats.pset.getUntrackedParameter<int>("queueSize");
    maker.addInt(tableDiv, queueSize);

    // Max request rate
    tableDiv = maker.addNode("td", tableRow, _tableValueAttr);
    try
    {
      const double rate = stats.pset.getUntrackedParameter<double>("maxEventRequestRate");
      maker.addDouble(tableDiv, rate, 1);
    }
    catch (edm::Exception& e)
    {
      maker.addText(tableDiv, "unlimited");
    }

    // Event rate
    tableDiv = maker.addNode("td", tableRow, _tableValueAttr);
    maker.addDouble(tableDiv, stats.sizeStats.getSampleRate(stor::MonitoredQuantity::FULL));
    tableDiv = maker.addNode("td", tableRow, _tableValueAttr);
    maker.addDouble(tableDiv, stats.sizeStats.getSampleRate(stor::MonitoredQuantity::RECENT));

    // Average event size
    tableDiv = maker.addNode("td", tableRow, _tableValueAttr);
    maker.addDouble(tableDiv, stats.sizeStats.getValueAverage(stor::MonitoredQuantity::FULL));
    tableDiv = maker.addNode("td", tableRow, _tableValueAttr);
    maker.addDouble(tableDiv, stats.sizeStats.getValueAverage(stor::MonitoredQuantity::RECENT));
    
    // Bandwidth
    tableDiv = maker.addNode("td", tableRow, _tableValueAttr);
    maker.addDouble(tableDiv, stats.sizeStats.getValueRate(stor::MonitoredQuantity::FULL));
    tableDiv = maker.addNode("td", tableRow, _tableValueAttr);
    maker.addDouble(tableDiv, stats.sizeStats.getValueRate(stor::MonitoredQuantity::RECENT));
  }

  
  void SMPSWebPageHelper::addDOMforDQMEventServers
  (
    stor::XHTMLMaker& maker,
    stor::XHTMLMaker::Node* parent
  )
  {
  }
  
} // namespace smproxy


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
