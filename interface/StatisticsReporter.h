// $Id: StatisticsReporter.h,v 1.13 2010/04/12 15:25:26 mommsen Exp $
/// @file: StatisticsReporter.h 

#ifndef SMProxyServer_StatisticsReporter_h
#define SMProxyServer_StatisticsReporter_h

#include "toolbox/lang/Class.h"
#include "toolbox/task/WaitingWorkLoop.h"
#include "xdaq/Application.h"
#include "xdata/InfoSpace.h"

#include "EventFilter/SMProxyServer/interface/Configuration.h"
#include "EventFilter/StorageManager/interface/AlarmHandler.h"
#include "EventFilter/StorageManager/interface/EventConsumerMonitorCollection.h"
#include "EventFilter/StorageManager/interface/DQMConsumerMonitorCollection.h"
#include "EventFilter/StorageManager/interface/DQMEventMonitorCollection.h"
#include "EventFilter/StorageManager/interface/Utils.h"

#include "boost/shared_ptr.hpp"
#include "boost/thread/mutex.hpp"

#include <string>
#include <list>
#include <vector>
#include <utility>


namespace smproxy {

  /**
   * Singleton to keep track of all monitoring and statistics issues
   *
   * This class also starts the monitoring workloop to update the 
   * statistics for all MonitorCollections.
   *
   * $Author: mommsen $
   * $Revision: 1.13 $
   * $Date: 2010/04/12 15:25:26 $
   */
  
  class StatisticsReporter : public toolbox::lang::Class, public xdata::ActionListener
  {
  public:
    
    explicit StatisticsReporter
    (
      xdaq::Application*,
      const QueueConfigurationParams&
    );
    
    virtual ~StatisticsReporter();

    const stor::DQMEventMonitorCollection& getDQMEventMonitorCollection() const
    { return _dqmEventMonCollection; }

    stor::DQMEventMonitorCollection& getDQMEventMonitorCollection()
    { return _dqmEventMonCollection; }


    const stor::EventConsumerMonitorCollection& getEventConsumerMonitorCollection() const
    { return _eventConsumerMonCollection; }

    stor::EventConsumerMonitorCollection& getEventConsumerMonitorCollection()
    { return _eventConsumerMonCollection; }


    const stor::DQMConsumerMonitorCollection& getDQMConsumerMonitorCollection() const
    { return _dqmConsumerMonCollection; }

    stor::DQMConsumerMonitorCollection& getDQMConsumerMonitorCollection()
    { return _dqmConsumerMonCollection; }


    /**
     * Create and start the monitoring workloop
     */
    void startWorkLoop(std::string workloopName);

    /**
     * Reset all monitored quantities
     */
    void reset();

    /**
     * Access alarm handler
     */
    stor::AlarmHandlerPtr alarmHandler() { return _alarmHandler; }

    /**
     * Update the variables put into the application info space
     */
    virtual void actionPerformed(xdata::Event&);


  private:

    typedef std::list<std::string> InfoSpaceItemNames;

    //Prevent copying of the StatisticsReporter
    StatisticsReporter(StatisticsReporter const&);
    StatisticsReporter& operator=(StatisticsReporter const&);

    void createMonitoringInfoSpace();
    void collectInfoSpaceItems();
    void putItemsIntoInfoSpace(stor::MonitorCollection::InfoSpaceItems&);
    bool monitorAction(toolbox::task::WorkLoop*);
    void calculateStatistics();
    void updateInfoSpace();

    xdaq::Application* _app;
    stor::AlarmHandlerPtr _alarmHandler;
    stor::utils::duration_t _monitoringSleepSec;
    stor::utils::time_point_t _lastMonitorAction;

    stor::DQMEventMonitorCollection _dqmEventMonCollection;
    stor::EventConsumerMonitorCollection _eventConsumerMonCollection;
    stor::DQMConsumerMonitorCollection _dqmConsumerMonCollection;
    toolbox::task::WorkLoop* _monitorWL;      
    bool _doMonitoring;

    // Stuff dealing with the monitoring info space
    xdata::InfoSpace *_infoSpace;
    InfoSpaceItemNames _infoSpaceItemNames;

  };

  typedef boost::shared_ptr<StatisticsReporter> StatisticsReporterPtr;
  
} // namespace smproxy

#endif // SMProxyServer_StatisticsReporter_h 


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
