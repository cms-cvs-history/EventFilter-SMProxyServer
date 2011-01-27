// $Id: Configuration.h,v 1.1.2.5 2011/01/26 16:06:54 mommsen Exp $
/// @file: Configuration.h 

#ifndef EventFilter_SMProxyServer_Configuration_h
#define EventFilter_SMProxyServer_Configuration_h

#include "EventFilter/StorageManager/interface/Configuration.h"
#include "EventFilter/StorageManager/interface/Utils.h"

#include "xdata/InfoSpace.h"
#include "xdata/String.h"
#include "xdata/Integer.h"
#include "xdata/UnsignedInteger32.h"
#include "xdata/Double.h"
#include "xdata/Boolean.h"
#include "xdata/Vector.h"

#include "boost/thread/mutex.hpp"


namespace smproxy
{
  /**
   * Data structure to hold configuration parameters
   * that are relevant for retrieving events from the SM
   */
  struct DataRetrieverParams
  {
    typedef std::vector<std::string> SMRegistrationList;
    SMRegistrationList _smRegistrationList;
    bool _allowMissingSM;
    uint32_t _maxConnectionRetries;
    uint32_t _connectTrySleepTime;
    uint32_t _headerRetryInterval;
    stor::utils::duration_t _sleepTimeIfIdle;

    // not mapped to infospace params
    uint32_t _smpsInstance;
    std::string _hostName;
  };

  /**
   * Data structure to hold configuration parameters
   * that are used for the various queues in the system.
   */
  struct QueueConfigurationParams
  {
    uint32_t _registrationQueueSize;
    stor::utils::duration_t _monitoringSleepSec;
  };

  /**
   * Class for managing configuration information from the infospace
   * and providing local copies of that information that are updated
   * only at requested times.
   *
   * $Author: mommsen $
   * $Revision: 1.1.2.5 $
   * $Date: 2011/01/26 16:06:54 $
   */

  class Configuration : public xdata::ActionListener
  {
  public:

    /**
     * Constructs a Configuration instance for the specified infospace
     * and application instance number.
     */
    Configuration(xdata::InfoSpace* infoSpace, unsigned long instanceNumber);

    /**
     * Destructor.
     */
    virtual ~Configuration()
    {
      // should we detach from the infospace???
    }

    /**
     * Get run number:
     */
    unsigned int getRunNumber() const;

    /**
     * Returns a copy of the event retriever parameters. These values
     * will be current as of the most recent global update of the local
     * cache from the infospace (see the updateAllParams() method) or
     * the most recent update of only the event retrieved parameters
     * (see the updateDataRetrieverParams() method).
     */
    struct DataRetrieverParams getDataRetrieverParams() const;

    /**
     * Returns a copy of the DQM processing parameters.  These values
     * will be current as of the most recent global update of the local
     * cache from the infospace (see the updateAllParams() method).
     */
    struct stor::DQMProcessingParams getDQMProcessingParams() const;

    /**
     * Returns a copy of the event serving parameters.  These values
     * will be current as of the most recent global update of the local
     * cache from the infospace (see the updateAllParams() method).
     */
    struct stor::EventServingParams getEventServingParams() const;

    /**
     * Returns a copy of the queue configuration parameters.  These values
     * will be current as of the most recent global update of the local
     * cache from the infospace (see the updateAllParams() method).
     */
    struct QueueConfigurationParams getQueueConfigurationParams() const;

    /**
     * Updates the local copy of all configuration parameters from
     * the infospace.
     */
    void updateAllParams();

    /**
     * Gets invoked when a operation is performed on the infospace
     * that we are interested in knowing about.
     */
    virtual void actionPerformed(xdata::Event& isEvt);


  private:

    void setDataRetrieverDefaults(unsigned long instanceNumber);
    void setDQMProcessingDefaults();
    void setEventServingDefaults();
    void setQueueConfigurationDefaults();

    void setupRunInfoSpaceParams(xdata::InfoSpace*);
    void setupDataRetrieverInfoSpaceParams(xdata::InfoSpace*);
    void setupDQMProcessingInfoSpaceParams(xdata::InfoSpace*);
    void setupEventServingInfoSpaceParams(xdata::InfoSpace*);
    void setupQueueConfigurationInfoSpaceParams(xdata::InfoSpace*);

    void updateLocalRunData();
    void updateLocalDataRetrieverData();
    void updateLocalDQMProcessingData();
    void updateLocalEventServingData();
    void updateLocalQueueConfigurationData();

    struct DataRetrieverParams _dataRetrieverParamCopy;
    struct stor::DQMProcessingParams _dqmParamCopy;
    struct stor::EventServingParams _eventServeParamCopy;
    struct QueueConfigurationParams _queueConfigParamCopy;

    mutable boost::mutex _generalMutex;
    
    xdata::UnsignedInteger32 _infospaceRunNumber;
    unsigned int _localRunNumber;
    
    xdata::Vector<xdata::String> _smRegistrationList;
    xdata::Boolean _allowMissingSM;
    xdata::UnsignedInteger32 _maxConnectionRetries;
    xdata::UnsignedInteger32 _connectTrySleepTime; // seconds
    xdata::UnsignedInteger32 _headerRetryInterval; // seconds
    xdata::UnsignedInteger32 _sleepTimeIfIdle;  // milliseconds

    xdata::Boolean _collateDQM;
    xdata::Boolean _archiveDQM;
    xdata::Integer _archiveIntervalDQM;  // lumi sections
    xdata::String  _filePrefixDQM;
    xdata::Integer _purgeTimeDQM;  // seconds
    xdata::Integer _readyTimeDQM;  // seconds
    xdata::Boolean _useCompressionDQM;
    xdata::Integer _compressionLevelDQM;
    
    xdata::Integer _activeConsumerTimeout;  // seconds
    xdata::Integer _consumerQueueSize;
    xdata::String  _consumerQueuePolicy;
    xdata::Integer _DQMactiveConsumerTimeout;  // seconds
    xdata::Integer _DQMconsumerQueueSize;
    xdata::String  _DQMconsumerQueuePolicy;
    
    xdata::UnsignedInteger32 _registrationQueueSize;
    xdata::Double _monitoringSleepSec;  // seconds

  };

  typedef boost::shared_ptr<Configuration> ConfigurationPtr;

} // namespace smproxy

#endif // EventFilter_SMProxyServer_Configuration_h


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -

