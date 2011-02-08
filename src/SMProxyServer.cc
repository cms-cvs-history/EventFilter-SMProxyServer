// $Id: SMProxyServer.cc,v 1.44.2.4 2011/01/26 16:06:54 mommsen Exp $
/// @file: SMProxyServer.cc

#include "EventFilter/SMProxyServer/interface/Exception.h"
#include "EventFilter/SMProxyServer/interface/SMProxyServer.h"
#include "EventFilter/SMProxyServer/interface/StateMachine.h"
#include "EventFilter/StorageManager/interface/SoapUtils.h"
#include "EventFilter/StorageManager/src/ConsumerUtils.icc"

#include "FWCore/Utilities/interface/EDMException.h"

#include "xcept/tools.h"
#include "xdaq/NamespaceURI.h"
#include "xdata/InfoSpaceFactory.h"
#include "xgi/Method.h"
#include "xoap/Method.h"

#include <cstdlib>

using namespace std;
using namespace smproxy;


SMProxyServer::SMProxyServer(xdaq::ApplicationStub * s) :
  xdaq::Application(s)
{  
  LOG4CPLUS_INFO(this->getApplicationLogger(),"Making SMProxyServer");

  // bind all callback functions
  bindStateMachineCallbacks();
  bindWebInterfaceCallbacks();
  bindConsumerCallbacks();

  std::string errorMsg = "Exception in SMProxyServer constructor: ";
  try
  {
    initializeSharedResources();
  }
  catch(std::exception &e)
  {
    errorMsg += e.what();
    LOG4CPLUS_FATAL( getApplicationLogger(), e.what() );
    XCEPT_RAISE( stor::exception::Exception, e.what() );
  }
  catch(...)
  {
    errorMsg += "unknown exception";
    LOG4CPLUS_FATAL( getApplicationLogger(), errorMsg );
    XCEPT_RAISE( stor::exception::Exception, errorMsg );
  }

  startWorkerThreads();
}


void SMProxyServer::bindStateMachineCallbacks()
{
  xoap::bind( this,
              &SMProxyServer::handleFSMSoapMessage,
              "Configure",
              XDAQ_NS_URI );
  xoap::bind( this,
              &SMProxyServer::handleFSMSoapMessage,
              "Enable",
              XDAQ_NS_URI );
  xoap::bind( this,
              &SMProxyServer::handleFSMSoapMessage,
              "Stop",
              XDAQ_NS_URI );
  xoap::bind( this,
              &SMProxyServer::handleFSMSoapMessage,
              "Halt",
              XDAQ_NS_URI );
}


void SMProxyServer::bindWebInterfaceCallbacks()
{
  xgi::bind(this,&SMProxyServer::css,                      "styles.css");
  xgi::bind(this,&SMProxyServer::defaultWebPage,           "Default");
  xgi::bind(this,&SMProxyServer::dataRetrieverWebPage,     "dataRetriever");
  xgi::bind(this,&SMProxyServer::dqmEventStatisticsWebPage,"dqmEventStatistics");
  xgi::bind(this,&SMProxyServer::consumerStatisticsWebPage,"consumerStatistics" );
}


void SMProxyServer::bindConsumerCallbacks()
{
  // event consumers
  xgi::bind( this, &SMProxyServer::processConsumerRegistrationRequest, "registerConsumer" );
  xgi::bind( this, &SMProxyServer::processConsumerHeaderRequest, "getregdata" );
  xgi::bind( this, &SMProxyServer::processConsumerEventRequest, "geteventdata" );

  // dqm event consumers
  xgi::bind(this,&SMProxyServer::processDQMConsumerRegistrationRequest, "registerDQMConsumer");
  xgi::bind(this,&SMProxyServer::processDQMConsumerEventRequest, "getDQMeventdata");
}


void SMProxyServer::initializeSharedResources()
{
  _stateMachine.reset( new StateMachine(this) );
  
  _consumerUtils.reset( new ConsumerUtils_t (
      _stateMachine->getConfiguration(),
      _stateMachine->getRegistrationCollection(),
      _stateMachine->getRegistrationQueue(),
      _stateMachine->getInitMsgCollection(),
      _stateMachine->getEventQueueCollection(),
      _stateMachine->getDQMEventQueueCollection(),
      _stateMachine->getStatisticsReporter()->alarmHandler()
    ) );

  _smpsWebPageHelper.reset( new SMPSWebPageHelper(
      getApplicationDescriptor(), _stateMachine));
}


void SMProxyServer::startWorkerThreads()
{
  // Start the workloops
  try
  {
    _stateMachine->getStatisticsReporter()->startWorkLoop("theStatisticsReporter");
  }
  catch(xcept::Exception &e)
  {
    _stateMachine->processEvent( Fail(e) );
  }
  catch(std::exception &e)
  {
    XCEPT_DECLARE(exception::Exception,
      sentinelException, e.what());
    _stateMachine->processEvent( Fail(sentinelException) );
  }
  catch(...)
  {
    std::string errorMsg = "Unknown exception when starting the workloops";
    XCEPT_DECLARE(exception::Exception,
      sentinelException, errorMsg);
    _stateMachine->processEvent( Fail(sentinelException) );
  }
}


///////////////////////////////////////
// Web interface call back functions //
///////////////////////////////////////

void SMProxyServer::css(xgi::Input *in, xgi::Output *out)
throw (xgi::exception::Exception)
{
  _smpsWebPageHelper->css(in,out);
}


void SMProxyServer::defaultWebPage(xgi::Input *in, xgi::Output *out)
throw (xgi::exception::Exception)
{
  std::string errorMsg = "Failed to create the default webpage";
  
  try
  {
    _smpsWebPageHelper->defaultWebPage(out);
  }
  catch(std::exception &e)
  {
    errorMsg += ": ";
    errorMsg += e.what();
    
    LOG4CPLUS_ERROR(getApplicationLogger(), errorMsg);
    XCEPT_RAISE(xgi::exception::Exception, errorMsg);
  }
  catch(...)
  {
    errorMsg += ": Unknown exception";
    
    LOG4CPLUS_ERROR(getApplicationLogger(), errorMsg);
    XCEPT_RAISE(xgi::exception::Exception, errorMsg);
  }
}


void SMProxyServer::dataRetrieverWebPage(xgi::Input* in, xgi::Output* out)
throw( xgi::exception::Exception )
{

  std::string err_msg =
    "Failed to create data retriever web page";

  try
  {
    _smpsWebPageHelper->dataRetrieverWebPage(out);
  }
  catch( std::exception &e )
  {
    err_msg += ": ";
    err_msg += e.what();
    LOG4CPLUS_ERROR( getApplicationLogger(), err_msg );
    XCEPT_RAISE( xgi::exception::Exception, err_msg );
  }
  catch(...)
  {
    err_msg += ": Unknown exception";
    LOG4CPLUS_ERROR( getApplicationLogger(), err_msg );
    XCEPT_RAISE( xgi::exception::Exception, err_msg );
  }
}


void SMProxyServer::consumerStatisticsWebPage(xgi::Input* in, xgi::Output* out)
throw( xgi::exception::Exception )
{

  std::string err_msg =
    "Failed to create consumer web page";

  try
  {
    _smpsWebPageHelper->consumerStatisticsWebPage(out);
  }
  catch( std::exception &e )
  {
    err_msg += ": ";
    err_msg += e.what();
    LOG4CPLUS_ERROR( getApplicationLogger(), err_msg );
    XCEPT_RAISE( xgi::exception::Exception, err_msg );
  }
  catch(...)
  {
    err_msg += ": Unknown exception";
    LOG4CPLUS_ERROR( getApplicationLogger(), err_msg );
    XCEPT_RAISE( xgi::exception::Exception, err_msg );
  }
}


void SMProxyServer::dqmEventStatisticsWebPage(xgi::Input *in, xgi::Output *out)
throw (xgi::exception::Exception)
{
  std::string errorMsg = "Failed to create the DQM event statistics webpage";

  try
  {
    _smpsWebPageHelper->dqmEventStatisticsWebPage(out);
  }
  catch(std::exception &e)
  {
    errorMsg += ": ";
    errorMsg += e.what();
    
    LOG4CPLUS_ERROR(getApplicationLogger(), errorMsg);
    XCEPT_RAISE(xgi::exception::Exception, errorMsg);
  }
  catch(...)
  {
    errorMsg += ": Unknown exception";
    
    LOG4CPLUS_ERROR(getApplicationLogger(), errorMsg);
    XCEPT_RAISE(xgi::exception::Exception, errorMsg);
  }
}


///////////////////////////////////////
// State Machine call back functions //
///////////////////////////////////////

xoap::MessageReference SMProxyServer::handleFSMSoapMessage( xoap::MessageReference msg )
  throw( xoap::exception::Exception )
{
  std::string errorMsg;
  xoap::MessageReference returnMsg;

  try {
    errorMsg = "Failed to extract FSM event and parameters from SOAP message: ";
    std::string command = stor::soaputils::extractParameters(msg, this);
    std::string newState = "unknown";  

    errorMsg = "Failed to process '" + command + "' state machine event: ";
    if (command == "Configure")
    {
      newState = _stateMachine->processEvent( Configure() );
    }
    else if (command == "Enable")
    {
      newState = _stateMachine->processEvent( Enable() );
    }
    else if (command == "Stop")
    {
      newState = _stateMachine->processEvent( Stop() );
    }
    else if (command == "Halt")
    {
      newState = _stateMachine->processEvent( Halt() );
    }
    else
    {
      XCEPT_RAISE(exception::StateMachine,
        "Received an unknown state machine event '" + command + "'.");
    }

    errorMsg = "Failed to create FSM SOAP reply message: ";
    returnMsg = stor::soaputils::createFsmSoapResponseMsg(command, newState);
  }
  catch (cms::Exception& e) {
    errorMsg += e.explainSelf();
    XCEPT_DECLARE(xoap::exception::Exception,
      sentinelException, errorMsg);
    _stateMachine->processEvent( Fail(sentinelException) );
  }
  catch (xcept::Exception &e) {
    XCEPT_DECLARE_NESTED(xoap::exception::Exception,
      sentinelException, errorMsg, e);
    _stateMachine->processEvent( Fail(sentinelException) );
  }
  catch (std::exception& e) {
    errorMsg += e.what();
    XCEPT_DECLARE(xoap::exception::Exception,
      sentinelException, errorMsg);
    _stateMachine->processEvent( Fail(sentinelException) );
  }
  catch (...) {
    errorMsg += "Unknown exception";
    XCEPT_DECLARE(xoap::exception::Exception,
      sentinelException, errorMsg);
    _stateMachine->processEvent( Fail(sentinelException) );
  }

  return returnMsg;
}


////////////////////////////
//// Consumer callbacks ////
////////////////////////////

void
SMProxyServer::processConsumerRegistrationRequest( xgi::Input* in, xgi::Output* out )
  throw( xgi::exception::Exception )
{
  _consumerUtils->processConsumerRegistrationRequest(in,out);
}


void
SMProxyServer::processConsumerHeaderRequest( xgi::Input* in, xgi::Output* out )
  throw( xgi::exception::Exception )
{
  _consumerUtils->processConsumerHeaderRequest(in,out);
}


void
SMProxyServer::processConsumerEventRequest( xgi::Input* in, xgi::Output* out )
  throw( xgi::exception::Exception )
{
  _consumerUtils->processConsumerEventRequest(in,out);
}


void
SMProxyServer::processDQMConsumerRegistrationRequest( xgi::Input* in, xgi::Output* out )
  throw( xgi::exception::Exception )
{
  _consumerUtils->processDQMConsumerRegistrationRequest(in,out);
}


void
SMProxyServer::processDQMConsumerEventRequest( xgi::Input* in, xgi::Output* out )
  throw( xgi::exception::Exception )
{
  _consumerUtils->processDQMConsumerEventRequest(in,out);
}


//////////////////////////////////////////////////////////////////////////
// *** Provides factory method for the instantiation of SM applications //
//////////////////////////////////////////////////////////////////////////
// This macro is depreciated:
XDAQ_INSTANTIATE(SMProxyServer)

// One should use the XDAQ_INSTANTIATOR() in the header file
// and this one here. But this breaks the backward compatibility,
// as all xml configuration files would have to be changed to use
// 'stor::SMProxyServer' instead of 'SMProxyServer'.
// XDAQ_INSTANTIATOR_IMPL(stor::SMProxyServer)


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
