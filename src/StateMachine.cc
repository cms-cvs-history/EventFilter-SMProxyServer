// $Id: StateMachine.cc,v 1.1.2.8 2011/01/27 14:55:54 mommsen Exp $
/// @file: StateMachine.cc

#include "EventFilter/SMProxyServer/interface/DataManager.h"
#include "EventFilter/SMProxyServer/interface/StateMachine.h"
#include "EventFilter/SMProxyServer/interface/States.h"
#include "EventFilter/StorageManager/interface/EventConsumerMonitorCollection.h"
#include "EventFilter/StorageManager/interface/DQMConsumerMonitorCollection.h"

#include "xdata/InfoSpace.h"

#include <boost/bind.hpp>

#include <sstream>


namespace smproxy
{
  StateMachine::StateMachine
  (
    xdaq::Application* app
  ):
  _app(app),
  _rcmsStateNotifier
  (
    app->getApplicationLogger(),
    app->getApplicationDescriptor(),
    app->getApplicationContext()
  ),
  _reasonForFailed(""),
  _stateName("Halted")
  {
    std::ostringstream oss;
    oss << app->getApplicationDescriptor()->getClassName()
      << app->getApplicationDescriptor()->getInstance();
    _appNameAndInstance = oss.str();

    xdata::InfoSpace *is = app->getApplicationInfoSpace();
    is->fireItemAvailable("rcmsStateListener",
      _rcmsStateNotifier.getRcmsStateListenerParameter() );
    is->fireItemAvailable("foundRcmsStateListener",
      _rcmsStateNotifier.getFoundRcmsStateListenerParameter() );
    _rcmsStateNotifier.findRcmsStateListener();
    _rcmsStateNotifier.subscribeToChangesInRcmsStateListener(is);
  
    is->fireItemAvailable("stateName", &_stateName);

    initiate();

    _configuration.reset(new Configuration(
        app->getApplicationInfoSpace(), app->getApplicationDescriptor()->getInstance()
      ));

    _registrationCollection.reset( new stor::RegistrationCollection() );
    
    _registrationQueue.reset(new stor::RegistrationQueue(
        _configuration->getQueueConfigurationParams()._registrationQueueSize
      ));
    
    _initMsgCollection.reset(new stor::InitMsgCollection());

    _statisticsReporter.reset(new StatisticsReporter(app,
        _configuration->getQueueConfigurationParams()));

    _eventQueueCollection.reset(new EventQueueCollection(
        _statisticsReporter->getEventConsumerMonitorCollection()));
    
    _dqmEventQueueCollection.reset(new stor::DQMEventQueueCollection(
        _statisticsReporter->getDQMConsumerMonitorCollection()));
    
    _dataManager.reset(new DataManager(this));
  }
  
  
  std::string StateMachine::processEvent(const boost::statechart::event_base& event)
  {
    boost::mutex::scoped_lock sl(_eventMutex);
    process_event(event);
    return state_cast<const StateName&>().stateName();
  }
  
  
  void StateMachine::setExternallyVisibleStateName(const std::string& stateName)
  {
    _stateName = stateName;
    _rcmsStateNotifier.stateChanged(stateName,
      _appNameAndInstance + " has reached target state " +
      stateName);
  }
  
  
  void StateMachine::failEvent(const Fail& evt)
  {
    _stateName = "Failed";
    _reasonForFailed = evt.getTraceback();
    
    LOG4CPLUS_FATAL(_app->getApplicationLogger(),
      "Failed: " << evt.getReason() << ". " << _reasonForFailed);
    
    _rcmsStateNotifier.stateChanged(_stateName, evt.getReason());
    
    _app->notifyQualified("fatal", evt.getException());
  }
  
  
  void StateMachine::unconsumed_event(const boost::statechart::event_base& evt)
  {
    LOG4CPLUS_ERROR(_app->getApplicationLogger(),
      "The " << typeid(evt).name()
      << " event is not supported from the "
      << _stateName.toString() << " state!");
  }
  
  
  void StateMachine::updateConfiguration()
  {
    std::string errorMsg = "Failed to update configuration parameters";
    try
    {
      _configuration->updateAllParams();
    }
    catch(xcept::Exception &e)
    {
      XCEPT_DECLARE_NESTED(exception::Configuration,
        sentinelException, errorMsg, e);
      processEvent( Fail(sentinelException) );
    }
    catch( std::exception &e )
    {
      errorMsg.append(": ");
      errorMsg.append( e.what() );
      
      XCEPT_DECLARE(exception::Configuration,
        sentinelException, errorMsg);
      processEvent( Fail(sentinelException) );
    }
    catch(...)
    {
      errorMsg.append(": unknown exception");
      
      XCEPT_DECLARE(exception::Configuration,
        sentinelException, errorMsg);
      processEvent( Fail(sentinelException) );
    }
  }
  
  
  void StateMachine::setQueueSizes()
  {
    QueueConfigurationParams queueParams =
      _configuration->getQueueConfigurationParams();
    _registrationQueue->
      set_capacity(queueParams._registrationQueueSize);
  }
  
  
  void StateMachine::clearInitMsgCollection()
  {
    _initMsgCollection->clear();
  }
  
  
  void StateMachine::clearConsumerRegistrations()
  {
    _registrationCollection->clearRegistrations();
    _eventQueueCollection->removeQueues();
    _dqmEventQueueCollection->removeQueues();
  }
  
  
  void StateMachine::enableConsumerRegistration()
  {
    _dataManager->start(_configuration->getDataRetrieverParams());
    _registrationCollection->enableConsumerRegistration();
  }
 
  
  void StateMachine::disableConsumerRegistration()
  {
    _registrationCollection->disableConsumerRegistration();
    _dataManager->stop();
  }
 
  
  void StateMachine::clearQueues()
  {
    _registrationQueue->clear();
    _eventQueueCollection->clearQueues();
    _dqmEventQueueCollection->clearQueues();
  }
  
  
  void Configuring::entryAction()
  {
    configuringThread_.reset(
      new boost::thread( boost::bind( &Configuring::activity, this) )
    );
  }
  
  
  void Configuring::activity()
  {
    outermost_context_type& stateMachine = outermost_context();
    stateMachine.updateConfiguration();
    boost::this_thread::interruption_point();
    stateMachine.setQueueSizes();
    boost::this_thread::interruption_point();
    stateMachine.processEvent( ConfiguringDone() );
  }

  
  void Configuring::exitAction()
  {
    configuringThread_->interrupt();
  }
  
  
  void Starting::entryAction()
  {
    startingThread_.reset(
      new boost::thread( boost::bind( &Starting::activity, this) )
    );
  }
  
  
  void Starting::activity()
  {
    outermost_context_type& stateMachine = outermost_context();
    stateMachine.clearInitMsgCollection();
    boost::this_thread::interruption_point();
    stateMachine.clearConsumerRegistrations();
    boost::this_thread::interruption_point();
    stateMachine.enableConsumerRegistration();
    boost::this_thread::interruption_point();
    stateMachine.processEvent( StartingDone() );
  }

  
  void Starting::exitAction()
  {
    startingThread_->interrupt();
  }
  
  
  void Stopping::entryAction()
  {
    stoppingThread_.reset(
      new boost::thread( boost::bind( &Stopping::activity, this) )
    );
  }
  
  
  void Stopping::activity()
  {
    outermost_context_type& stateMachine = outermost_context();
    stateMachine.disableConsumerRegistration();
    boost::this_thread::interruption_point();
    stateMachine.clearQueues();
    boost::this_thread::interruption_point();
    stateMachine.processEvent( StoppingDone() );
  }

  
  void Stopping::exitAction()
  {
    stoppingThread_->interrupt();
  }
  
  
  void Halting::entryAction()
  {
    haltingThread_.reset(
      new boost::thread( boost::bind( &Halting::activity, this) )
    );
  }
  
  
  void Halting::activity()
  {
    outermost_context_type& stateMachine = outermost_context();
    stateMachine.disableConsumerRegistration();
    boost::this_thread::interruption_point();
    stateMachine.clearQueues();
    boost::this_thread::interruption_point();
    stateMachine.processEvent( HaltingDone() );
  }

  
  void Halting::exitAction()
  {
    haltingThread_->interrupt();
  }

} // namespace smproxy


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
