// $Id: Configuration.cc,v 1.1.2.6 2011/02/11 16:44:52 mommsen Exp $
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
    setDQMProcessingDefaults();
    setEventServingDefaults();
    setQueueConfigurationDefaults();

    setupDataRetrieverInfoSpaceParams(infoSpace);
    setupDQMProcessingInfoSpaceParams(infoSpace);
    setupEventServingInfoSpaceParams(infoSpace);
    setupQueueConfigurationInfoSpaceParams(infoSpace);
  }

  struct DataRetrieverParams Configuration::getDataRetrieverParams() const
  {
    boost::mutex::scoped_lock sl(_generalMutex);
    return _dataRetrieverParamCopy;
  }

  struct stor::DQMProcessingParams Configuration::getDQMProcessingParams() const
  {
    boost::mutex::scoped_lock sl(_generalMutex);
    return _dqmParamCopy;
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
    updateLocalDataRetrieverData();
    updateLocalDQMProcessingData();
    updateLocalEventServingData();
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

  void Configuration::setDQMProcessingDefaults()
  {
    _dqmParamCopy._collateDQM = false;
    _dqmParamCopy._archiveDQM = false;
    _dqmParamCopy._filePrefixDQM = "/tmp/DQM";
    _dqmParamCopy._archiveIntervalDQM = 0;
    _dqmParamCopy._purgeTimeDQM = boost::posix_time::seconds(300);
    _dqmParamCopy._readyTimeDQM = boost::posix_time::seconds(120);
    _dqmParamCopy._useCompressionDQM = true;
    _dqmParamCopy._compressionLevelDQM = 1;
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
    _sleepTimeIfIdle = _dataRetrieverParamCopy._sleepTimeIfIdle.total_milliseconds();

    // bind the local xdata variables to the infospace
    infoSpace->fireItemAvailable("SMRegistrationList", &_smRegistrationList);
    infoSpace->fireItemAvailable("allowMissingSM", &_allowMissingSM);
    infoSpace->fireItemAvailable("maxConnectionRetries", &_maxConnectionRetries);
    infoSpace->fireItemAvailable("connectTrySleepTime", &_connectTrySleepTime);
    infoSpace->fireItemAvailable("headerRetryInterval", &_headerRetryInterval);
    infoSpace->fireItemAvailable("sleepTimeIfIdle", &_sleepTimeIfIdle);
  }

  void Configuration::
  setupDQMProcessingInfoSpaceParams(xdata::InfoSpace* infoSpace)
  {
    // copy the initial defaults to the xdata variables
    _collateDQM = _dqmParamCopy._collateDQM;
    _archiveDQM = _dqmParamCopy._archiveDQM;
    _archiveIntervalDQM = _dqmParamCopy._archiveIntervalDQM;
    _filePrefixDQM = _dqmParamCopy._filePrefixDQM;
    _purgeTimeDQM = _dqmParamCopy._purgeTimeDQM.total_seconds();
    _readyTimeDQM = _dqmParamCopy._readyTimeDQM.total_seconds();
    _useCompressionDQM = _dqmParamCopy._useCompressionDQM;
    _compressionLevelDQM = _dqmParamCopy._compressionLevelDQM;

    // bind the local xdata variables to the infospace
    infoSpace->fireItemAvailable("collateDQM", &_collateDQM);
    infoSpace->fireItemAvailable("archiveDQM", &_archiveDQM);
    infoSpace->fireItemAvailable("archiveIntervalDQM", &_archiveIntervalDQM);
    infoSpace->fireItemAvailable("purgeTimeDQM", &_purgeTimeDQM);
    infoSpace->fireItemAvailable("readyTimeDQM", &_readyTimeDQM);
    infoSpace->fireItemAvailable("filePrefixDQM", &_filePrefixDQM);
    infoSpace->fireItemAvailable("useCompressionDQM", &_useCompressionDQM);
    infoSpace->fireItemAvailable("compressionLevelDQM", &_compressionLevelDQM);
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
 
  void Configuration::updateLocalDataRetrieverData()
  {
    stor::utils::getStdVector(_smRegistrationList, _dataRetrieverParamCopy._smRegistrationList);
    _dataRetrieverParamCopy._allowMissingSM = _allowMissingSM;
    _dataRetrieverParamCopy._maxConnectionRetries = _maxConnectionRetries;
    _dataRetrieverParamCopy._connectTrySleepTime = _connectTrySleepTime;
    _dataRetrieverParamCopy._headerRetryInterval = _headerRetryInterval;
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
    _dqmParamCopy._collateDQM = _collateDQM;
    _dqmParamCopy._archiveDQM = _archiveDQM;
    _dqmParamCopy._archiveIntervalDQM = _archiveIntervalDQM;
    _dqmParamCopy._filePrefixDQM = _filePrefixDQM;
    _dqmParamCopy._purgeTimeDQM =
      boost::posix_time::seconds( static_cast<int>(_purgeTimeDQM) );
    _dqmParamCopy._readyTimeDQM =
      boost::posix_time::seconds( static_cast<int>(_readyTimeDQM) );
    _dqmParamCopy._useCompressionDQM = _useCompressionDQM;
    _dqmParamCopy._compressionLevelDQM = _compressionLevelDQM;

    // make sure that purge time is larger than ready time
    if ( _dqmParamCopy._purgeTimeDQM < _dqmParamCopy._readyTimeDQM )
    {
      _dqmParamCopy._purgeTimeDQM = _dqmParamCopy._readyTimeDQM + boost::posix_time::seconds(10);
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
