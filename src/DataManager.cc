// $Id: DataManager.cc,v 1.25 2010/12/10 19:38:48 mommsen Exp $
/// @file: DataManager.cc

#include "EventFilter/SMProxyServer/interface/DataManager.h"


namespace smproxy
{
  DataManager::DataManager
  (
    boost::shared_ptr<stor::InitMsgCollection> imc,
    boost::shared_ptr<EventQueueCollection> eqc,
    boost::shared_ptr<stor::RegistrationQueue> regQueue,
    DataRetrieverParams const& drp
  ) :
  _initMsgCollection(imc),
  _eventQueueCollection(eqc),
  _registrationQueue(regQueue),
  _dataRetrieverParams(drp),
  _process(true)
  {
    _thread.reset(
      new boost::thread( boost::bind( &DataManager::doIt, this) )
    );
  }

  DataManager::~DataManager()
  {
    stop();
  }
  
  
  void DataManager::stop()
  {
    _process = false;
    _thread->join();

    _eventRetrievers.clear();
  }


  void DataManager::doIt()
  {
    stor::RegInfoBasePtr regInfo;

    while (_process) {

      if ( _registrationQueue->deq_timed_wait(regInfo, boost::posix_time::seconds(1)) )
      {
        if ( ! (addEventConsumer(regInfo) || addDQMEventConsumer(regInfo)) )
        {
          // unknown consumer registration received
        }
      }
    }
  }


  bool DataManager::addEventConsumer(stor::RegInfoBasePtr regInfo)
  {
    stor::EventConsRegPtr eventConsumer =
      boost::dynamic_pointer_cast<stor::EventConsumerRegistrationInfo>(regInfo);
    
    if ( ! eventConsumer ) return false;

    EventRetrieverMap::iterator pos = _eventRetrievers.lower_bound(eventConsumer);
    if ( pos == _eventRetrievers.end() || (_eventRetrievers.key_comp()(eventConsumer, pos->first)) )
    {
      // no retriever found for this event requests
      EventRetrieverPtr eventRetriever(
        new EventRetriever(_initMsgCollection, _eventQueueCollection,
          _dataRetrieverParams, eventConsumer->getPSet())
      );
      _eventRetrievers.insert(pos, EventRetrieverMap::value_type(eventConsumer, eventRetriever));
    }
    else
    {
      // retriever already available
      pos->second->addQueue( eventConsumer->queueId() );
    }
    
    return true;
  }
  
  
  bool DataManager::addDQMEventConsumer(stor::RegInfoBasePtr regInfo)
  {
    return false;
  }

} // namespace smproxy
  
/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
