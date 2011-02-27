// $Id: DataManager.cc,v 1.1.2.10 2011/02/26 09:17:26 mommsen Exp $
/// @file: DataManager.cc

#include "EventFilter/SMProxyServer/interface/Exception.h"
#include "EventFilter/SMProxyServer/interface/DataManager.h"
#include "EventFilter/SMProxyServer/interface/StateMachine.h"
#include "EventFilter/SMProxyServer/src/EventRetriever.icc"
#include "FWCore/Utilities/interface/UnixSignalHandlers.h"

#include <boost/pointer_cast.hpp>


namespace smproxy
{
  DataManager::DataManager
  (
    StateMachine* stateMachine
  ) :
  _stateMachine(stateMachine),
  _registrationQueue(stateMachine->getRegistrationQueue())
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
    // enqueue a dummy RegistrationInfoBase to tell the thread to stop
    _registrationQueue->enq_wait( stor::RegPtr() );
    _thread->join();

    edm::shutdown_flag = true;
    _dataEventRetrievers.clear();
  }
  
  
  bool DataManager::getQueueIDsFromDataEventRetrievers
  (
    stor::EventConsRegPtr eventConsumer,
    stor::QueueIDs& queueIDs
  ) const
  {
    if ( ! eventConsumer ) return false;
    
    DataEventRetrieverMap::const_iterator pos =
      _dataEventRetrievers.find(eventConsumer);
    if ( pos == _dataEventRetrievers.end() ) return false;
    
    queueIDs = pos->second->getQueueIDs();
    return true;
  }
  
  
  bool DataManager::getQueueIDsFromDQMEventRetrievers
  (
    stor::DQMEventConsRegPtr dqmEventConsumer,
    stor::QueueIDs& queueIDs
  ) const
  {
    if ( ! dqmEventConsumer ) return false;
    
    DQMEventRetrieverMap::const_iterator pos =
      _dqmEventRetrievers.find(dqmEventConsumer);
    if ( pos == _dqmEventRetrievers.end() ) return false;
    
    queueIDs = pos->second->getQueueIDs();
    return true;
  }
  
  
  void DataManager::activity()
  {
    try
    {
      doIt();
    }
    catch(xcept::Exception &e)
    {
      _stateMachine->moveToFailedState(e);
    }
    catch(std::exception &e)
    {
      XCEPT_DECLARE(exception::Exception,
        sentinelException, e.what());
      _stateMachine->moveToFailedState(sentinelException);
    }
    catch(...)
    {
      std::string errorMsg = "Unknown exception in watch dog";
      XCEPT_DECLARE(exception::Exception,
        sentinelException, errorMsg);
      _stateMachine->moveToFailedState(sentinelException);
    }
  }
  
  
  void DataManager::doIt()
  {
    stor::RegPtr regPtr;
    bool process(true);

    do
    {
      _registrationQueue->deq_wait(regPtr);

      if ( ! (addEventConsumer(regPtr) || addDQMEventConsumer(regPtr)) )
      {
        // base type received, signalling the end of the run
        process = false;
      }
    } while (process);
  }
  
  
  bool DataManager::addEventConsumer(stor::RegPtr regPtr)
  {
    stor::EventConsRegPtr eventConsumer =
      boost::dynamic_pointer_cast<stor::EventConsumerRegistrationInfo>(regPtr);
    
    if ( ! eventConsumer ) return false;

    DataEventRetrieverMap::iterator pos = _dataEventRetrievers.lower_bound(eventConsumer);
    if ( pos == _dataEventRetrievers.end() || (_dataEventRetrievers.key_comp()(eventConsumer, pos->first)) )
    {
      // no retriever found for this event requests
      DataEventRetrieverPtr dataEventRetriever(
        new DataEventRetriever(_stateMachine, eventConsumer)
      );
      _dataEventRetrievers.insert(pos,
        DataEventRetrieverMap::value_type(eventConsumer, dataEventRetriever));
    }
    else
    {
      pos->second->addConsumer( eventConsumer );
    }

    return true;
  }
  
  
  bool DataManager::addDQMEventConsumer(stor::RegPtr regPtr)
  {
    stor::DQMEventConsRegPtr dqmEventConsumer =
      boost::dynamic_pointer_cast<stor::DQMEventConsumerRegistrationInfo>(regPtr);
    
    if ( ! dqmEventConsumer ) return false;

    DQMEventRetrieverMap::iterator pos = _dqmEventRetrievers.lower_bound(dqmEventConsumer);
    if ( pos == _dqmEventRetrievers.end() || (_dqmEventRetrievers.key_comp()(dqmEventConsumer, pos->first)) )
    {
      // no retriever found for this DQM event requests
      DQMEventRetrieverPtr dqmEventRetriever(
        new DQMEventRetriever(_stateMachine, dqmEventConsumer)
      );
      _dqmEventRetrievers.insert(pos,
        DQMEventRetrieverMap::value_type(dqmEventConsumer, dqmEventRetriever));
    }
    else
    {
      pos->second->addConsumer( dqmEventConsumer );
    }
    
    return true;
  }
  
  
  void DataManager::watchDog()
  {
    try
    {
      checkForStaleConsumers();
    }
    catch(boost::thread_interrupted)
    {
      // thread was interrupted.
    }
    catch(xcept::Exception &e)
    {
      _stateMachine->moveToFailedState(e);
    }
    catch(std::exception &e)
    {
      XCEPT_DECLARE(exception::Exception,
        sentinelException, e.what());
      _stateMachine->moveToFailedState(sentinelException);
    }
    catch(...)
    {
      std::string errorMsg = "Unknown exception in watch dog";
      XCEPT_DECLARE(exception::Exception,
        sentinelException, errorMsg);
      _stateMachine->moveToFailedState(sentinelException);
    }
  }
  
  
  void DataManager::checkForStaleConsumers()
  {
    EventQueueCollectionPtr eventQueueCollection =
      _stateMachine->getEventQueueCollection();
    stor::DQMEventQueueCollectionPtr dqmEventQueueCollection =
      _stateMachine->getDQMEventQueueCollection();
    
    while (true)
    {
      boost::this_thread::sleep(boost::posix_time::seconds(1));
      stor::utils::time_point_t now = stor::utils::getCurrentTime();
      eventQueueCollection->clearStaleQueues(now);
      dqmEventQueueCollection->clearStaleQueues(now);
    }
  }

} // namespace smproxy
  
/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
