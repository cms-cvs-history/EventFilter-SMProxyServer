// $Id: DataManager.h,v 1.1.2.7 2011/02/11 12:13:44 mommsen Exp $
/// @file: DataManager.h 

#ifndef EventFilter_SMProxyServer_DataManager_h
#define EventFilter_SMProxyServer_DataManager_h

#include "EventFilter/SMProxyServer/interface/Configuration.h"
#include "EventFilter/SMProxyServer/interface/DataRetrieverMonitorCollection.h"
#include "EventFilter/SMProxyServer/interface/EventRetriever.h"
#include "EventFilter/StorageManager/interface/DQMEventConsumerRegistrationInfo.h"
#include "EventFilter/StorageManager/interface/DQMEventQueueCollection.h"
#include "EventFilter/StorageManager/interface/EventConsumerRegistrationInfo.h"
#include "EventFilter/StorageManager/interface/EventQueueCollection.h"
#include "EventFilter/StorageManager/interface/RegistrationInfoBase.h"
#include "EventFilter/StorageManager/interface/RegistrationQueue.h"

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/thread.hpp>

#include <map>


namespace smproxy {

  class StateMachine;

  /**
   * Manages the data retrieval
   *
   * $Author: mommsen $
   * $Revision: 1.1.2.7 $
   * $Date: 2011/02/11 12:13:44 $
   */
  
  class DataManager
  {
  public:

    DataManager(StateMachine*);

    ~DataManager();

    /**
     * Start retrieving data
     */
    void start(DataRetrieverParams const&);

    /**
     * Stop retrieving data
     */
    void stop();

    /**
     * Get list of data event consumer queueIDs for given event type.
     * Returns false if the event type is not found.
     */
    bool getQueueIDsFromDataEventRetrievers
    (
      stor::EventConsRegPtr,
      stor::QueueIDs&
    ) const;

    /**
     * Get list of DQM event consumer queueIDs for given event type.
     * Returns false if the event type is not found.
     */
    bool getQueueIDsFromDQMEventRetrievers
    (
      stor::DQMEventConsRegPtr,
      stor::QueueIDs&
    ) const;


  private:

    void activity();
    void doIt();
    bool addEventConsumer(stor::RegPtr);
    bool addDQMEventConsumer(stor::RegPtr);
    void watchDog();
    void checkForStaleConsumers();

    StateMachine* _stateMachine;
    stor::RegistrationQueuePtr _registrationQueue;
    DataRetrieverParams _dataRetrieverParams;

    boost::scoped_ptr<boost::thread> _thread;
    boost::scoped_ptr<boost::thread> _watchDogThread;

    typedef EventRetriever<stor::EventConsumerRegistrationInfo,
                           EventQueueCollectionPtr> DataEventRetriever;
    typedef boost::shared_ptr<DataEventRetriever> DataEventRetrieverPtr;
    typedef std::map<stor::EventConsRegPtr, DataEventRetrieverPtr,
                     stor::utils::ptr_comp<stor::EventConsumerRegistrationInfo> > DataEventRetrieverMap;
    DataEventRetrieverMap _dataEventRetrievers;

    typedef EventRetriever<stor::DQMEventConsumerRegistrationInfo,
                           stor::DQMEventQueueCollectionPtr> DQMEventRetriever;
    typedef boost::shared_ptr<DQMEventRetriever> DQMEventRetrieverPtr;
    typedef std::map<stor::DQMEventConsRegPtr, DQMEventRetrieverPtr,
                     stor::utils::ptr_comp<stor::DQMEventConsumerRegistrationInfo> > DQMEventRetrieverMap;
    DQMEventRetrieverMap _dqmEventRetrievers;

  };

  typedef boost::shared_ptr<DataManager> DataManagerPtr;
  
} // namespace smproxy

#endif // EventFilter_SMProxyServer_DataManager_h 


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
