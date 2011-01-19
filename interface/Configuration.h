// $Id: Configuration.h,v 1.1.2.1 2011/01/18 15:32:34 mommsen Exp $
/// @file: Configuration.h 

#ifndef SMProxyServer_Configuration_h
#define SMProxyServer_Configuration_h

#include "EventFilter/StorageManager/interface/Utils.h"

#include "xdata/InfoSpace.h"
#include "xdata/String.h"
//#include "xdata/Integer.h"
#include "xdata/UnsignedInteger32.h"
#include "xdata/Double.h"
//#include "xdata/Boolean.h"
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
    stor::utils::duration_t _sleepTimeIfIdle;

    // not mapped to infospace params
    uint32_t _smpsInstance;
    std::string _hostName;
  };

  /**
   * Class for managing configuration information from the infospace
   * and providing local copies of that information that are updated
   * only at requested times.
   *
   * $Author: mommsen $
   * $Revision: 1.1.2.1 $
   * $Date: 2011/01/18 15:32:34 $
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
     * Updates the local copy of all configuration parameters from
     * the infospace.
     */
    void updateAllParams();

    /**
     * Updates the local copy of the run configuration
     * parameters from the infospace.
     */
    void updateRunParams();

    /**
     * Updates the local copy of the event retriever configuration
     * parameters from the infospace.
     */
    void updateDataRetrieverParams();

  private:

    void setDataRetrieverDefaults(unsigned long instanceNumber);

    void setupRunInfoSpaceParams(xdata::InfoSpace* infoSpace);
    void setupDataRetrieverInfoSpaceParams(xdata::InfoSpace* infoSpace);

    void updateLocalRunData();
    void updateLocalDataRetrieverData();

    struct DataRetrieverParams _dataRetrieverParamCopy;

    mutable boost::mutex _generalMutex;

    xdata::UnsignedInteger32 _infospaceRunNumber;
    unsigned int _localRunNumber;

    xdata::Vector<xdata::String> _smRegistrationList;
    xdata::Double _sleepTimeIfIdle;
  };

} // namespace smproxy

#endif // SMProxyServer_Configuration_h


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -

