// $Id: EventRetriever.h,v 1.1.2.6 2011/01/27 14:55:54 mommsen Exp $
/// @file: EventRetriever.h 

#ifndef EventFilter_SMProxyServer_EventRetriever_h
#define EventFilter_SMProxyServer_EventRetriever_h

#include "EventFilter/SMProxyServer/interface/Configuration.h"
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
   * $Revision: 1.1.2.6 $
   * $Date: 2011/01/27 14:55:54 $
   */
  
  class EventRetriever
  {
  public:

    EventRetriever
    (
      StateMachine*,
      edm::ParameterSet const&
    );

    ~EventRetriever();

    /**
     * Add a consumer
     */
    void addConsumer(stor::EventConsRegPtr);

    /**
     * Stop retrieving events
     */
    void stop();

  private:

    void activity();
    void doIt();
    bool connect();
    void connectToSM(const std::string& sourceURL);
    void getInitMsg();
    bool getNextEvent(EventMsg&);
    bool anyActiveConsumers(EventQueueCollectionPtr) const;
    void disconnectFromCurrentSM();
    
    //Prevent copying of the EventRetriever
    EventRetriever(EventRetriever const&);
    EventRetriever& operator=(EventRetriever const&);

    StateMachine* _stateMachine;
    edm::ParameterSet _pset;
    const DataRetrieverParams _dataRetrieverParams;
    DataRetrieverMonitorCollection& _dataRetrieverMonitorCollection;

    stor::utils::time_point_t _nextRequestTime;
    stor::utils::duration_t _minEventRequestInterval;

    boost::scoped_ptr<boost::thread> _thread;
    static size_t _retrieverCount;
    size_t _instance;

    typedef boost::shared_ptr<stor::EventServerProxy> EventServerPtr;
    typedef std::vector<EventServerPtr> EventServers;
    EventServers _eventServers;
    EventServers::iterator _nextSMtoUse;

    typedef std::map<std::string,bool> ConnectedSMs;
    ConnectedSMs _connectedSMs;

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
