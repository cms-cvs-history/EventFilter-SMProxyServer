// $Id: Configuration.cc,v 1.1.2.1 2011/01/18 15:32:34 mommsen Exp $
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

    setupDataRetrieverInfoSpaceParams(infoSpace);
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

  void Configuration::updateAllParams()
  {
    boost::mutex::scoped_lock sl(_generalMutex);
    updateLocalRunData();
    updateLocalDataRetrieverData();
  }

  void Configuration::updateRunParams()
  {
    boost::mutex::scoped_lock sl(_generalMutex);
    updateLocalRunData();
  }

  void Configuration::updateDataRetrieverParams()
  {
    boost::mutex::scoped_lock sl(_generalMutex);
    updateLocalDataRetrieverData();
  }

  void Configuration::setDataRetrieverDefaults(unsigned long instanceNumber)
  {
    _dataRetrieverParamCopy._smpsInstance = instanceNumber;
    _dataRetrieverParamCopy._smRegistrationList.clear();
    _dataRetrieverParamCopy._sleepTimeIfIdle =
      boost::posix_time::seconds(1);

    std::string tmpString(toolbox::net::getHostName());
    // strip domainame
    std::string::size_type pos = tmpString.find('.');  
    if (pos != std::string::npos) {  
      std::string basename = tmpString.substr(0,pos);  
      tmpString = basename;
    }
    _dataRetrieverParamCopy._hostName = tmpString;
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
    _sleepTimeIfIdle =
      stor::utils::duration_to_seconds(_dataRetrieverParamCopy._sleepTimeIfIdle);

    // bind the local xdata variables to the infospace
    infoSpace->fireItemAvailable("SMRegistrationList", &_smRegistrationList);
    infoSpace->fireItemAvailable("SleepTimeIfIdle", &_sleepTimeIfIdle);
  }
  
  void Configuration::updateLocalRunData()
  {
    _localRunNumber = _infospaceRunNumber;
  }

  void Configuration::updateLocalDataRetrieverData()
  {
    stor::utils::getStdVector(_smRegistrationList, _dataRetrieverParamCopy._smRegistrationList);
    _dataRetrieverParamCopy._sleepTimeIfIdle =
      stor::utils::seconds_to_duration(_sleepTimeIfIdle);
  }

} // namespace smproxy

/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
