// $Id: DataManager.h,v 1.1.2.5 2011/01/26 16:06:54 mommsen Exp $
/// @file: DataManager.h 

#ifndef EventFilter_SMProxyServer_DataManager_h
#define EventFilter_SMProxyServer_DataManager_h

#include "EventFilter/SMProxyServer/interface/Configuration.h"
#include "EventFilter/SMProxyServer/interface/DataRetrieverMonitorCollection.h"
#include "EventFilter/SMProxyServer/interface/EventRetriever.h"
#include "EventFilter/StorageManager/interface/EventConsumerRegistrationInfo.h"
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
   * $Revision: 1.1.2.5 $
   * $Date: 2011/01/26 16:06:54 $
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


  private:

    void activity();
    void doIt();
    bool addEventConsumer(stor::RegInfoBasePtr);
    bool addDQMEventConsumer(stor::RegInfoBasePtr);
    void watchDog();
    void checkForStaleConsumers();

    StateMachine* _stateMachine;
    stor::RegistrationQueuePtr _registrationQueue;
    DataRetrieverParams _dataRetrieverParams;

    boost::scoped_ptr<boost::thread> _thread;
    boost::scoped_ptr<boost::thread> _watchDogThread;

    typedef boost::shared_ptr<EventRetriever> EventRetrieverPtr;
    typedef std::map<stor::EventConsRegPtr, EventRetrieverPtr,
                     stor::utils::ptr_comp<stor::EventConsumerRegistrationInfo> > EventRetrieverMap;
    EventRetrieverMap _eventRetrievers;

  };
  
} // namespace smproxy

#endif // EventFilter_SMProxyServer_DataManager_h 


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
