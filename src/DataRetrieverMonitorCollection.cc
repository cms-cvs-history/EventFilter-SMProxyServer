// $Id: DataRetrieverMonitorCollection.cc,v 1.1.2.1 2011/01/26 16:06:54 mommsen Exp $
/// @file: DataRetrieverMonitorCollection.cc

#include <string>
#include <sstream>
#include <iomanip>

#include "EventFilter/SMProxyServer/interface/Exception.h"
#include "EventFilter/SMProxyServer/interface/DataRetrieverMonitorCollection.h"

using namespace smproxy;

DataRetrieverMonitorCollection::DataRetrieverMonitorCollection(const stor::utils::duration_t& updateInterval) :
MonitorCollection(updateInterval),
_totalSize(updateInterval, boost::posix_time::seconds(60)),
_updateInterval(updateInterval)
{}


ConnectionID DataRetrieverMonitorCollection::addNewConnection(const edm::ParameterSet& pset)
{
  boost::mutex::scoped_lock sl(_retrieverStatsMutex);
  ++_nextConnectionId;
  DataRetrieverMQPtr dataRetrieverMQ( new DataRetrieverMQ(pset, _updateInterval) );
  _retrieverStats.insert(RetrieverStatMap::value_type(_nextConnectionId, dataRetrieverMQ));
  return _nextConnectionId;
}


bool DataRetrieverMonitorCollection::setConnectionStatus
(
  const ConnectionID& connectionId,
  const ConnectionStatus& status
)
{
  boost::mutex::scoped_lock sl(_retrieverStatsMutex);
  RetrieverStatMap::const_iterator pos = _retrieverStats.find(connectionId);
  if ( pos == _retrieverStats.end() ) return false;
  pos->second->_connectionStatus = status;
  return true;
}


void DataRetrieverMonitorCollection::updatePSet
(
  const edm::ParameterSet& pset
)
{
  boost::mutex::scoped_lock sl(_retrieverStatsMutex);
  for (RetrieverStatMap::const_iterator it = _retrieverStats.begin(),
         itEnd = _retrieverStats.end(); it != itEnd; ++it)
  {
    const std::string sourceURL = 
      it->second->_pset.getParameter<std::string>("sourceURL");
    it->second->_pset = pset;
    it->second->_pset.addParameter<std::string>("sourceURL", sourceURL);
  }
}


bool DataRetrieverMonitorCollection::addRetrievedSample
(
  const ConnectionID& connectionId,
  const unsigned int& size
)
{
  boost::mutex::scoped_lock sl(_retrieverStatsMutex);

  RetrieverStatMap::const_iterator pos = _retrieverStats.find(connectionId);
  if ( pos == _retrieverStats.end() ) return false;

  const double sizeKB = static_cast<double>(size) / 1024;
  pos->second->_size.addSample(sizeKB);
  _totalSize.addSample(sizeKB);

  return true;
}


void DataRetrieverMonitorCollection::getTotalStats(DataRetrieverStats& stats) const
{
  _totalSize.getStats(stats.sizeStats);
}


void DataRetrieverMonitorCollection::getStatsByConnection(DataRetrieverStatList& drsl) const
{
  boost::mutex::scoped_lock sl(_retrieverStatsMutex);
  drsl.clear();

  for (RetrieverStatMap::const_iterator it = _retrieverStats.begin(),
         itEnd = _retrieverStats.end(); it != itEnd; ++it)
  {
    const DataRetrieverMQPtr mq = it->second;
    DataRetrieverStats stats;
    stats.pset = mq->_pset;
    stats.connectionStatus = mq->_connectionStatus;
    mq->_size.getStats(stats.sizeStats);
    drsl.push_back(stats);
  }
  std::sort(drsl.begin(), drsl.end());
}


void DataRetrieverMonitorCollection::do_calculateStatistics()
{
  _totalSize.calculateStatistics();

  boost::mutex::scoped_lock sl(_retrieverStatsMutex);
  for (RetrieverStatMap::const_iterator it = _retrieverStats.begin(),
         itEnd = _retrieverStats.end(); it != itEnd; ++it)
  {
    it->second->_size.calculateStatistics();
  }
}


void DataRetrieverMonitorCollection::do_reset()
{
  _totalSize.reset();

  boost::mutex::scoped_lock sl(_retrieverStatsMutex);
  _retrieverStats.clear();
}


DataRetrieverMonitorCollection::DataRetrieverMQ::DataRetrieverMQ
(
  const edm::ParameterSet& pset,
  const stor::utils::duration_t& updateInterval
):
_pset(pset),
_connectionStatus(UNKNOWN),
_size(updateInterval, boost::posix_time::seconds(60))
{}


std::ostream& smproxy::operator<<(std::ostream& os, const DataRetrieverMonitorCollection::ConnectionStatus& status)
{
  switch (status)
  {
    case DataRetrieverMonitorCollection::CONNECTED :
      os << "Connected";
      break;
    case DataRetrieverMonitorCollection::CONNECTION_FAILED :
      os << "Could not connect. SM not running?";
      break;
    case DataRetrieverMonitorCollection::DISCONNECTED :
      os << "Lost connection to SM. Did it fail?";
      break;
    case DataRetrieverMonitorCollection::UNKNOWN :
      os << "unknown";
      break;
  }

  return os;
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
