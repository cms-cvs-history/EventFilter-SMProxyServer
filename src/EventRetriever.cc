// $Id: EventRetriever.cc,v 1.1.2.14 2011/02/11 12:13:44 mommsen Exp $
/// @file: EventRetriever.cc

#include "EventFilter/SMProxyServer/interface/EventRetriever.h"
#include "EventFilter/SMProxyServer/interface/Exception.h"
#include "EventFilter/SMProxyServer/interface/StateMachine.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "FWCore/Utilities/interface/Exception.h"
#include "FWCore/Utilities/interface/UnixSignalHandlers.h"
#include "IOPool/Streamer/interface/EventMessage.h"
#include "IOPool/Streamer/interface/InitMessage.h"


namespace smproxy
{
  size_t EventRetriever::_retrieverCount(0);


  EventRetriever::EventRetriever
  (
    StateMachine* stateMachine,
    stor::EventConsRegPtr consumer
  ) :
  _stateMachine(stateMachine),
  _pset(consumer->getPSet()),
  _dataRetrieverParams(stateMachine->getConfiguration()->getDataRetrieverParams()),
  _dataRetrieverMonitorCollection(stateMachine->getStatisticsReporter()->getDataRetrieverMonitorCollection()),
  _minEventRequestInterval(consumer->minEventRequestInterval()),
  _instance(++_retrieverCount)
  {
    _pset.addUntrackedParameter<std::string>("consumerName",
      _stateMachine->getApplicationDescriptor()->getContextDescriptor()->getURL()+"/"+
      _stateMachine->getApplicationDescriptor()->getURN());

    if ( _dataRetrieverParams._allowMissingSM )
    {
      _pset.addUntrackedParameter<int>("maxConnectTries", 0);
    }
    else
    {
      _pset.addUntrackedParameter<int>("maxConnectTries", _dataRetrieverParams._maxConnectionRetries);
      _pset.addUntrackedParameter<int>("connectTrySleepTime", _dataRetrieverParams._connectTrySleepTime);
    }
    _pset.addUntrackedParameter<int>("headerRetryInterval", _dataRetrieverParams._headerRetryInterval);

    _nextRequestTime = stor::utils::getCurrentTime();
    _queueIDs.push_back(consumer->queueId());

    _thread.reset(
      new boost::thread( boost::bind( &EventRetriever::activity, this) )
    );
  }
  
  
  EventRetriever::~EventRetriever()
  {
    stop();
  }
  
  
  void EventRetriever::addConsumer(stor::EventConsRegPtr consumer)
  {
    if ( adjustMinEventRequestInterval( consumer->minEventRequestInterval() ) )
    {
      _dataRetrieverMonitorCollection.updateConsumerInfo( consumer ); 
    }

    boost::mutex::scoped_lock sl(_queueIDsLock);
    _queueIDs.push_back(consumer->queueId());
  }
  
  
  bool EventRetriever::adjustMinEventRequestInterval(const stor::utils::duration_t& newInterval)
  {
    if ( _minEventRequestInterval.is_not_a_date_time() )
    {
      // A previous registered consumer wants to go as fast as possible.
      // Thus, do nothing
      return false;
    }

    if ( newInterval.is_not_a_date_time() )
    {
      // The new consumer wants to go as fast as possible.
      _minEventRequestInterval = boost::posix_time::not_a_date_time;
      _nextRequestTime = stor::utils::getCurrentTime();
      return true;
    }

    if ( newInterval < _minEventRequestInterval )
    {
      // The new consumer wants to go faster
      // Correct next request time for the shorter interval
      _nextRequestTime -= (_minEventRequestInterval - newInterval);
      _minEventRequestInterval = newInterval;
      return true;      
    }

    return false;
  }

  
  void EventRetriever::stop()
  {
    _thread->interrupt();
    _thread->join();
    
    boost::mutex::scoped_lock sl(_queueIDsLock);
    _queueIDs.clear();
  }
    
  
  void EventRetriever::activity()
  {
    try
    {
      doIt();
    }
    catch(boost::thread_interrupted)
    {
      // thread was interrupted.
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
      std::string errorMsg = "Unknown exception while retrieving events";
      XCEPT_DECLARE(exception::Exception,
        sentinelException, errorMsg);
      _stateMachine->processEvent( Fail(sentinelException) );
    }
  }
  
  void EventRetriever::doIt()
  {
    connect();

    getInitMsg();

    EventQueueCollectionPtr eventQueueCollection =
      _stateMachine->getEventQueueCollection();

    while ( !edm::shutdown_flag )
    {
      if ( anyActiveConsumers(eventQueueCollection) )
      {
        EventMsg event;
        if ( ! getNextEvent(event) ) break;
        
        {
          boost::mutex::scoped_lock sl(_queueIDsLock);
          event.tagForEventConsumers(_queueIDs);
        }
        
        eventQueueCollection->addEvent(event);
      }
      else
      {
        boost::this_thread::sleep(_dataRetrieverParams._sleepTimeIfIdle);
      }
    }
  }
  
  
  bool EventRetriever::anyActiveConsumers(EventQueueCollectionPtr eventQueueCollection) const
  {
    boost::mutex::scoped_lock sl(_queueIDsLock);
    stor::utils::time_point_t now = stor::utils::getCurrentTime();
    
    for ( QueueIDs::const_iterator it = _queueIDs.begin(), itEnd = _queueIDs.end();
          it != itEnd; ++it)
    {
      if ( ! eventQueueCollection->stale(*it, now) ) return true;
    }
    
    return false;
  }
  
  
  bool EventRetriever::connect()
  {
    size_t smCount = _dataRetrieverParams._smRegistrationList.size();
    _eventServers.clear();

    for (size_t i = 0; i < smCount; ++i)
    { 
      if (edm::shutdown_flag) continue;
      
      // each event retriever shall start from a different SM
      size_t smInstance = (_instance + i) % smCount;
      std::string sourceURL = _dataRetrieverParams._smRegistrationList[smInstance];
      connectToSM(sourceURL);
    }

    if ( _eventServers.empty() )
    {
      XCEPT_RAISE(exception::DataRetrieval, "Could not connect to any SM");
    }
    
    // start with the first SM in our list
    _nextSMtoUse = _eventServers.begin();

    const size_t connectedSM = _eventServers.size();
    
    return (connectedSM == smCount);
  }
  
  
  void EventRetriever::connectToSM(const std::string& sourceURL)
  {
    _pset.addParameter<std::string>("sourceURL", sourceURL);

    const ConnectionID connectionId =
      _dataRetrieverMonitorCollection.addNewConnection(_pset);
    
    try
    {
      EventServerPtr eventServerPtr(new stor::EventServerProxy(_pset));
      
      _eventServers.insert(EventServers::value_type(connectionId, eventServerPtr));
      _dataRetrieverMonitorCollection.setConnectionStatus(
        connectionId, DataRetrieverMonitorCollection::CONNECTED);
    }
    catch (cms::Exception& e)
    {
      _dataRetrieverMonitorCollection.setConnectionStatus(
        connectionId, DataRetrieverMonitorCollection::CONNECTION_FAILED);
      
      std::ostringstream errorMsg;
      errorMsg << "Failed to connect to SM on " << sourceURL;
      
      if ( ! _dataRetrieverParams._allowMissingSM )
        XCEPT_RAISE(exception::DataRetrieval, errorMsg.str());
    }
  }
  
  
  void EventRetriever::getInitMsg()
  {
    do
    {
      try
      {
        std::string data;
        _nextSMtoUse->second->getInitMsg(data);
        InitMsgView initMsgView(&data[0]);
        _stateMachine->getInitMsgCollection()->addIfUnique(initMsgView);
        return;
      }
      catch (cms::Exception& e)
      {
        // Faulty init msg retrieved
        disconnectFromCurrentSM();
      }
    }
    while (_nextSMtoUse != _eventServers.end());

    XCEPT_RAISE(exception::DataRetrieval, "Lost connection to all SMs");
  }
  
  
  bool EventRetriever::getNextEvent(EventMsg& event)
  {
    stor::utils::sleepUntil(_nextRequestTime);

    std::string data;
    size_t tries = 0;

    while ( ! edm::shutdown_flag && data.empty() )
    {
      if ( tries == _eventServers.size() )
      {
        // all event servers have been tried
        boost::this_thread::sleep(_dataRetrieverParams._sleepTimeIfIdle);
        tries = 0;
        continue;
      }

      if ( ++_nextSMtoUse == _eventServers.end() )
        _nextSMtoUse = _eventServers.begin();

      try
      {      
        _nextSMtoUse->second->getEventMaybe(data);
        ++tries;
      }
      catch (cms::Exception& e)
      {
        // SM is no longer responding
        disconnectFromCurrentSM();
        tries = 0;
      }
    }

    if ( edm::shutdown_flag ) return false;

    HeaderView headerView(&data[0]);
    if (headerView.code() == Header::DONE) return false;
    
    event = EventMsg( EventMsgView(&data[0]) );
    _dataRetrieverMonitorCollection.addRetrievedSample(
      _nextSMtoUse->first, event.totalDataSize()
    );

    if ( ! _minEventRequestInterval.is_not_a_date_time() )
      _nextRequestTime = stor::utils::getCurrentTime() + _minEventRequestInterval;

    return true;
  }


  void EventRetriever::disconnectFromCurrentSM()
  {
    // this code is not very efficient, but rarely used
    _dataRetrieverMonitorCollection.setConnectionStatus(
      _nextSMtoUse->first, DataRetrieverMonitorCollection::DISCONNECTED);
    _eventServers.erase(_nextSMtoUse);
    _nextSMtoUse = _eventServers.begin();
  }


} // namespace smproxy
  
/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
