// $Id: SMPSWebPageHelper.cc,v 1.1.2.5 2011/01/24 14:32:56 mommsen Exp $
/// @file: SMPSWebPageHelper.cc

#include "EventFilter/SMProxyServer/interface/SMPSWebPageHelper.h"


namespace smproxy
{
  SMPSWebPageHelper::SMPSWebPageHelper
  (
    xdaq::ApplicationDescriptor* appDesc,
    StateMachinePtr stateMachine
  ) :
  stor::WebPageHelper(appDesc),
  _stateMachine(stateMachine),
  _consumerWebPageHelper(appDesc)
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
