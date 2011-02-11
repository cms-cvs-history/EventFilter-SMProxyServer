// $Id: DataRetrieverMonitorCollection.cc,v 1.1.2.2 2011/02/08 16:51:51 mommsen Exp $
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
  boost::mutex::scoped_lock sl(_statsMutex);
  ++_nextConnectionId;
  const std::string sourceURL = pset.getParameter<std::string>("sourceURL");
  stor::EventConsRegPtr ecrp( new stor::EventConsumerRegistrationInfo("", sourceURL, pset));
  DataRetrieverMQPtr dataRetrieverMQ( new DataRetrieverMQ(ecrp, _updateInterval) );
  _retrieverMqMap.insert(RetrieverMqMap::value_type(_nextConnectionId, dataRetrieverMQ));
  return _nextConnectionId;
}


bool DataRetrieverMonitorCollection::setConnectionStatus
(
  const ConnectionID& connectionId,
  const ConnectionStatus& status
)
{
  boost::mutex::scoped_lock sl(_statsMutex);
  RetrieverMqMap::const_iterator pos = _retrieverMqMap.find(connectionId);
  if ( pos == _retrieverMqMap.end() ) return false;
  pos->second->_connectionStatus = status;
  return true;
}


void DataRetrieverMonitorCollection::updateConsumerInfo
(
  const stor::EventConsRegPtr eventConsRegPtr
)
{
  boost::mutex::scoped_lock sl(_statsMutex);
  for (RetrieverMqMap::const_iterator it = _retrieverMqMap.begin(),
         itEnd = _retrieverMqMap.end(); it != itEnd; ++it)
  {
    it->second->_eventConsRegPtr = eventConsRegPtr;
  }
}


bool DataRetrieverMonitorCollection::addRetrievedSample
(
  const ConnectionID& connectionId,
  const unsigned int& size
)
{
  boost::mutex::scoped_lock sl(_statsMutex);

  RetrieverMqMap::const_iterator retrieverPos = _retrieverMqMap.find(connectionId);
  if ( retrieverPos == _retrieverMqMap.end() ) return false;

  const double sizeKB = static_cast<double>(size) / 1024;
  retrieverPos->second->_size.addSample(sizeKB);

  stor::EventConsRegPtr ecrp = retrieverPos->second->_eventConsRegPtr;

  EventTypeMqMap::iterator eventTypePos = _eventTypeMqMap.lower_bound(ecrp);
  
  if (eventTypePos == _eventTypeMqMap.end() || (_eventTypeMqMap.key_comp()(ecrp, eventTypePos->first)))
  {
    // The key does not exist in the map, add it to the map
    // Use pos as a hint to insert, so it can avoid another lookup
    eventTypePos = _eventTypeMqMap.insert(eventTypePos,
      EventTypeMqMap::value_type(ecrp,
        stor::MonitoredQuantityPtr(
          new stor::MonitoredQuantity( _updateInterval, boost::posix_time::seconds(60) )
        )
      )
    );
  }
  eventTypePos->second->addSample(sizeKB);

  _totalSize.addSample(sizeKB);

  return true;
}


void DataRetrieverMonitorCollection::getSummaryStats(DataRetrieverSummaryStats& stats) const
{
  boost::mutex::scoped_lock sl(_statsMutex);

  stats.registeredSMs = 0;
  stats.activeSMs = 0;

  for (RetrieverMqMap::const_iterator it = _retrieverMqMap.begin(),
         itEnd = _retrieverMqMap.end(); it != itEnd; ++it)
  {
    ++stats.registeredSMs;
    if ( it->second->_connectionStatus == CONNECTED )
      ++stats.activeSMs;
  }
  
  stats.eventTypeStats.clear();
  stats.eventTypeStats.reserve(_eventTypeMqMap.size());
  
  for (EventTypeMqMap::const_iterator it = _eventTypeMqMap.begin(),
         itEnd = _eventTypeMqMap.end(); it != itEnd; ++it)
  {
    stor::MonitoredQuantity::Stats etStats;
    it->second->getStats(etStats);
    stats.eventTypeStats.push_back(
      std::make_pair(it->first, etStats));
  }
  
  _totalSize.getStats(stats.sizeStats);
}


void DataRetrieverMonitorCollection::getStatsByConnection(DataRetrieverStatList& drsl) const
{
  boost::mutex::scoped_lock sl(_statsMutex);
  drsl.clear();

  for (RetrieverMqMap::const_iterator it = _retrieverMqMap.begin(),
         itEnd = _retrieverMqMap.end(); it != itEnd; ++it)
  {
    const DataRetrieverMQPtr mq = it->second;
    DataRetrieverStats stats;
    stats.eventConsRegPtr = mq->_eventConsRegPtr;
    stats.connectionStatus = mq->_connectionStatus;
    mq->_size.getStats(stats.sizeStats);
    drsl.push_back(stats);
  }
  std::sort(drsl.begin(), drsl.end());
}


void DataRetrieverMonitorCollection::do_calculateStatistics()
{
  boost::mutex::scoped_lock sl(_statsMutex);

  _totalSize.calculateStatistics();

  for (RetrieverMqMap::const_iterator it = _retrieverMqMap.begin(),
         itEnd = _retrieverMqMap.end(); it != itEnd; ++it)
  {
    it->second->_size.calculateStatistics();
  }

  for (EventTypeMqMap::iterator it = _eventTypeMqMap.begin(),
         itEnd = _eventTypeMqMap.end(); it != itEnd; ++it)
  {
    it->second->calculateStatistics();
  }
}


void DataRetrieverMonitorCollection::do_reset()
{
  boost::mutex::scoped_lock sl(_statsMutex);
  _totalSize.reset();
  _retrieverMqMap.clear();
  _eventTypeMqMap.clear();
}


DataRetrieverMonitorCollection::DataRetrieverMQ::DataRetrieverMQ
(
  const stor::EventConsRegPtr eventConsRegPtr,
  const stor::utils::duration_t& updateInterval
):
_eventConsRegPtr(eventConsRegPtr),
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
