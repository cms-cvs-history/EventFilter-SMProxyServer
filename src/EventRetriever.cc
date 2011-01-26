// $Id: EventRetriever.cc,v 1.1.2.7 2011/01/26 11:15:10 mommsen Exp $
/// @file: EventRetriever.cc

#include "EventFilter/SMProxyServer/interface/EventRetriever.h"
#include "EventFilter/SMProxyServer/interface/Exception.h"
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
    stor::InitMsgCollectionPtr imc,
    EventQueueCollectionPtr eqc,
    DataRetrieverParams const& drp,
    DataRetrieverMonitorCollection& drmc,
    edm::ParameterSet const& pset
  ) :
  _initMsgCollection(imc),
  _eventQueueCollection(eqc),
  _dataRetrieverParams(drp),
  _dataRetrieverMonitorCollection(drmc),
  _pset(pset),
  _minEventRequestInterval(boost::posix_time::not_a_date_time),
  _connected(false),
  _instance(++_retrieverCount)
  {
    std::ostringstream consumerName;
    consumerName << "SMPS_" << drp._smpsInstance;
    _pset.addUntrackedParameter<std::string>("consumerName", consumerName.str());

    _nextRequestTime = stor::utils::getCurrentTime();

    _thread.reset(
      new boost::thread( boost::bind( &EventRetriever::doIt, this) )
    );
  }
  
  
  EventRetriever::~EventRetriever()
  {
    stop();
  }
  
  
  void EventRetriever::addConsumer(stor::EventConsRegPtr consumer)
  {
    stor::utils::duration_t newMinEventRequestInterval =
      consumer->minEventRequestInterval();

    // Only update the _minEventRequestInterval if the new
    // consumer specifies a shorter interval than any other
    // consumer. If there's already a consumer not specifying
    // an update interval, leave it unspecified.
    if ( ! newMinEventRequestInterval.is_not_a_date_time() &&
      ! _minEventRequestInterval.is_not_a_date_time() &&
      newMinEventRequestInterval < _minEventRequestInterval )
    {
      // correct next request time for the shorter interval
      _nextRequestTime -= (_minEventRequestInterval - newMinEventRequestInterval);
      _minEventRequestInterval = newMinEventRequestInterval;
    }

    boost::mutex::scoped_lock sl(_queueIDsLock);
    _queueIDs.push_back(consumer->queueId());
  }
  
  
  void EventRetriever::stop()
  {
    _thread->interrupt();
    _thread->join();
    
    boost::mutex::scoped_lock sl(_queueIDsLock);
    _queueIDs.clear();
  }
  
  
  void EventRetriever::doIt()
  {
    connect();

    getInitMsg();

    while ( !edm::shutdown_flag )
    {
      if ( anyActiveConsumers() )
      {
        EventMsg event;
        if ( ! getNextEvent(event) ) break;
        
        {
          boost::mutex::scoped_lock sl(_queueIDsLock);
          event.tagForEventConsumers(_queueIDs);
        }
        
        _eventQueueCollection->addEvent(event);
      }
      else
      {
        boost::this_thread::sleep(_dataRetrieverParams._sleepTimeIfIdle);
      }
    }
  }
  
  
  bool EventRetriever::anyActiveConsumers()
  {
    boost::mutex::scoped_lock sl(_queueIDsLock);
    stor::utils::time_point_t now = stor::utils::getCurrentTime();
    
    for ( QueueIDs::const_iterator it = _queueIDs.begin(), itEnd = _queueIDs.end();
          it != itEnd; ++it)
    {
      if ( ! _eventQueueCollection->stale(*it, now) ) return true;
    }
    
    return false;
  }
  
  
  bool EventRetriever::connect()
  {
    size_t smCount = _dataRetrieverParams._smRegistrationList.size();
    _eventServers.clear();
    _eventServers.reserve(smCount);

    for (size_t i = 0; i < smCount; ++i)
    { 
      if (edm::shutdown_flag) continue;
      
      // each event retriever shall start from a different SM
      size_t smInstance = (_instance + i) % smCount;
      _pset.addParameter<std::string>("sourceURL",
        _dataRetrieverParams._smRegistrationList[smInstance]);

      try
      {
        EventServerPtr eventServerPtr(new stor::EventServerProxy(_pset));
        _eventServers.push_back(eventServerPtr);
        _connected = true;
      }
      catch (cms::Exception& e)
      {
        std::ostringstream errorMsg;
        errorMsg << "Failed to connect to SM on " << 
          _dataRetrieverParams._smRegistrationList[smInstance];

        if ( _dataRetrieverParams._allowMissingSM )
        {
          edm::LogInfo("EventRetriever") << errorMsg;
        }
        else
        {
          XCEPT_RAISE(exception::DataRetrieval, errorMsg.str());
        }
      }
    }
    // start with the first SM in our list
    _nextSMtoUse = _eventServers.begin();

    size_t connectedSM = _eventServers.size();
    _dataRetrieverMonitorCollection.getEventConnectionCountMQ().addSample(connectedSM);

    return (connectedSM == smCount);
  }
  
  
  void EventRetriever::getInitMsg() const
  {
    std::string data;
    (*_nextSMtoUse)->getInitMsg(data);

    InitMsgView initMsgView(&data[0]);
    _initMsgCollection->addIfUnique(initMsgView);
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
      }

      if ( ++_nextSMtoUse == _eventServers.end() )
        _nextSMtoUse = _eventServers.begin();
      
      (*_nextSMtoUse)->getEventMaybe(data);
      ++tries;
    }

    if ( edm::shutdown_flag ) return false;

    HeaderView headerView(&data[0]);
    if (headerView.code() == Header::DONE) return false;
    
    event = EventMsg( EventMsgView(&data[0]) );
    _dataRetrieverMonitorCollection.getEventSizeMQ().addSample(
      static_cast<double>( event.totalDataSize() ) / 0x100000 );

    if ( ! _minEventRequestInterval.is_not_a_date_time() )
      _nextRequestTime = stor::utils::getCurrentTime() + _minEventRequestInterval;

    return true;
  }

} // namespace smproxy
  
/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
