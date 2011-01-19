// $Id: DataManager.cc,v 1.1.2.1 2011/01/18 15:32:34 mommsen Exp $
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
  _dataRetrieverParams(drp)
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
    // enqueue a dummy RegInfoBase to tell the thread to stop
    _registrationQueue->enq_wait( stor::RegInfoBasePtr() );
    _thread->join();

    _eventRetrievers.clear();
  }


  void DataManager::doIt()
  {
    stor::RegInfoBasePtr regInfo;
    bool process(true);

    do
    {
      _registrationQueue->deq_wait(regInfo);

      if ( ! (addEventConsumer(regInfo) || addDQMEventConsumer(regInfo)) )
      {
        // base type received, signalling the end of the run
        process = false;
      }
    } while (process);
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
