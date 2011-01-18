// $Id: EventRetriever.h,v 1.12 2010/12/10 19:38:48 mommsen Exp $
/// @file: EventRetriever.h 

#ifndef SMProxyServer_EventRetriever_h
#define SMProxyServer_EventRetriever_h

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
   * $Revision: 1.12 $
   * $Date: 2010/12/10 19:38:48 $
   */
  
  class EventRetriever
  {
  public:


    EventRetriever
    (
      boost::shared_ptr<stor::InitMsgCollection>,
      boost::shared_ptr<EventQueueCollection>,
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
    bool getNextEvent(EventMsg);

    //Prevent copying of the EventRetriever
    EventRetriever(EventRetriever const&);
    EventRetriever& operator=(EventRetriever const&);

    boost::shared_ptr<stor::InitMsgCollection> _initMsgCollection;
    boost::shared_ptr<EventQueueCollection> _eventQueueCollection;
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

#endif // SMProxyServer_EventRetriever_h 


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
