// $Id: DataManager.cc,v 1.1.2.5 2011/01/24 14:32:56 mommsen Exp $
/// @file: DataManager.cc

#include "EventFilter/SMProxyServer/interface/DataManager.h"
#include "FWCore/Utilities/interface/UnixSignalHandlers.h"


namespace smproxy
{
  DataManager::DataManager
  (
    stor::InitMsgCollectionPtr imc,
    EventQueueCollectionPtr eqc,
    stor::DQMEventQueueCollectionPtr dqc,
    stor::RegistrationQueuePtr regQueue,
    DataRetrieverMonitorCollection& drmc
  ) :
  _initMsgCollection(imc),
  _eventQueueCollection(eqc),
  _dqmEventQueueCollection(dqc),
  _registrationQueue(regQueue),
  _dataRetrieverMonitorCollection(drmc)
  {
    _watchDogThread.reset(
      new boost::thread( boost::bind( &DataManager::checkForStaleConsumers, this) )
    );
  }

  DataManager::~DataManager()
  {
    stop();
    _watchDogThread->interrupt();
    _watchDogThread->join();
  }
  
  
  void DataManager::start(DataRetrieverParams const& drp)
  {
    _dataRetrieverParams = drp;
    edm::shutdown_flag = false;
    _thread.reset(
      new boost::thread( boost::bind( &DataManager::doIt, this) )
    );
  }
  
  
  void DataManager::stop()
  {
    // enqueue a dummy RegInfoBase to tell the thread to stop
    _registrationQueue->enq_wait( stor::RegInfoBasePtr() );
    _thread->join();

    edm::shutdown_flag = true;
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
          _dataRetrieverParams, _dataRetrieverMonitorCollection,
          eventConsumer->getPSet())
      );
      pos = _eventRetrievers.insert(pos,
        EventRetrieverMap::value_type(eventConsumer, eventRetriever));
    }

    pos->second->addConsumer( eventConsumer );
    
    return true;
  }
  
  
  bool DataManager::addDQMEventConsumer(stor::RegInfoBasePtr regInfo)
  {
    return false;
  }
  
  
  void DataManager::checkForStaleConsumers()
  {
    while (true)
    {
      boost::this_thread::sleep(boost::posix_time::seconds(1));
      stor::utils::time_point_t now = stor::utils::getCurrentTime();
      _eventQueueCollection->clearStaleQueues(now);
      _dqmEventQueueCollection->clearStaleQueues(now);
    }
  }

} // namespace smproxy
  
/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
