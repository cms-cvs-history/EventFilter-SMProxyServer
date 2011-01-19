// $Id: DataManager.h,v 1.1.2.1 2011/01/18 15:32:34 mommsen Exp $
/// @file: DataManager.h 

#ifndef SMProxyServer_DataManager_h
#define SMProxyServer_DataManager_h

#include "EventFilter/SMProxyServer/interface/Configuration.h"
#include "EventFilter/SMProxyServer/interface/EventRetriever.h"
#include "EventFilter/SMProxyServer/interface/EventQueueCollection.h"
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
   * $Revision: 1.1.2.1 $
   * $Date: 2011/01/18 15:32:34 $
   */
  
  class DataManager
  {
  public:

    DataManager
    (
      boost::shared_ptr<stor::InitMsgCollection>,
      boost::shared_ptr<EventQueueCollection>,
      boost::shared_ptr<stor::RegistrationQueue>,
      DataRetrieverParams const&
    );

    ~DataManager();

    /**
     * Stop retrieving data
     */
    void stop();


  private:

    void doIt();
    bool addEventConsumer(stor::RegInfoBasePtr);
    bool addDQMEventConsumer(stor::RegInfoBasePtr);

    boost::shared_ptr<stor::InitMsgCollection> _initMsgCollection;
    boost::shared_ptr<EventQueueCollection> _eventQueueCollection;
    boost::shared_ptr<stor::RegistrationQueue> _registrationQueue;
    const DataRetrieverParams _dataRetrieverParams;

    boost::scoped_ptr<boost::thread> _thread;

    typedef boost::shared_ptr<EventRetriever> EventRetrieverPtr;
    typedef std::map<stor::EventConsRegPtr, EventRetrieverPtr,
                     stor::utils::ptr_comp<stor::EventConsumerRegistrationInfo> > EventRetrieverMap;
    EventRetrieverMap _eventRetrievers;

  };
  
} // namespace smproxy

#endif // SMProxyServer_DataManager_h 


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
