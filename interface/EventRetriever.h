// $Id: EventRetriever.h,v 1.1.2.10 2011/02/11 12:13:44 mommsen Exp $
/// @file: EventRetriever.h 

#ifndef EventFilter_SMProxyServer_EventRetriever_h
#define EventFilter_SMProxyServer_EventRetriever_h

#include "EventFilter/SMProxyServer/interface/Configuration.h"
#include "EventFilter/SMProxyServer/interface/ConnectionID.h"
#include "EventFilter/SMProxyServer/interface/DataRetrieverMonitorCollection.h"
#include "EventFilter/SMProxyServer/interface/EventQueueCollection.h"
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
   * $Revision: 1.1.2.10 $
   * $Date: 2011/02/11 12:13:44 $
   */
  
  class EventRetriever
  {
  public:

    EventRetriever
    (
      StateMachine*,
      const stor::EventConsRegPtr
    );

    ~EventRetriever();

    /**
     * Add a consumer
     */
    void addConsumer(const stor::EventConsRegPtr);

    /**
     * Stop retrieving events
     */
    void stop();

    /**
     * Return the list of QueueIDs attached to the EventRetriever
     */
    const std::vector<stor::QueueID>& getQueueIDs() const
    { return _queueIDs; }

 
  private:

    void activity(const edm::ParameterSet&);
    void doIt(const edm::ParameterSet&);
    bool connect(const edm::ParameterSet&);
    void connectToSM(const std::string& sourceURL, const edm::ParameterSet&);
    bool openConnection(const ConnectionID&, const stor::EventConsRegPtr);
    bool tryToReconnect();
    void getInitMsg();
    bool getNextEvent(EventMsg&);
    bool adjustMinEventRequestInterval(const stor::utils::duration_t&);
    void updateConsumersSetting(const stor::utils::duration_t&);
    bool anyActiveConsumers(EventQueueCollectionPtr) const;
    void disconnectFromCurrentSM();
    
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

    typedef boost::shared_ptr<stor::EventServerProxy> EventServerPtr;
    typedef std::map<ConnectionID, EventServerPtr> EventServers;
    EventServers _eventServers;
    EventServers::iterator _nextSMtoUse;

    typedef std::vector<ConnectionID> ConnectionIDs;
    ConnectionIDs _connectionIDs;
    mutable boost::mutex _connectionIDsLock;

    stor::utils::time_point_t _nextReconnectTry;

    typedef std::vector<stor::QueueID> QueueIDs;
    QueueIDs _queueIDs;
    mutable boost::mutex _queueIDsLock;
  };
  
} // namespace smproxy

#endif // EventFilter_SMProxyServer_EventRetriever_h 


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
