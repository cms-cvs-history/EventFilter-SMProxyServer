// $Id: SMPSWebPageHelper.h,v 1.12.2.1 2011/01/24 12:18:39 mommsen Exp $
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
   * $Revision: 1.12.2.1 $
   * $Date: 2011/01/24 12:18:39 $
   */
  
  class SMPSWebPageHelper : public stor::WebPageHelper
  {
  public:

    SMPSWebPageHelper
    (
      xdaq::ApplicationDescriptor*,
      StateMachinePtr
    );
    
    /**
       Generates consumer statistics page
    */
    void consumerStatistics(xgi::Output*);
    
    
  private:
    
    /**
     * Adds the links for the other hyperdaq webpages
     */
    virtual void addDOMforHyperLinks(stor::XHTMLMaker&, stor::XHTMLMaker::Node* parent) {};


    //Prevent copying of the SMPSWebPageHelper
    SMPSWebPageHelper(SMPSWebPageHelper const&);
    SMPSWebPageHelper& operator=(SMPSWebPageHelper const&);

    StateMachinePtr _stateMachine;

    typedef stor::ConsumerWebPageHelper<EventQueueCollection,StatisticsReporter> ConsumerWebPageHelper_t;
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
