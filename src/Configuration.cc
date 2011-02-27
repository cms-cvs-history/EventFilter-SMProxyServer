// $Id: Configuration.cc,v 1.1.2.9 2011/02/26 09:17:26 mommsen Exp $
/// @file: Configuration.cc

#include "EventFilter/SMProxyServer/interface/Configuration.h"

#include <toolbox/net/Utils.h>

#include <sstream>


namespace smproxy
{
  Configuration::Configuration
  (
    xdata::InfoSpace* infoSpace,
    unsigned long instanceNumber
  )
  {
    // default values are used to initialize infospace values,
    // so they should be set first
    setDataRetrieverDefaults(instanceNumber);
    setEventServingDefaults();
    setDQMProcessingDefaults();
    setDQMArchivingDefaults();
    setQueueConfigurationDefaults();

    setupDataRetrieverInfoSpaceParams(infoSpace);
    setupEventServingInfoSpaceParams(infoSpace);
    setupDQMProcessingInfoSpaceParams(infoSpace);
    setupDQMArchivingInfoSpaceParams(infoSpace);
    setupQueueConfigurationInfoSpaceParams(infoSpace);
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

  struct stor::DQMProcessingParams Configuration::getDQMProcessingParams() const
  {
    boost::mutex::scoped_lock sl(_generalMutex);
    return _dqmProcessingParamCopy;
  }

  struct DQMArchivingParams Configuration::getDQMArchivingParams() const
  {
    boost::mutex::scoped_lock sl(_generalMutex);
    return _dqmArchivingParamCopy;
  }

  struct QueueConfigurationParams Configuration::getQueueConfigurationParams() const
  {
    boost::mutex::scoped_lock sl(_generalMutex);
    return _queueConfigParamCopy;
  }

  void Configuration::updateAllParams()
  {
    boost::mutex::scoped_lock sl(_generalMutex);
    updateLocalDataRetrieverData();
    updateLocalEventServingData();
    updateLocalDQMProcessingData();
    updateLocalDQMArchivingData();
    updateLocalQueueConfigurationData();
  }

  void Configuration::setDataRetrieverDefaults(unsigned long instanceNumber)
  {
    _dataRetrieverParamCopy._smpsInstance = instanceNumber;
    _dataRetrieverParamCopy._smRegistrationList.clear();
    _dataRetrieverParamCopy._allowMissingSM = true;
    _dataRetrieverParamCopy._maxConnectionRetries = 5;
    _dataRetrieverParamCopy._connectTrySleepTime = 10;
    _dataRetrieverParamCopy._headerRetryInterval = 5;
    _dataRetrieverParamCopy._retryInterval = 1;
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
    _eventServeParamCopy._consumerQueueSize = 10;
    _eventServeParamCopy._consumerQueuePolicy = "DiscardOld";
    _eventServeParamCopy._DQMactiveConsumerTimeout = boost::posix_time::seconds(60);
    _eventServeParamCopy._DQMconsumerQueueSize = 15;
    _eventServeParamCopy._DQMconsumerQueuePolicy = "DiscardOld";
  }

  void Configuration::setDQMProcessingDefaults()
  {
    _dqmProcessingParamCopy._collateDQM = false;
    _dqmProcessingParamCopy._readyTimeDQM = boost::posix_time::seconds(120);
    _dqmProcessingParamCopy._useCompressionDQM = true;
    _dqmProcessingParamCopy._compressionLevelDQM = 1;
  }

  void Configuration::setDQMArchivingDefaults()
  {
    _dqmArchivingParamCopy._archiveDQM = false;
    _dqmArchivingParamCopy._filePrefixDQM = "/tmp/DQM";
    _dqmArchivingParamCopy._archiveIntervalDQM = 0;
  }

  void Configuration::setQueueConfigurationDefaults()
  {
    _queueConfigParamCopy._registrationQueueSize = 128;
    _queueConfigParamCopy._monitoringSleepSec = boost::posix_time::seconds(1);
  }

  void Configuration::
  setupDataRetrieverInfoSpaceParams(xdata::InfoSpace* infoSpace)
  {
    // copy the initial defaults into the xdata variables
    stor::utils::getXdataVector(_dataRetrieverParamCopy._smRegistrationList, _smRegistrationList);
    _allowMissingSM = _dataRetrieverParamCopy._allowMissingSM;
    _maxConnectionRetries = _dataRetrieverParamCopy._maxConnectionRetries;
    _connectTrySleepTime = _dataRetrieverParamCopy._connectTrySleepTime;
    _headerRetryInterval = _dataRetrieverParamCopy._headerRetryInterval;
    _retryInterval = _dataRetrieverParamCopy._retryInterval;
    _sleepTimeIfIdle = _dataRetrieverParamCopy._sleepTimeIfIdle.total_milliseconds();

    // bind the local xdata variables to the infospace
    infoSpace->fireItemAvailable("SMRegistrationList", &_smRegistrationList);
    infoSpace->fireItemAvailable("allowMissingSM", &_allowMissingSM);
    infoSpace->fireItemAvailable("maxConnectionRetries", &_maxConnectionRetries);
    infoSpace->fireItemAvailable("connectTrySleepTime", &_connectTrySleepTime);
    infoSpace->fireItemAvailable("headerRetryInterval", &_headerRetryInterval);
    infoSpace->fireItemAvailable("retryInterval", &_retryInterval);
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
  setupDQMProcessingInfoSpaceParams(xdata::InfoSpace* infoSpace)
  {
    // copy the initial defaults to the xdata variables
    _collateDQM = _dqmProcessingParamCopy._collateDQM;
    _readyTimeDQM = _dqmProcessingParamCopy._readyTimeDQM.total_seconds();
    _useCompressionDQM = _dqmProcessingParamCopy._useCompressionDQM;
    _compressionLevelDQM = _dqmProcessingParamCopy._compressionLevelDQM;

    // bind the local xdata variables to the infospace
    infoSpace->fireItemAvailable("collateDQM", &_collateDQM);
    infoSpace->fireItemAvailable("readyTimeDQM", &_readyTimeDQM);
    infoSpace->fireItemAvailable("useCompressionDQM", &_useCompressionDQM);
    infoSpace->fireItemAvailable("compressionLevelDQM", &_compressionLevelDQM);
  }

  void Configuration::
  setupDQMArchivingInfoSpaceParams(xdata::InfoSpace* infoSpace)
  {
    // copy the initial defaults to the xdata variables
    _archiveDQM = _dqmArchivingParamCopy._archiveDQM;
    _archiveIntervalDQM = _dqmArchivingParamCopy._archiveIntervalDQM;
    _filePrefixDQM = _dqmArchivingParamCopy._filePrefixDQM;

    // bind the local xdata variables to the infospace
    infoSpace->fireItemAvailable("archiveDQM", &_archiveDQM);
    infoSpace->fireItemAvailable("archiveIntervalDQM", &_archiveIntervalDQM);
    infoSpace->fireItemAvailable("filePrefixDQM", &_filePrefixDQM);
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
 
  void Configuration::updateLocalDataRetrieverData()
  {
    stor::utils::getStdVector(_smRegistrationList, _dataRetrieverParamCopy._smRegistrationList);
    _dataRetrieverParamCopy._allowMissingSM = _allowMissingSM;
    _dataRetrieverParamCopy._maxConnectionRetries = _maxConnectionRetries;
    _dataRetrieverParamCopy._connectTrySleepTime = _connectTrySleepTime;
    _dataRetrieverParamCopy._headerRetryInterval = _headerRetryInterval;
    _dataRetrieverParamCopy._retryInterval = _retryInterval;
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

  void Configuration::updateLocalDQMProcessingData()
  {
    _dqmProcessingParamCopy._collateDQM = _collateDQM;
    _dqmProcessingParamCopy._readyTimeDQM =
      boost::posix_time::seconds( static_cast<int>(_readyTimeDQM) );
    _dqmProcessingParamCopy._useCompressionDQM = _useCompressionDQM;
    _dqmProcessingParamCopy._compressionLevelDQM = _compressionLevelDQM;
  }

  void Configuration::updateLocalDQMArchivingData()
  {
    _dqmArchivingParamCopy._archiveDQM = _archiveDQM;
    _dqmArchivingParamCopy._archiveIntervalDQM = _archiveIntervalDQM;
    _dqmArchivingParamCopy._filePrefixDQM = _filePrefixDQM;
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
