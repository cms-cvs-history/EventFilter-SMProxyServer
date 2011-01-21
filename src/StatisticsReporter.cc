// $Id: StatisticsReporter.cc,v 1.20 2010/12/14 12:56:52 mommsen Exp $
/// @file: StatisticsReporter.cc

#include <sstream>

#include "toolbox/net/URN.h"
#include "toolbox/task/Action.h"
#include "toolbox/task/WorkLoopFactory.h"
#include "xcept/tools.h"
#include "xdaq/ApplicationDescriptor.h"
#include "xdata/Event.h"
#include "xdata/InfoSpaceFactory.h"

#include "EventFilter/StorageManager/interface/AlarmHandler.h"
#include "EventFilter/StorageManager/interface/MonitoredQuantity.h"
#include "EventFilter/SMProxyServer/interface/Exception.h"
#include "EventFilter/SMProxyServer/interface/StatisticsReporter.h"

using namespace smproxy;


StatisticsReporter::StatisticsReporter
(
  xdaq::Application *app,
  const QueueConfigurationParams& qcp
) :
_app(app),
_alarmHandler(new stor::AlarmHandler(app)),
_monitoringSleepSec(qcp._monitoringSleepSec),
_dqmEventMonCollection(_monitoringSleepSec*5),
_eventConsumerMonCollection(_monitoringSleepSec),
_dqmConsumerMonCollection(_monitoringSleepSec),
_doMonitoring(_monitoringSleepSec>boost::posix_time::seconds(0))
{
  reset();
  createMonitoringInfoSpace();
  collectInfoSpaceItems();
}


void StatisticsReporter::startWorkLoop(std::string workloopName)
{
  if ( !_doMonitoring ) return;

  try
  {
    std::string identifier = stor::utils::getIdentifier(_app->getApplicationDescriptor());

    _monitorWL=
      toolbox::task::getWorkLoopFactory()->getWorkLoop(
        identifier + workloopName, "waiting");

    if ( ! _monitorWL->isActive() )
    {
      toolbox::task::ActionSignature* monitorAction = 
        toolbox::task::bind(this, &StatisticsReporter::monitorAction, 
          identifier + "MonitorAction");
      _monitorWL->submit(monitorAction);

      _lastMonitorAction = stor::utils::getCurrentTime();
      _monitorWL->activate();
    }
  }
  catch (xcept::Exception& e)
  {
    std::string msg = "Failed to start workloop 'StatisticsReporter' with 'MonitorAction'.";
    XCEPT_RETHROW(exception::Monitoring, msg, e);
  }
}


StatisticsReporter::~StatisticsReporter()
{
  // Stop the monitoring activity
  _doMonitoring = false;

  // Cancel the workloop (will wait until the action has finished)
  if ( _monitorWL && _monitorWL->isActive() ) _monitorWL->cancel();
}



void StatisticsReporter::createMonitoringInfoSpace()
{
  // Create an infospace which can be monitored.
    
  std::ostringstream oss;
  oss << "urn:xdaq-monitorable-" << _app->getApplicationDescriptor()->getClassName();
  
  std::string errorMsg =
    "Failed to create monitoring info space " + oss.str();
  
  try
  {
    toolbox::net::URN urn = _app->createQualifiedInfoSpace(oss.str());
    xdata::getInfoSpaceFactory()->lock();
    _infoSpace = xdata::getInfoSpaceFactory()->get(urn.toString());
    xdata::getInfoSpaceFactory()->unlock();
  }
  catch(xdata::exception::Exception &e)
  {
    xdata::getInfoSpaceFactory()->unlock();
    
    XCEPT_RETHROW(exception::Infospace, errorMsg, e);
  }
  catch (...)
  {
    xdata::getInfoSpaceFactory()->unlock();
    
    errorMsg += " : unknown exception";
    XCEPT_RAISE(exception::Infospace, errorMsg);
  }
}


void StatisticsReporter::collectInfoSpaceItems()
{
  stor::MonitorCollection::InfoSpaceItems infoSpaceItems;
  _infoSpaceItemNames.clear();

  _dqmEventMonCollection.appendInfoSpaceItems(infoSpaceItems);
  _eventConsumerMonCollection.appendInfoSpaceItems(infoSpaceItems);
  _dqmConsumerMonCollection.appendInfoSpaceItems(infoSpaceItems);

  putItemsIntoInfoSpace(infoSpaceItems);
}


void StatisticsReporter::putItemsIntoInfoSpace(stor::MonitorCollection::InfoSpaceItems& items)
{
  
  for ( stor::MonitorCollection::InfoSpaceItems::const_iterator it = items.begin(),
          itEnd = items.end();
        it != itEnd;
        ++it )
  {
    try
    {
      // fireItemAvailable locks the infospace internally
      _infoSpace->fireItemAvailable(it->first, it->second);
    }
    catch(xdata::exception::Exception &e)
    {
      std::stringstream oss;
      
      oss << "Failed to put " << it->first;
      oss << " into info space " << _infoSpace->name();
      
      XCEPT_RETHROW(exception::Monitoring, oss.str(), e);
    }

    // keep a list of info space names for the fireItemGroupChanged
    _infoSpaceItemNames.push_back(it->first);
  }
}


bool StatisticsReporter::monitorAction(toolbox::task::WorkLoop* wl)
{
  stor::utils::sleepUntil(_lastMonitorAction + _monitoringSleepSec);
  _lastMonitorAction = stor::utils::getCurrentTime();

  std::string errorMsg = "Failed to update the monitoring information";

  try
  {
    calculateStatistics();
    updateInfoSpace();
  }
  catch(xcept::Exception &e)
  {
    LOG4CPLUS_ERROR(_app->getApplicationLogger(),
      errorMsg << xcept::stdformat_exception_history(e));

    XCEPT_DECLARE_NESTED(exception::Monitoring,
      sentinelException, errorMsg, e);
    _app->notifyQualified("error", sentinelException);
  }
  catch(std::exception &e)
  {
    errorMsg += ": ";
    errorMsg += e.what();

    LOG4CPLUS_ERROR(_app->getApplicationLogger(),
      errorMsg);
    
    XCEPT_DECLARE(exception::Monitoring,
      sentinelException, errorMsg);
    _app->notifyQualified("error", sentinelException);
  }
  catch(...)
  {
    errorMsg += ": Unknown exception";

    LOG4CPLUS_ERROR(_app->getApplicationLogger(),
      errorMsg);
    
    XCEPT_DECLARE(exception::Monitoring,
      sentinelException, errorMsg);
    _app->notifyQualified("error", sentinelException);
  }

  return _doMonitoring;
}


void StatisticsReporter::calculateStatistics()
{
  const stor::utils::time_point_t now = stor::utils::getCurrentTime();

  _dqmEventMonCollection.calculateStatistics(now);
  _eventConsumerMonCollection.calculateStatistics(now);
  _dqmConsumerMonCollection.calculateStatistics(now);
}


void StatisticsReporter::updateInfoSpace()
{
  std::string errorMsg =
    "Failed to update values of items in info space " + _infoSpace->name();

  // Lock the infospace to assure that all items are consistent
  try
  {
    _infoSpace->lock();

    _dqmEventMonCollection.updateInfoSpaceItems();
    _eventConsumerMonCollection.updateInfoSpaceItems();
    _dqmConsumerMonCollection.updateInfoSpaceItems();

    _infoSpace->unlock();
  }
  catch(std::exception &e)
  {
    _infoSpace->unlock();
    
    errorMsg += ": ";
    errorMsg += e.what();
    XCEPT_RAISE(exception::Monitoring, errorMsg);
  }
  catch (...)
  {
    _infoSpace->unlock();
    
    errorMsg += " : unknown exception";
    XCEPT_RAISE(exception::Monitoring, errorMsg);
  }
  
  try
  {
    // The fireItemGroupChanged locks the infospace
    _infoSpace->fireItemGroupChanged(_infoSpaceItemNames, this);
  }
  catch (xdata::exception::Exception &e)
  {
    XCEPT_RETHROW(exception::Monitoring, errorMsg, e);
  }
}


void StatisticsReporter::reset()
{
  const stor::utils::time_point_t now = stor::utils::getCurrentTime();

  _dqmEventMonCollection.reset(now);
  _eventConsumerMonCollection.reset(now);
  _dqmConsumerMonCollection.reset(now);

  _alarmHandler->clearAllAlarms();
}


void StatisticsReporter::actionPerformed(xdata::Event& ispaceEvent)
{
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
