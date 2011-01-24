// $Id: EventRetriever.h,v 1.1.2.3 2011/01/24 12:43:17 mommsen Exp $
/// @file: EventRetriever.h 

#ifndef EventFilter_SMProxyServer_EventRetriever_h
#define EventFilter_SMProxyServer_EventRetriever_h

#include "EventFilter/SMProxyServer/interface/Configuration.h"
#include "EventFilter/SMProxyServer/interface/EventQueueCollection.h"
#include "EventFilter/StorageManager/interface/EventServerProxy.h"
#include "EventFilter/StorageManager/interface/InitMsgCollection.h"
#include "EventFilter/StorageManager/interface/QueueID.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/thread.hpp>

#include <vector>



namespace smproxy {


  /**
   * Retrieve events from the event server
   *
   * $Author: mommsen $
   * $Revision: 1.1.2.3 $
   * $Date: 2011/01/24 12:43:17 $
   */
  
  class EventRetriever
  {
  public:


    EventRetriever
    (
      stor::InitMsgCollectionPtr,
      EventQueueCollectionPtr,
      DataRetrieverParams const&,
      edm::ParameterSet const&
    );

    ~EventRetriever();

    /**
     * Add consumer queue
     */
    void addQueue(const stor::QueueID&);

    /**
     * Stop retrieving events
     */
    void stop();

  private:

    void doIt();
    bool connect();
    void getInitMsg() const;
    bool getNextEvent(EventMsg&);
    bool anyActiveConsumers();

    //Prevent copying of the EventRetriever
    EventRetriever(EventRetriever const&);
    EventRetriever& operator=(EventRetriever const&);

    stor::InitMsgCollectionPtr _initMsgCollection;
    EventQueueCollectionPtr _eventQueueCollection;
    const DataRetrieverParams _dataRetrieverParams;
    edm::ParameterSet _pset;

    boost::scoped_ptr<boost::thread> _thread;
    bool _process;
    bool _connected;
    static size_t _retrieverCount;
    size_t _instance;

    typedef boost::shared_ptr<stor::EventServerProxy> EventServerPtr;
    typedef std::vector<EventServerPtr> EventServers;
    EventServers _eventServers;
    EventServers::const_iterator _nextSMtoUse;

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
