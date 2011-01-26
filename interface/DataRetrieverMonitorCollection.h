// $Id: DataRetrieverMonitorCollection.h,v 1.7.8.1 2011/01/24 12:18:39 mommsen Exp $
/// @file: DataRetrieverMonitorCollection.h 

#ifndef EventFilter_SMProxyServer_DataRetrieverMonitorCollection_h
#define EventFilter_SMProxyServer_DataRetrieverMonitorCollection_h

#include "EventFilter/StorageManager/interface/MonitorCollection.h"


namespace smproxy {

  /**
   * A collection of MonitoredQuantities related to data retrieval
   *
   * $Author: mommsen $
   * $Revision: 1.7.8.1 $
   * $Date: 2011/01/24 12:18:39 $
   */
  
  class DataRetrieverMonitorCollection : public stor::MonitorCollection
  {
  private:

    stor::MonitoredQuantity _eventConnectionCount;
    stor::MonitoredQuantity _eventSize;
    stor::MonitoredQuantity _eventBandwidth;

    stor::MonitoredQuantity _dqmEventConnectionCount;
    stor::MonitoredQuantity _dqmEventSize;
    stor::MonitoredQuantity _dqmEventBandwidth;

  public:

    struct DataRetrieverStats
    {
      stor::MonitoredQuantity::Stats eventConnectionCountStats;
      stor::MonitoredQuantity::Stats eventSizeStats;                //MB
      stor::MonitoredQuantity::Stats eventBandwidthStats;           //MB/s
      
      stor::MonitoredQuantity::Stats dqmEventConnectionCountStats;
      stor::MonitoredQuantity::Stats dqmEventSizeStats;             //MB
      stor::MonitoredQuantity::Stats dqmEventBandwidthStats;        //MB/s
    };

    explicit DataRetrieverMonitorCollection(const stor::utils::duration_t& updateInterval);

    const stor::MonitoredQuantity& getEventConnectionCountMQ() const {
      return _eventConnectionCount;
    }
    stor::MonitoredQuantity& getEventConnectionCountMQ() {
      return _eventConnectionCount;
    }

    const stor::MonitoredQuantity& getEventSizeMQ() const {
      return _eventSize;
    }
    stor::MonitoredQuantity& getEventSizeMQ() {
      return _eventSize;
    }

    const stor::MonitoredQuantity& getEventBandwidthMQ() const {
      return _eventBandwidth;
    }
    stor::MonitoredQuantity& getEventBandwidthMQ() {
      return _eventBandwidth;
    }

    const stor::MonitoredQuantity& getDQMEventConnectionCountMQ() const {
      return _dqmEventConnectionCount;
    }
    stor::MonitoredQuantity& getDQMEventConnectionCountMQ() {
      return _dqmEventConnectionCount;
    }

    const stor::MonitoredQuantity& getDQMEventSizeMQ() const {
      return _dqmEventSize;
    }
    stor::MonitoredQuantity& getDQMEventSizeMQ() {
      return _dqmEventSize;
    }

    const stor::MonitoredQuantity& getDQMEventBandwidthMQ() const {
      return _dqmEventBandwidth;
    }
    stor::MonitoredQuantity& getDQMEventBandwidthMQ() {
      return _dqmEventBandwidth;
    }

   /**
    * Write all our collected statistics into the given Stats struct.
    */
    void getStats(DataRetrieverStats& stats) const;


  private:

    //Prevent copying of the DataRetrieverMonitorCollection
    DataRetrieverMonitorCollection(DataRetrieverMonitorCollection const&);
    DataRetrieverMonitorCollection& operator=(DataRetrieverMonitorCollection const&);

    virtual void do_calculateStatistics();
    virtual void do_reset();
    // virtual void do_appendInfoSpaceItems(InfoSpaceItems&);
    // virtual void do_updateInfoSpaceItems();

  };
  
} // namespace stor

#endif // EventFilter_SMProxyServer_DataRetrieverMonitorCollection_h 


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
