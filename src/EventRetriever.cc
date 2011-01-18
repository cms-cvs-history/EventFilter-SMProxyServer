// $Id: EventRetriever.cc,v 1.25 2010/12/10 19:38:48 mommsen Exp $
/// @file: EventRetriever.cc

#include "EventFilter/SMProxyServer/interface/EventRetriever.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "FWCore/Utilities/interface/Exception.h"
#include "FWCore/Utilities/interface/UnixSignalHandlers.h"


namespace smproxy
{
  size_t EventRetriever::_retrieverCount(0);


  EventRetriever::EventRetriever
  (
    boost::shared_ptr<stor::InitMsgCollection> imc,
    boost::shared_ptr<EventQueueCollection> eqc,
    DataRetrieverParams const& drp,
    edm::ParameterSet const& pset
  ) :
  _initMsgCollection(imc),
  _eventQueueCollection(eqc),
  _dataRetrieverParams(drp),
  _pset(pset),
  _connected(false),
  _instance(++_retrieverCount)
  {
    _thread.reset(
      new boost::thread( boost::bind( &EventRetriever::doIt, this) )
    );
  }
  
  
  EventRetriever::~EventRetriever()
  {
    stop();
  }
  
  
  void EventRetriever::addQueue(const stor::QueueID& qid)
  {
    boost::mutex::scoped_lock sl(_queueIDsLock);
    _queueIDs.push_back(qid);
  }
  
  
  void EventRetriever::stop()
  {
    edm::shutdown_flag = true;
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
      if ( _queueIDs.empty() )
      {
        sleep(1);
      }
      else
      {
        EventMsg event;
        if ( ! getNextEvent(event) ) break;

        {
          boost::mutex::scoped_lock sl(_queueIDsLock);
          event.tagForEventConsumers(_queueIDs);
        }

        _eventQueueCollection->addEvent(event);
      }
    }
  }
  
  
  bool EventRetriever::connect()
  {
    size_t smCount = _dataRetrieverParams._smRegistrationList.size();
    _eventServers.clear();
    _eventServers.reserve(smCount);

    for (size_t i = 0; i < smCount; ++i)
    { 
      if (edm::shutdown_flag) continue;
      
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
        edm::LogInfo("EventRetriever") << "Failed to connect to SM on " << 
          _dataRetrieverParams._smRegistrationList[smInstance];
      }
    }
    _nextSMtoUse = _eventServers.begin();

    return (_eventServers.size() == smCount);
  }
  
  
  void EventRetriever::getInitMsg() const
  {
    std::string data;
    (*_nextSMtoUse)->getInitMsg(data);

    InitMsgView initMsgView(&data[0]);
    _initMsgCollection->addIfUnique(initMsgView);
  }
  
  
  bool EventRetriever::getNextEvent(EventMsg event)
  {
    std::string data;

    while ( ! (*(++_nextSMtoUse))->getEventMaybe(data) ) {}
    // How to throttle the request rate if no SM has data?

    EventMsgView eventMsgView(&data[0]);
    if (eventMsgView.code() == Header::DONE) return false;
    
    event = EventMsg(eventMsgView);

    return true;
  }

} // namespace smproxy
  
/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
