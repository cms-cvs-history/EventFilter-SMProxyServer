// $Id: Configuration.cc,v 1.1.2.3 2011/01/21 15:54:57 mommsen Exp $
/// @file: Configuration.cc

#include "EventFilter/SMProxyServer/interface/Configuration.h"

#include <toolbox/net/Utils.h>

#include <sstream>


namespace smproxy
{
  Configuration::Configuration(xdata::InfoSpace* infoSpace,
                               unsigned long instanceNumber) :
    _infospaceRunNumber(0),
    _localRunNumber(0)
  {
    // default values are used to initialize infospace values,
    // so they should be set first
    setDataRetrieverDefaults(instanceNumber);
    setEventServingDefaults();
    setQueueConfigurationDefaults();

    setupDataRetrieverInfoSpaceParams(infoSpace);
    setupEventServingInfoSpaceParams(infoSpace);
    setupQueueConfigurationInfoSpaceParams(infoSpace);
  }

  unsigned int Configuration::getRunNumber() const
  {
    return _localRunNumber;
  }

  struct DataRetrieverParams Configuration::getDataRetrieverParams() const
  {
    boost::mutex::scoped_lock sl(_generalMutex);
    return _dataRetrieverParamCopy;
  }

  struct stor::EventServingParams Configuration::getEventServingParams() const
  {
    boost::mutex::scoped_lock sl(_generalMutex);
    return _eventServeParamCopy;
  }

  struct QueueConfigurationParams Configuration::getQueueConfigurationParams() const
  {
    boost::mutex::scoped_lock sl(_generalMutex);
    return _queueConfigParamCopy;
  }

  void Configuration::updateAllParams()
  {
    boost::mutex::scoped_lock sl(_generalMutex);
    updateLocalRunData();
    updateLocalDataRetrieverData();
    updateLocalEventServingData();
    updateLocalQueueConfigurationData();
  }

  void Configuration::setDataRetrieverDefaults(unsigned long instanceNumber)
  {
    _dataRetrieverParamCopy._smpsInstance = instanceNumber;
    _dataRetrieverParamCopy._smRegistrationList.clear();
    _dataRetrieverParamCopy._allowMissingSM = true;
    _dataRetrieverParamCopy._sleepTimeIfIdle =
      boost::posix_time::milliseconds(100);

    std::string tmpString(toolbox::net::getHostName());
    // strip domainame
    std::string::size_type pos = tmpString.find('.');  
    if (pos != std::string::npos) {  
      std::string basename = tmpString.substr(0,pos);  
      tmpString = basename;
    }
    _dataRetrieverParamCopy._hostName = tmpString;
  }
  
  void Configuration::setEventServingDefaults()
  {
    _eventServeParamCopy._activeConsumerTimeout = boost::posix_time::seconds(60);
    _eventServeParamCopy._consumerQueueSize = 5;
    _eventServeParamCopy._consumerQueuePolicy = "DiscardOld";
    _eventServeParamCopy._DQMactiveConsumerTimeout = boost::posix_time::seconds(60);
    _eventServeParamCopy._DQMconsumerQueueSize = 15;
    _eventServeParamCopy._DQMconsumerQueuePolicy = "DiscardOld";
  }

  void Configuration::setQueueConfigurationDefaults()
  {
    _queueConfigParamCopy._registrationQueueSize = 128;
    _queueConfigParamCopy._monitoringSleepSec = boost::posix_time::seconds(1);
  }

  void Configuration::
  setupRunInfoSpaceParams(xdata::InfoSpace* infoSpace)
  {
    // copy the initial defaults into the xdata variables
    _infospaceRunNumber = _localRunNumber;

    // bind the local xdata variables to the infospace
    infoSpace->fireItemAvailable("runNumber", &_infospaceRunNumber);
  }

  void Configuration::
  setupDataRetrieverInfoSpaceParams(xdata::InfoSpace* infoSpace)
  {
    // copy the initial defaults into the xdata variables
    stor::utils::getXdataVector(_dataRetrieverParamCopy._smRegistrationList, _smRegistrationList);
    _allowMissingSM = _dataRetrieverParamCopy._allowMissingSM;
    _sleepTimeIfIdle = _dataRetrieverParamCopy._sleepTimeIfIdle.total_milliseconds();

    // bind the local xdata variables to the infospace
    infoSpace->fireItemAvailable("SMRegistrationList", &_smRegistrationList);
    infoSpace->fireItemAvailable("allowMissingSM", &_allowMissingSM);
    infoSpace->fireItemAvailable("sleepTimeIfIdle", &_sleepTimeIfIdle);
  }
  
  void Configuration::
  setupEventServingInfoSpaceParams(xdata::InfoSpace* infoSpace)
  {
    // copy the initial defaults to the xdata variables
    _activeConsumerTimeout = _eventServeParamCopy._activeConsumerTimeout.total_seconds();
    _consumerQueueSize = _eventServeParamCopy._consumerQueueSize;
    _consumerQueuePolicy = _eventServeParamCopy._consumerQueuePolicy;
    _DQMactiveConsumerTimeout = _eventServeParamCopy._DQMactiveConsumerTimeout.total_seconds();
    _DQMconsumerQueueSize = _eventServeParamCopy._DQMconsumerQueueSize;
    _DQMconsumerQueuePolicy = _eventServeParamCopy._DQMconsumerQueuePolicy;

    // bind the local xdata variables to the infospace
    infoSpace->fireItemAvailable("activeConsumerTimeout", &_activeConsumerTimeout);
    infoSpace->fireItemAvailable("consumerQueueSize", &_consumerQueueSize);
    infoSpace->fireItemAvailable("consumerQueuePolicy", &_consumerQueuePolicy);
    infoSpace->fireItemAvailable("DQMactiveConsumerTimeout", &_DQMactiveConsumerTimeout);
    infoSpace->fireItemAvailable("DQMconsumerQueueSize", &_DQMconsumerQueueSize);
    infoSpace->fireItemAvailable("DQMconsumerQueuePolicy",&_DQMconsumerQueuePolicy);
  }
  
  void Configuration::
  setupQueueConfigurationInfoSpaceParams(xdata::InfoSpace* infoSpace)
  {
    // copy the initial defaults to the xdata variables
    _registrationQueueSize = _queueConfigParamCopy._registrationQueueSize;
    _monitoringSleepSec =
      stor::utils::duration_to_seconds(_queueConfigParamCopy._monitoringSleepSec);
    
    // bind the local xdata variables to the infospace
    infoSpace->fireItemAvailable("registrationQueueSize", &_registrationQueueSize);
    infoSpace->fireItemAvailable("monitoringSleepSec", &_monitoringSleepSec);
  }
 
  void Configuration::updateLocalRunData()
  {
    _localRunNumber = _infospaceRunNumber;
  }

  void Configuration::updateLocalDataRetrieverData()
  {
    stor::utils::getStdVector(_smRegistrationList, _dataRetrieverParamCopy._smRegistrationList);
    _dataRetrieverParamCopy._allowMissingSM = _allowMissingSM;
    _dataRetrieverParamCopy._sleepTimeIfIdle =
      boost::posix_time::milliseconds(_sleepTimeIfIdle);
  }

  void Configuration::updateLocalEventServingData()
  {
    _eventServeParamCopy._activeConsumerTimeout =
      boost::posix_time::seconds( static_cast<int>(_activeConsumerTimeout) );
    _eventServeParamCopy._consumerQueueSize = _consumerQueueSize;
    _eventServeParamCopy._consumerQueuePolicy = _consumerQueuePolicy;
    _eventServeParamCopy._DQMactiveConsumerTimeout = 
      boost::posix_time::seconds( static_cast<int>(_DQMactiveConsumerTimeout) );
    _eventServeParamCopy._DQMconsumerQueueSize = _DQMconsumerQueueSize;
    _eventServeParamCopy._DQMconsumerQueuePolicy = _DQMconsumerQueuePolicy;
    
    // validation
    if (_eventServeParamCopy._consumerQueueSize < 1)
    {
      _eventServeParamCopy._consumerQueueSize = 1;
    }
    if (_eventServeParamCopy._DQMconsumerQueueSize < 1)
    {
      _eventServeParamCopy._DQMconsumerQueueSize = 1;
    }
  }

  void Configuration::updateLocalQueueConfigurationData()
  {
    _queueConfigParamCopy._registrationQueueSize = _registrationQueueSize;
    _queueConfigParamCopy._monitoringSleepSec =
      stor::utils::seconds_to_duration(_monitoringSleepSec);
  }

  void Configuration::actionPerformed(xdata::Event& ispaceEvent)
  {
    boost::mutex::scoped_lock sl(_generalMutex);
  }


} // namespace smproxy

/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
