// $Id: DataRetrieverMonitorCollection.cc,v 1.10 2010/12/14 12:56:52 mommsen Exp $
/// @file: DataRetrieverMonitorCollection.cc

#include <string>
#include <sstream>
#include <iomanip>

#include "EventFilter/SMProxyServer/interface/Exception.h"
#include "EventFilter/SMProxyServer/interface/DataRetrieverMonitorCollection.h"

using namespace smproxy;

DataRetrieverMonitorCollection::DataRetrieverMonitorCollection(const stor::utils::duration_t& updateInterval) :
MonitorCollection(updateInterval),
_eventConnectionCount(updateInterval, boost::posix_time::seconds(60)),
_eventSize(updateInterval, boost::posix_time::seconds(60)),
_eventBandwidth(updateInterval, boost::posix_time::seconds(60)),
_dqmEventConnectionCount(updateInterval, boost::posix_time::seconds(60)),
_dqmEventSize(updateInterval, boost::posix_time::seconds(60)),
_dqmEventBandwidth(updateInterval, boost::posix_time::seconds(60))
{}


void DataRetrieverMonitorCollection::getStats(DataRetrieverStats& stats) const
{
  _eventConnectionCount.getStats(stats.eventConnectionCountStats);
  _eventSize.getStats(stats.eventSizeStats);
  _eventBandwidth.getStats(stats.eventBandwidthStats);

  _dqmEventConnectionCount.getStats(stats.dqmEventConnectionCountStats);
  _dqmEventSize.getStats(stats.dqmEventSizeStats);
  _dqmEventBandwidth.getStats(stats.dqmEventBandwidthStats);
}


void DataRetrieverMonitorCollection::do_calculateStatistics()
{
  _eventConnectionCount.calculateStatistics();
  _eventSize.calculateStatistics();
  stor::MonitoredQuantity::Stats eventSizeStats;
  _eventSize.getStats(eventSizeStats);
  if (eventSizeStats.getSampleCount() > 0) {
    _eventBandwidth.addSample(eventSizeStats.getLastValueRate());
  }
  _eventBandwidth.calculateStatistics();

  _dqmEventConnectionCount.calculateStatistics();
  _dqmEventSize.calculateStatistics();
  stor::MonitoredQuantity::Stats dqmEventSizeStats;
  _dqmEventSize.getStats(dqmEventSizeStats);
  if (dqmEventSizeStats.getSampleCount() > 0) {
    _dqmEventBandwidth.addSample(dqmEventSizeStats.getLastValueRate());
  }
  _dqmEventBandwidth.calculateStatistics();
}


void DataRetrieverMonitorCollection::do_reset()
{
  _eventConnectionCount.reset();
  _eventSize.reset();
  _eventBandwidth.reset();
  
  _dqmEventConnectionCount.reset();
  _dqmEventSize.reset();
  _dqmEventBandwidth.reset();
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
