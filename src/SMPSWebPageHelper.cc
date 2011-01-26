// $Id: SMPSWebPageHelper.cc,v 1.1.2.2 2011/01/25 17:22:24 mommsen Exp $
/// @file: SMPSWebPageHelper.cc

#include "EventFilter/SMProxyServer/interface/SMPSWebPageHelper.h"
#include "EventFilter/StorageManager/src/ConsumerWebPageHelper.icc"


namespace smproxy
{
  SMPSWebPageHelper::SMPSWebPageHelper
  (
    xdaq::ApplicationDescriptor* appDesc,
    StateMachinePtr stateMachine
  ) :
  stor::WebPageHelper(appDesc, "$Name:  $"),
  _stateMachine(stateMachine),
  _consumerWebPageHelper(appDesc, "$Name:  $")
  { }
  
  
  void SMPSWebPageHelper::consumerStatistics(xgi::Output* out)
  {
    _consumerWebPageHelper.consumerStatistics(out,
      _stateMachine->getExternallyVisibleStateName(),
      _stateMachine->getStateName(),
      _stateMachine->getReasonForFailed(),
      _stateMachine->getStatisticsReporter(),
      _stateMachine->getRegistrationCollection(),
      _stateMachine->getEventQueueCollection(),
      _stateMachine->getDQMEventQueueCollection()
    );
  }

} // namespace smproxy
  
/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
