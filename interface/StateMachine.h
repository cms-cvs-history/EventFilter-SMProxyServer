// $Id: StateMachine.h,v 1.1.2.6 2011/02/11 12:13:44 mommsen Exp $
/// @file: StateMachine.h 

#ifndef EventFilter_SMProxyServer_StateMachine_h
#define EventFilter_SMProxyServer_StateMachine_h

#include "EventFilter/SMProxyServer/interface/Configuration.h"
#include "EventFilter/SMProxyServer/interface/DataManager.h"
#include "EventFilter/SMProxyServer/interface/EventQueueCollection.h"
#include "EventFilter/SMProxyServer/interface/StatisticsReporter.h"
#include "EventFilter/StorageManager/interface/DQMEventQueueCollection.h"
#include "EventFilter/StorageManager/interface/InitMsgCollection.h"
#include "EventFilter/StorageManager/interface/RegistrationCollection.h"
#include "EventFilter/StorageManager/interface/RegistrationQueue.h"

#include "xcept/Exception.h"
#include "xcept/tools.h"
#include "xdaq/Application.h"
#include "xdaq/ApplicationDescriptor.h"
#include "xdaq2rc/RcmsStateNotifier.h"

#include <boost/shared_ptr.hpp>
#include <boost/statechart/event_base.hpp>
#include <boost/statechart/state.hpp>
#include <boost/statechart/state_machine.hpp>
#include <boost/thread/mutex.hpp>

#include <string>


namespace smproxy
{

  //////////////////////
  // Outermost states //
  //////////////////////
  class AllOk;
  class Failed;

  ///////////////////
  // Public events //
  ///////////////////
  class Configure : public boost::statechart::event<Configure> {};
  class Enable : public boost::statechart::event<Enable> {};
  class Halt : public boost::statechart::event<Halt> {};
  class Stop : public boost::statechart::event<Stop> {};

  class Fail : public boost::statechart::event<Fail>
  {
  public:
    Fail(xcept::Exception& exception) : _exception(exception) {};
    std::string getReason() const { return _exception.message(); }
    std::string getTraceback() const { return xcept::stdformat_exception_history(_exception); }
    xcept::Exception& getException() const { return _exception; }

  private:
    mutable xcept::Exception _exception;
  };

  struct StateName {
    virtual std::string stateName() const = 0;
  };

  ///////////////////////
  // The state machine //
  ///////////////////////

  class StateMachine: public boost::statechart::state_machine<StateMachine,AllOk>
  {
    
  public:
    
    StateMachine
    (
      xdaq::Application*
    );

    std::string processEvent(const boost::statechart::event_base&);
    void moveToFailedState(xcept::Exception& e)
    { processEvent( Fail(e) ); }

    void setExternallyVisibleStateName(const std::string& stateName);
    void failEvent(const Fail&);
    void unconsumed_event(const boost::statechart::event_base&);

    std::string getReasonForFailed()
    { return _reasonForFailed; }
    std::string getStateName()
    { return state_cast<const StateName&>().stateName(); }
    std::string getExternallyVisibleStateName()
    { return _stateName.toString(); }

    ConfigurationPtr getConfiguration() const
    { return _configuration; }
    DataManagerPtr getDataManager() const
    { return _dataManager; }
    stor::RegistrationCollectionPtr getRegistrationCollection() const
    { return _registrationCollection; }
    stor::RegistrationQueuePtr getRegistrationQueue() const
    { return  _registrationQueue; }
    stor::InitMsgCollectionPtr getInitMsgCollection() const
    { return _initMsgCollection; }
    EventQueueCollectionPtr getEventQueueCollection() const
    { return _eventQueueCollection; }
    stor::DQMEventQueueCollectionPtr getDQMEventQueueCollection() const
    { return _dqmEventQueueCollection; }
    StatisticsReporterPtr getStatisticsReporter() const
    { return _statisticsReporter; }
    xdaq::ApplicationDescriptor* getApplicationDescriptor() const
    { return _app->getApplicationDescriptor(); }

    void updateConfiguration();
    void setQueueSizes();
    void clearInitMsgCollection();
    void clearConsumerRegistrations();
    void enableConsumerRegistration();
    void disableConsumerRegistration();
    void clearQueues();
    
    
  private:

    xdaq::Application* _app;
    xdaq2rc::RcmsStateNotifier _rcmsStateNotifier;
    ConfigurationPtr _configuration;
    DataManagerPtr _dataManager;
    stor::RegistrationCollectionPtr _registrationCollection;
    stor::RegistrationQueuePtr _registrationQueue;
    stor::InitMsgCollectionPtr _initMsgCollection;
    StatisticsReporterPtr _statisticsReporter;
    EventQueueCollectionPtr _eventQueueCollection;
    stor::DQMEventQueueCollectionPtr _dqmEventQueueCollection;

    mutable boost::mutex _eventMutex;
    
    std::string _appNameAndInstance;
    std::string _reasonForFailed;
    xdata::String _stateName;
    
  };
  
  typedef boost::shared_ptr<StateMachine> StateMachinePtr;

} //namespace smproxy

#endif //SMProxyServer_StateMachine_h

/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
