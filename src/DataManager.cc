// $Id: DataManager.cc,v 1.1.2.7 2011/01/27 14:55:54 mommsen Exp $
/// @file: DataManager.cc

#include "EventFilter/SMProxyServer/interface/Exception.h"
#include "EventFilter/SMProxyServer/interface/DataManager.h"
#include "EventFilter/SMProxyServer/interface/StateMachine.h"
#include "FWCore/Utilities/interface/UnixSignalHandlers.h"


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
    // enqueue a dummy RegInfoBase to tell the thread to stop
    _registrationQueue->enq_wait( stor::RegInfoBasePtr() );
    _thread->join();

    edm::shutdown_flag = true;
    _eventRetrievers.clear();
  }
  
  
  void DataManager::activity()
  {
    try
    {
      doIt();
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
      std::string errorMsg = "Unknown exception in watch dog";
      XCEPT_DECLARE(exception::Exception,
        sentinelException, errorMsg);
      _stateMachine->processEvent( Fail(sentinelException) );
    }
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
        new EventRetriever(_stateMachine, eventConsumer)
      );
      _eventRetrievers.insert(pos,
        EventRetrieverMap::value_type(eventConsumer, eventRetriever));
    }
    else
    {
      pos->second->addConsumer( eventConsumer );
    }

    return true;
  }
  
  
  bool DataManager::addDQMEventConsumer(stor::RegInfoBasePtr regInfo)
  {
    return false;
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
      std::string errorMsg = "Unknown exception in watch dog";
      XCEPT_DECLARE(exception::Exception,
        sentinelException, errorMsg);
      _stateMachine->processEvent( Fail(sentinelException) );
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
