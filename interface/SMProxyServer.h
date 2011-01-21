// $Id: SMProxyServer.h,v 1.58 2010/04/16 12:31:34 mommsen Exp $
/// @file: SMProxyServer.h 

#ifndef SMProxyServer_SMProxyServer_h
#define SMProxyServer_SMProxyServer_h

#include "EventFilter/SMProxyServer/interface/StateMachine.h"
#include "EventFilter/StorageManager/interface/ConsumerUtils.h"
#include "EventFilter/StorageManager/interface/WebPageHelper.h"

#include "FWCore/PluginManager/interface/ProblemTracker.h"

#include "xdaq/Application.h"
#include "xgi/exception/Exception.h"
#include "xoap/MessageReference.h"


namespace xgi {
  class Input;
  class Output;
}

namespace smproxy {

  /**
   * Main class of the SMProxyServer XDAQ application
   *
   * $Author: mommsen $
   * $Revision: 1.58 $
   * $Date: 2010/04/16 12:31:34 $
   */

  class SMProxyServer: public xdaq::Application
  {

  public:
  
    SMProxyServer( xdaq::ApplicationStub* s );
  
    ~SMProxyServer();


  private:  
  
    SMProxyServer(SMProxyServer const&); // not implemented
    SMProxyServer& operator=(SMProxyServer const&); // not implemented

    /**
     * Bind callbacks for state machine SOAP messages
     */
    void bindStateMachineCallbacks();

    /**
     * Callback for SOAP message containint a state machine event,
     * possibly including new configuration values
     */
    xoap::MessageReference handleFSMSoapMessage( xoap::MessageReference )
      throw( xoap::exception::Exception );


    /**
     * Bind callbacks for web interface
     */
    void bindWebInterfaceCallbacks();

    /**
     * Webinterface callback for style sheet
     */
    void css(xgi::Input *in, xgi::Output *out)
      throw (xgi::exception::Exception);

    /**
     * Webinterface callback creating default web page
     */
    void defaultWebPage(xgi::Input *in, xgi::Output *out)
      throw (xgi::exception::Exception);

    /**
     * Webinterface callback creating web page showing the connected consumers
     */
    void consumerStatisticsPage( xgi::Input* in, xgi::Output* out )
      throw( xgi::exception::Exception );

    /**
     * Webinterface callback creating web page showing statistics about the
     * processed DQM events.
     */
    void dqmEventStatisticsWebPage(xgi::Input *in, xgi::Output *out)
      throw (xgi::exception::Exception);

    /**
     * Webinterface callback creating web page showing statistics about the
     * data throughput in the SM.
     */
    void throughputWebPage(xgi::Input *in, xgi::Output *out)
      throw (xgi::exception::Exception);

    /**
     * Bind callbacks for consumers
     */
    void bindConsumerCallbacks();

    /**
     * Callback handling event consumer registration request
     */
    void processConsumerRegistrationRequest( xgi::Input* in, xgi::Output* out )
      throw( xgi::exception::Exception );

    /**
     * Callback handling event consumer init message request
     */
    void processConsumerHeaderRequest( xgi::Input* in, xgi::Output* out )
      throw( xgi::exception::Exception );

    /**
     * Callback handling event consumer event request
     */
    void processConsumerEventRequest( xgi::Input* in, xgi::Output* out )
      throw( xgi::exception::Exception );
 
    /**
     * Callback handling DQM event consumer registration request
     */
    void processDQMConsumerRegistrationRequest( xgi::Input* in, xgi::Output* out )
      throw( xgi::exception::Exception );

    /**
     * Callback handling DQM event consumer DQM event request
     */
    void processDQMConsumerEventRequest( xgi::Input* in, xgi::Output* out )
      throw( xgi::exception::Exception );

    /**
     * Initialize the shared resources
     */
    void initializeSharedResources();

    /**
     * Create and start all worker threads
     */
    void startWorkerThreads();

    // instantiate the plugin manager, not referenced here after!
    edm::AssertHandler _ah;

    boost::scoped_ptr<StateMachine> _stateMachine;

    typedef ConsumerUtils<Configuration,EventQueueCollection> ConsumerUtils_t;
    boost::scoped_ptr<ConsumerUtils_t> _consumerUtils;
    stor::WebPageHelper _webPageHelper;

  };

} // namespace smproxy

#endif // SMProxyServer_SMProxyServer_h


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
