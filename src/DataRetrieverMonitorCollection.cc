// $Id: DataRetrieverMonitorCollection.cc,v 1.1.2.5 2011/02/17 13:19:28 mommsen Exp $
/// @file: DataRetrieverMonitorCollection.cc

#include <string>
#include <sstream>
#include <iomanip>

#include <boost/pointer_cast.hpp>

#include "EventFilter/SMProxyServer/interface/Exception.h"
#include "EventFilter/SMProxyServer/interface/DataRetrieverMonitorCollection.h"

using namespace smproxy;

DataRetrieverMonitorCollection::DataRetrieverMonitorCollection(const stor::utils::duration_t& updateInterval) :
MonitorCollection(updateInterval),
_totalSize(updateInterval, boost::posix_time::seconds(60)),
_updateInterval(updateInterval),
_eventTypeMqMap(updateInterval)
{}


ConnectionID DataRetrieverMonitorCollection::addNewConnection(const stor::RegPtr regPtr)
{
  boost::mutex::scoped_lock sl(_statsMutex);
  ++_nextConnectionId;

  DataRetrieverMQPtr dataRetrieverMQ( new DataRetrieverMQ(regPtr, _updateInterval) );
  _retrieverMqMap.insert(RetrieverMqMap::value_type(_nextConnectionId, dataRetrieverMQ));

  _eventTypeMqMap.insert(regPtr);
  
  _connectionMqMap.insert(ConnectionMqMap::value_type(regPtr->sourceURL(),
      stor::MonitoredQuantityPtr(
        new stor::MonitoredQuantity(_updateInterval, boost::posix_time::seconds(60))
      )
    ));

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


bool DataRetrieverMonitorCollection::getEventTypeStatsForConnection
(
  const ConnectionID& connectionId,
  EventTypeStats& stats
)
{
  boost::mutex::scoped_lock sl(_statsMutex);
  RetrieverMqMap::const_iterator pos = _retrieverMqMap.find(connectionId);

  if ( pos == _retrieverMqMap.end() ) return false;

  stats.regPtr = pos->second->_regPtr;
  stats.connectionStatus = pos->second->_connectionStatus;
  pos->second->_size.getStats(stats.sizeStats);

  return true;
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

  const stor::RegPtr regPtr = retrieverPos->second->_regPtr;

  _eventTypeMqMap.addSample(regPtr, sizeKB);

  const std::string sourceURL = regPtr->sourceURL();
  ConnectionMqMap::const_iterator connectionPos = _connectionMqMap.find(sourceURL);
  connectionPos->second->addSample(sizeKB);
  
  _totalSize.addSample(sizeKB);

  return true;
}


void DataRetrieverMonitorCollection::getSummaryStats(SummaryStats& stats) const
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
  
  _eventTypeMqMap.getStats(stats.eventTypeStats);
  
  _totalSize.getStats(stats.sizeStats);
}


void DataRetrieverMonitorCollection::getStatsByConnection(ConnectionStats& cs) const
{
  boost::mutex::scoped_lock sl(_statsMutex);
  cs.clear();
  
  for (ConnectionMqMap::const_iterator it = _connectionMqMap.begin(),
         itEnd = _connectionMqMap.end(); it != itEnd; ++it)
  {
    stor::MonitoredQuantity::Stats stats;
    it->second->getStats(stats);
    cs.insert(ConnectionStats::value_type(it->first, stats));
  }
}


void DataRetrieverMonitorCollection::getStatsByEventTypes(EventTypeStatList& etsl) const
{
  boost::mutex::scoped_lock sl(_statsMutex);
  etsl.clear();

  for (RetrieverMqMap::const_iterator it = _retrieverMqMap.begin(),
         itEnd = _retrieverMqMap.end(); it != itEnd; ++it)
  {
    const DataRetrieverMQPtr mq = it->second;
    EventTypeStats stats;
    stats.regPtr = mq->_regPtr;
    stats.connectionStatus = mq->_connectionStatus;
    mq->_size.getStats(stats.sizeStats);
    etsl.push_back(stats);
  }
  std::sort(etsl.begin(), etsl.end());
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
 
  for (ConnectionMqMap::const_iterator it = _connectionMqMap.begin(),
         itEnd = _connectionMqMap.end(); it != itEnd; ++it)
  {
    it->second->calculateStatistics();
  }

  _eventTypeMqMap.calculateStatistics();
}


void DataRetrieverMonitorCollection::do_reset()
{
  boost::mutex::scoped_lock sl(_statsMutex);
  _totalSize.reset();
  _retrieverMqMap.clear();
  _connectionMqMap.clear();
  _eventTypeMqMap.clear();
}


bool DataRetrieverMonitorCollection::EventTypeMqMap::
insert(const stor::RegPtr consumer)
{
  return (
    insert(boost::dynamic_pointer_cast<stor::EventConsumerRegistrationInfo>(consumer)) ||
    insert(boost::dynamic_pointer_cast<stor::DQMEventConsumerRegistrationInfo>(consumer))
  );
}


bool DataRetrieverMonitorCollection::EventTypeMqMap::
addSample(const stor::RegPtr consumer, const double& sizeKB)
{
  return (
    addSample(boost::dynamic_pointer_cast<stor::EventConsumerRegistrationInfo>(consumer), sizeKB) ||
    addSample(boost::dynamic_pointer_cast<stor::DQMEventConsumerRegistrationInfo>(consumer), sizeKB)
  );
}


void DataRetrieverMonitorCollection::EventTypeMqMap::
getStats(SummaryStats::EventTypeStatList& eventTypeStats) const
{
  eventTypeStats.clear();
  eventTypeStats.reserve(_eventMap.size()+_dqmEventMap.size());
  
  for (EventMap::const_iterator it = _eventMap.begin(),
         itEnd = _eventMap.end(); it != itEnd; ++it)
  {
    stor::MonitoredQuantity::Stats etStats;
    it->second->getStats(etStats);
    eventTypeStats.push_back(
      std::make_pair(it->first, etStats));
  }
  
  for (DQMEventMap::const_iterator it = _dqmEventMap.begin(),
         itEnd = _dqmEventMap.end(); it != itEnd; ++it)
  {
    stor::MonitoredQuantity::Stats etStats;
    it->second->getStats(etStats);
    eventTypeStats.push_back(
      std::make_pair(it->first, etStats));
  }
}


void DataRetrieverMonitorCollection::EventTypeMqMap::
calculateStatistics()
{
  for (EventMap::iterator it = _eventMap.begin(),
         itEnd = _eventMap.end(); it != itEnd; ++it)
  {
    it->second->calculateStatistics();
  }
  for (DQMEventMap::iterator it = _dqmEventMap.begin(),
         itEnd = _dqmEventMap.end(); it != itEnd; ++it)
  {
    it->second->calculateStatistics();
  }
}


void DataRetrieverMonitorCollection::EventTypeMqMap::
clear()
{
  _eventMap.clear();
  _dqmEventMap.clear();
}


bool DataRetrieverMonitorCollection::EventTypeMqMap::
insert(const stor::EventConsRegPtr eventConsumer)
{
  if ( eventConsumer == 0 ) return false;
  _eventMap.insert(EventMap::value_type(eventConsumer,
      stor::MonitoredQuantityPtr(
        new stor::MonitoredQuantity( _updateInterval, boost::posix_time::seconds(60) )
      )));
  return true;
}


bool DataRetrieverMonitorCollection::EventTypeMqMap::
insert(const stor::DQMEventConsRegPtr dqmEventConsumer)
{
  if ( dqmEventConsumer == 0 ) return false;
  _dqmEventMap.insert(DQMEventMap::value_type(dqmEventConsumer,
      stor::MonitoredQuantityPtr(
        new stor::MonitoredQuantity( _updateInterval, boost::posix_time::seconds(60) )
      )));
  return true;
}


bool DataRetrieverMonitorCollection::EventTypeMqMap::
addSample(const stor::EventConsRegPtr eventConsumer, const double& sizeKB)
{
  if ( eventConsumer == 0 ) return false;
  EventMap::const_iterator pos = _eventMap.find(eventConsumer);
  pos->second->addSample(sizeKB);
  return true;
}


bool DataRetrieverMonitorCollection::EventTypeMqMap::
addSample(const stor::DQMEventConsRegPtr dqmEventConsumer, const double& sizeKB)
{
  if ( dqmEventConsumer == 0 ) return false;
  DQMEventMap::const_iterator pos = _dqmEventMap.find(dqmEventConsumer);
  pos->second->addSample(sizeKB);
  return true;
}


bool DataRetrieverMonitorCollection::EventTypeStats::operator<(const EventTypeStats& other) const
{
  if ( regPtr->sourceURL() != other.regPtr->sourceURL() )
    return ( regPtr->sourceURL() < other.regPtr->sourceURL() );

  stor::EventConsRegPtr ecrp =
    boost::dynamic_pointer_cast<stor::EventConsumerRegistrationInfo>(regPtr);
  stor::EventConsRegPtr ecrpOther =
    boost::dynamic_pointer_cast<stor::EventConsumerRegistrationInfo>(other.regPtr);
  if ( ecrp && ecrpOther )
    return ( *ecrp < *ecrpOther);

  stor::DQMEventConsRegPtr dcrp =
    boost::dynamic_pointer_cast<stor::DQMEventConsumerRegistrationInfo>(regPtr);
  stor::DQMEventConsRegPtr dcrpOther =
    boost::dynamic_pointer_cast<stor::DQMEventConsumerRegistrationInfo>(other.regPtr);
  if ( dcrp && dcrpOther )
    return ( *dcrp < *dcrpOther);

  return false;
}


DataRetrieverMonitorCollection::DataRetrieverMQ::DataRetrieverMQ
(
  const stor::RegPtr regPtr,
  const stor::utils::duration_t& updateInterval
):
_regPtr(regPtr),
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
