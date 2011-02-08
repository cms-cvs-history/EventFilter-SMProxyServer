// $Id: DataRetrieverMonitorCollection.h,v 1.1.2.1 2011/01/26 16:06:54 mommsen Exp $
/// @file: DataRetrieverMonitorCollection.h 

#ifndef EventFilter_SMProxyServer_DataRetrieverMonitorCollection_h
#define EventFilter_SMProxyServer_DataRetrieverMonitorCollection_h

#include "EventFilter/SMProxyServer/interface/ConnectionID.h"
#include "EventFilter/StorageManager/interface/MonitorCollection.h"
#include "EventFilter/StorageManager/interface/MonitoredQuantity.h"
#include "EventFilter/StorageManager/interface/RegistrationInfoBase.h"
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
   * $Revision: 1.1.2.1 $
   * $Date: 2011/01/26 16:06:54 $
   */
  
  class DataRetrieverMonitorCollection : public stor::MonitorCollection
  {
  public:

    enum ConnectionStatus { CONNECTED, CONNECTION_FAILED, DISCONNECTED, UNKNOWN };

    struct DataRetrieverStats
    {
      edm::ParameterSet pset;
      ConnectionStatus connectionStatus;
      stor::MonitoredQuantity::Stats sizeStats;         //kB

      bool operator<(const DataRetrieverStats& other) const
      { return ( pset.getParameter<std::string>("sourceURL") < 
          other.pset.getParameter<std::string>("sourceURL")); }
    };
    typedef std::vector<DataRetrieverStats> DataRetrieverStatList;
    
    
    explicit DataRetrieverMonitorCollection(const stor::utils::duration_t& updateInterval);
    
    /**
     * Add a new  server connection.
     * Returns an unique connection ID.
     */
    ConnectionID addNewConnection(const edm::ParameterSet&);

    /**
     * Set status of given connection. Returns false if the ConnectionID is unknown.
     */
    bool setConnectionStatus(const ConnectionID&, const ConnectionStatus&);

    /**
     * Replace the consumer ParameterSet for all connections
     */
    void updatePSet(const edm::ParameterSet&);

    /**
     * Add a retrieved  sample in Bytes from the given connection.
     * Returns false if the ConnectionID is unknown.
     */
    bool addRetrievedSample(const ConnectionID&, const unsigned int& size);
    
    /**
     * Write the total  retrieval statistics into the given Stats struct.
     */
    void getTotalStats(DataRetrieverStats&) const;

    /**
     * Write the  retrieval statistics for the given connectionID into the Stats struct.
     */
    void getStatsByConnection(DataRetrieverStatList&) const;
    

  private:

    stor::MonitoredQuantity _totalSize;

    struct DataRetrieverMQ
    {
      edm::ParameterSet _pset;
      ConnectionStatus _connectionStatus;
      stor::MonitoredQuantity _size;       //kB

      DataRetrieverMQ
      (
        const edm::ParameterSet&,
        const stor::utils::duration_t& updateInterval
      );
    };

    //Prevent copying of the DataRetrieverMonitorCollection
    DataRetrieverMonitorCollection(DataRetrieverMonitorCollection const&);
    DataRetrieverMonitorCollection& operator=(DataRetrieverMonitorCollection const&);

    const stor::utils::duration_t _updateInterval;
    typedef boost::shared_ptr<DataRetrieverMQ> DataRetrieverMQPtr;
    typedef std::map<ConnectionID, DataRetrieverMQPtr> RetrieverStatMap;
    RetrieverStatMap _retrieverStats;
    mutable boost::mutex _retrieverStatsMutex;
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
