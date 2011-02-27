// $Id: EventRetriever.h,v 1.1.2.13 2011/02/26 09:17:26 mommsen Exp $
/// @file: EventRetriever.h 

#ifndef EventFilter_SMProxyServer_EventRetriever_h
#define EventFilter_SMProxyServer_EventRetriever_h

#include "EventFilter/SMProxyServer/interface/Configuration.h"
#include "EventFilter/SMProxyServer/interface/ConnectionID.h"
#include "EventFilter/SMProxyServer/interface/DataRetrieverMonitorCollection.h"
#include "EventFilter/SMProxyServer/interface/DQMEventMsg.h"
#include "EventFilter/SMProxyServer/interface/EventQueueCollection.h"
#include "EventFilter/StorageManager/interface/DQMEventStore.h"
#include "EventFilter/StorageManager/interface/EventServerProxy.h"
#include "EventFilter/StorageManager/interface/EventConsumerRegistrationInfo.h"
#include "EventFilter/StorageManager/interface/QueueID.h"
#include "EventFilter/StorageManager/interface/Utils.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/thread.hpp>

#include <string>
#include <vector>



namespace smproxy {

  class StateMachine;


  /**
   * Retrieve events from the event server
   *
   * $Author: mommsen $
   * $Revision: 1.1.2.13 $
   * $Date: 2011/02/26 09:17:26 $
   */

  template<class RegInfo, class QueueCollectionPtr> 
  class EventRetriever
  {
  public:

    typedef boost::shared_ptr<RegInfo> RegInfoPtr;

    EventRetriever
    (
      StateMachine*,
      const RegInfoPtr
    );

    ~EventRetriever();

    /**
     * Add a consumer
     */
    void addConsumer(const RegInfoPtr);

    /**
     * Stop retrieving events
     */
    void stop();

    /**
     * Return the list of QueueIDs attached to the EventRetriever
     */
    const stor::QueueIDs& getQueueIDs() const
    { return _queueIDs; }

    /**
     * Return the number of active connections to SMs
     */
    size_t getConnectedSMCount() const
    { return _eventServers.size(); }

 
  private:

    void activity(const edm::ParameterSet&);
    void doIt(const edm::ParameterSet&);
    void do_stop();
    bool connect(const edm::ParameterSet&);
    void connectToSM(const std::string& sourceURL, const edm::ParameterSet&);
    bool openConnection(const ConnectionID&, const RegInfoPtr);
    bool tryToReconnect();
    void getInitMsg();
    bool getNextEvent(stor::CurlInterface::Content&);
    bool adjustMinEventRequestInterval(const stor::utils::duration_t&);
    void updateConsumersSetting(const stor::utils::duration_t&);
    bool anyActiveConsumers(QueueCollectionPtr) const;
    void disconnectFromCurrentSM();
    void processCompletedTopLevelFolders();
    
    //Prevent copying of the EventRetriever
    EventRetriever(EventRetriever const&);
    EventRetriever& operator=(EventRetriever const&);

    StateMachine* _stateMachine;
    const DataRetrieverParams _dataRetrieverParams;
    DataRetrieverMonitorCollection& _dataRetrieverMonitorCollection;

    stor::utils::time_point_t _nextRequestTime;
    stor::utils::duration_t _minEventRequestInterval;

    boost::scoped_ptr<boost::thread> _thread;
    static size_t _retrieverCount;
    size_t _instance;

    typedef stor::EventServerProxy<RegInfo> EventServer;
    typedef boost::shared_ptr<EventServer> EventServerPtr;
    typedef std::map<ConnectionID, EventServerPtr> EventServers;
    EventServers _eventServers;
    typename EventServers::iterator _nextSMtoUse;

    typedef std::vector<ConnectionID> ConnectionIDs;
    ConnectionIDs _connectionIDs;
    mutable boost::mutex _connectionIDsLock;

    stor::utils::time_point_t _nextReconnectTry;

    stor::QueueIDs _queueIDs;
    mutable boost::mutex _queueIDsLock;

    stor::DQMEventStore<DQMEventMsg,
                        EventRetriever<RegInfo,QueueCollectionPtr>
                        > _dqmEventStore;
  
  };
  
} // namespace smproxy

#endif // EventFilter_SMProxyServer_EventRetriever_h 


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
