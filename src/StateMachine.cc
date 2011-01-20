// $Id: Configuration.cc,v 1.1.2.2 2011/01/19 16:22:02 mommsen Exp $
/// @file: StateMachine.cc

#include "EventFilter/SMProxyServer/interface/StateMachine.h"
#include "EventFilter/SMProxyServer/interface/States.h"

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
  _reasonForFailed("")
  {
    initiate();

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
  
  
  void Configuring::entryAction()
  {
    doConfiguring_ = true;
    configuringThread_.reset(
      new boost::thread( boost::bind( &Configuring::activity, this) )
    );
  }
  
  
  void Configuring::activity()
  {
    outermost_context_type& stateMachine = outermost_context();
    if (doConfiguring_) stateMachine.processEvent( ConfiguringDone() );
  }

  
  void Configuring::exitAction()
  {
    doConfiguring_ = false;
    configuringThread_->join();
  }
  
  
  void Starting::entryAction()
  {
    doStarting_ = true;
    startingThread_.reset(
      new boost::thread( boost::bind( &Starting::activity, this) )
    );
  }
  
  
  void Starting::activity()
  {
    outermost_context_type& stateMachine = outermost_context();
    if (doStarting_) stateMachine.processEvent( StartingDone() );
  }

  
  void Starting::exitAction()
  {
    doStarting_ = false;
    startingThread_->join();
  }
  
  
  void Stopping::entryAction()
  {
    doStopping_ = true;
    stoppingThread_.reset(
      new boost::thread( boost::bind( &Stopping::activity, this) )
    );
  }
  
  
  void Stopping::activity()
  {
    outermost_context_type& stateMachine = outermost_context();
    if (doStopping_) stateMachine.processEvent( StoppingDone() );
  }

  
  void Stopping::exitAction()
  {
    doStopping_ = false;
    stoppingThread_->join();
  }
  
  
  void Halting::entryAction()
  {
    doHalting_ = true;
    haltingThread_.reset(
      new boost::thread( boost::bind( &Halting::activity, this) )
    );
  }
  
  
  void Halting::activity()
  {
    outermost_context_type& stateMachine = outermost_context();
    if (doHalting_) stateMachine.processEvent( HaltingDone() );
  }

  
  void Halting::exitAction()
  {
    doHalting_ = false;
    haltingThread_->join();
  }

} // namespace smproxy


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
