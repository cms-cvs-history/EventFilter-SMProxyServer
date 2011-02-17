// $Id: DataRetrieverMonitorCollection.h,v 1.1.2.4 2011/02/15 14:06:53 mommsen Exp $
/// @file: DataRetrieverMonitorCollection.h 

#ifndef EventFilter_SMProxyServer_DataRetrieverMonitorCollection_h
#define EventFilter_SMProxyServer_DataRetrieverMonitorCollection_h

#include "EventFilter/SMProxyServer/interface/ConnectionID.h"
#include "EventFilter/StorageManager/interface/EventConsumerRegistrationInfo.h"
#include "EventFilter/StorageManager/interface/MonitorCollection.h"
#include "EventFilter/StorageManager/interface/MonitoredQuantity.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"

#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>

#include <map>
#include <string>
#include <vector>


namespace smproxy {

  /**
   * A collection of MonitoredQuantities related to data retrieval
   *
   * $Author: mommsen $
   * $Revision: 1.1.2.4 $
   * $Date: 2011/02/15 14:06:53 $
   */
  
  class DataRetrieverMonitorCollection : public stor::MonitorCollection
  {
  public:

    enum ConnectionStatus { CONNECTED, CONNECTION_FAILED, DISCONNECTED, UNKNOWN };
    
    struct SummaryStats
    {
      size_t registeredSMs;
      size_t activeSMs;
      stor::MonitoredQuantity::Stats sizeStats;         //kB

      typedef std::pair<stor::EventConsRegPtr, stor::MonitoredQuantity::Stats> EventTypeStats;
      typedef std::vector<EventTypeStats> EventTypeStatList;
      EventTypeStatList eventTypeStats;
    };

    typedef std::map<std::string, stor::MonitoredQuantity::Stats> ConnectionStats;

    struct EventTypeStats
    {
      stor::EventConsRegPtr eventConsRegPtr;
      ConnectionStatus connectionStatus;
      stor::MonitoredQuantity::Stats sizeStats;         //kB

      bool operator<(const EventTypeStats&) const;
    };
    typedef std::vector<EventTypeStats> EventTypeStatList;
    
    
    explicit DataRetrieverMonitorCollection(const stor::utils::duration_t& updateInterval);
    
    /**
     * Add a new  server connection.
     * Returns an unique connection ID.
     */
    ConnectionID addNewConnection(const stor::EventConsRegPtr);

    /**
     * Set status of given connection. Returns false if the ConnectionID is unknown.
     */
    bool setConnectionStatus(const ConnectionID&, const ConnectionStatus&);

    /**
     * Put the event type statistics for the given consumer ID into
     * the passed EventTypeStats. Return false if the connection ID is not found.
     */
    bool getEventTypeStatsForConnection(const ConnectionID&, EventTypeStats&);

    /**
     * Add a retrieved  sample in Bytes from the given connection.
     * Returns false if the ConnectionID is unknown.
     */
    bool addRetrievedSample(const ConnectionID&, const unsigned int& size);
    
    /**
     * Write the data retrieval summary statistics into the given struct.
     */
    void getSummaryStats(SummaryStats&) const;

    /**
     * Write the data retrieval statistics for each connection into the given struct.
     */
    void getStatsByConnection(ConnectionStats&) const;

    /**
     * Write the data retrieval statistics for each event type request into the given struct.
     */
    void getStatsByEventTypes(EventTypeStatList&) const;
    

  private:
    
    stor::MonitoredQuantity _totalSize;

    struct DataRetrieverMQ
    {
      stor::EventConsRegPtr _eventConsRegPtr;
      ConnectionStatus _connectionStatus;
      stor::MonitoredQuantity _size;       //kB

      DataRetrieverMQ
      (
        stor::EventConsRegPtr,
        const stor::utils::duration_t& updateInterval
      );
    };

    //Prevent copying of the DataRetrieverMonitorCollection
    DataRetrieverMonitorCollection(DataRetrieverMonitorCollection const&);
    DataRetrieverMonitorCollection& operator=(DataRetrieverMonitorCollection const&);

    const stor::utils::duration_t _updateInterval;
    typedef boost::shared_ptr<DataRetrieverMQ> DataRetrieverMQPtr;
    typedef std::map<ConnectionID, DataRetrieverMQPtr> RetrieverMqMap;
    RetrieverMqMap _retrieverMqMap;

    typedef std::map<std::string, stor::MonitoredQuantityPtr> ConnectionMqMap;
    ConnectionMqMap _connectionMqMap;

    typedef std::map<stor::EventConsRegPtr, stor::MonitoredQuantityPtr,
                     stor::utils::ptr_comp<stor::EventConsumerRegistrationInfo>
                     > EventTypeMqMap;
    EventTypeMqMap _eventTypeMqMap;

    mutable boost::mutex _statsMutex;
    ConnectionID _nextConnectionId;

    virtual void do_calculateStatistics();
    virtual void do_reset();
    // virtual void do_appendInfoSpaceItems(InfoSpaceItems&);
    // virtual void do_updateInfoSpaceItems();

  };

  std::ostream& operator<<(std::ostream&, const DataRetrieverMonitorCollection::ConnectionStatus&);

  
} // namespace smproxy

#endif // EventFilter_SMProxyServer_DataRetrieverMonitorCollection_h 


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
