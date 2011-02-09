// $Id: SMPSWebPageHelper.h,v 1.1.2.2 2011/02/08 16:51:51 mommsen Exp $
/// @file: SMPSWebPageHelper.h

#ifndef EventFilter_SMProxyServer_SMPSWebPageHelper_h
#define EventFilter_SMProxyServer_SMPSWebPageHelper_h

#include "EventFilter/SMProxyServer/interface/EventQueueCollection.h"
#include "EventFilter/SMProxyServer/interface/StateMachine.h"
#include "EventFilter/StorageManager/interface/ConsumerWebPageHelper.h"
#include "EventFilter/StorageManager/interface/WebPageHelper.h"


namespace smproxy {

  /**
   * Helper class to handle SM proxy server web page requests
   *
   * $Author: mommsen $
   * $Revision: 1.1.2.2 $
   * $Date: 2011/02/08 16:51:51 $
   */
  
  class SMPSWebPageHelper : public stor::WebPageHelper<SMPSWebPageHelper>
  {
  public:

    SMPSWebPageHelper
    (
      xdaq::ApplicationDescriptor*,
      StateMachinePtr
    );
    
    /**
       Generates the default web page
    */
    void defaultWebPage(xgi::Output*);
    
    /**
       Generates the data retriever web page
    */
    void dataRetrieverWebPage(xgi::Output*);
    
    /**
       Generates the data retriever web page
    */
    void dqmEventStatisticsWebPage(xgi::Output*);

    /**
       Generates consumer statistics page
    */
    void consumerStatisticsWebPage(xgi::Output*);
    
    
  private:
    
    /**
     * Adds the links for the other hyperdaq webpages
     */
    virtual void addDOMforHyperLinks(stor::XHTMLMaker&, stor::XHTMLMaker::Node* parent);

    /**
     * Adds the event servers to the parent DOM element
     */
    void addDOMforEventServers
    (
      stor::XHTMLMaker&,
      stor::XHTMLMaker::Node* parent
    );
 
    /**
     * Adds a table row for each event server
     */
    void addRowForEventServer
    (
      stor::XHTMLMaker&,
      stor::XHTMLMaker::Node* table,
      DataRetrieverMonitorCollection::DataRetrieverStats const&
    );
    

    /**
     * Adds the DQM event (histogram) servers to the parent DOM element
     */
    void addDOMforDQMEventServers
    (
      stor::XHTMLMaker&,
      stor::XHTMLMaker::Node* parent
    );
    
    //Prevent copying of the SMPSWebPageHelper
    SMPSWebPageHelper(SMPSWebPageHelper const&);
    SMPSWebPageHelper& operator=(SMPSWebPageHelper const&);

    StateMachinePtr _stateMachine;

    typedef stor::ConsumerWebPageHelper<SMPSWebPageHelper,
                                        EventQueueCollection,
                                        StatisticsReporter> ConsumerWebPageHelper_t;
    ConsumerWebPageHelper_t _consumerWebPageHelper;

  };

} // namespace smproxy

#endif // EventFilter_SMProxyServer_SMPSWebPageHelper_h 


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
