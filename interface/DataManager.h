// $Id: DataManager.h,v 1.1.2.3 2011/01/24 12:43:17 mommsen Exp $
/// @file: DataManager.h 

#ifndef EventFilter_SMProxyServer_DataManager_h
#define EventFilter_SMProxyServer_DataManager_h

#include "EventFilter/SMProxyServer/interface/Configuration.h"
#include "EventFilter/SMProxyServer/interface/EventRetriever.h"
#include "EventFilter/SMProxyServer/interface/EventQueueCollection.h"
#include "EventFilter/StorageManager/interface/DQMEventQueueCollection.h"
#include "EventFilter/StorageManager/interface/InitMsgCollection.h"
#include "EventFilter/StorageManager/interface/RegistrationInfoBase.h"
#include "EventFilter/StorageManager/interface/RegistrationQueue.h"
#include "EventFilter/StorageManager/interface/Utils.h"

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/thread.hpp>

#include <map>


namespace smproxy {


  /**
   * Manages the data retrieval
   *
   * $Author: mommsen $
   * $Revision: 1.1.2.3 $
   * $Date: 2011/01/24 12:43:17 $
   */
  
  class DataManager
  {
  public:

    DataManager
    (
      stor::InitMsgCollectionPtr,
      EventQueueCollectionPtr,
      stor::DQMEventQueueCollectionPtr,
      stor::RegistrationQueuePtr
    );

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

    void doIt();
    bool addEventConsumer(stor::RegInfoBasePtr);
    bool addDQMEventConsumer(stor::RegInfoBasePtr);
    void checkForStaleConsumers();

    stor::InitMsgCollectionPtr _initMsgCollection;
    EventQueueCollectionPtr _eventQueueCollection;
    stor::DQMEventQueueCollectionPtr _dqmEventQueueCollection;
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
