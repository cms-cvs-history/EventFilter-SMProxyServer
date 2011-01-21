// $Id: Exception.h,v 1.1.2.1 2011/01/20 10:27:22 mommsen Exp $
/// @file: Exception.h 

#ifndef SMProxyServer_Exception_h
#define SMProxyServer_Exception_h


#include "xcept/Exception.h"

namespace smproxy {

  /**
     List of exceptions thrown by the SMProxyServer

     $Author: mommsen $
     $Revision: 1.1.2.1 $
     $Date: 2011/01/20 10:27:22 $
  */
}

/**
 * Generic exception raised by the storage manager proxy server
 */
XCEPT_DEFINE_EXCEPTION(smproxy, Exception)

/**
 * Exception raised in case of a finite state machine error
 */
XCEPT_DEFINE_EXCEPTION(smproxy, StateMachine)

/**
 * Exception raised in case of a monitoring error
 */
XCEPT_DEFINE_EXCEPTION(smproxy, Monitoring)

/**
 * Exception raised in case of problems accessing the info space
 */
XCEPT_DEFINE_EXCEPTION(smproxy, Infospace)

/**
 * Exception raised in case of configuration problems
 */
XCEPT_DEFINE_EXCEPTION(smproxy, Configuration)

/**
 * Exception raised when a non-existant queue is requested.
 */
XCEPT_DEFINE_EXCEPTION(smproxy, UnknownQueueId)

/**
 * Consumer registration exception
 */
XCEPT_DEFINE_EXCEPTION(smproxy, ConsumerRegistration)

/**
 * DQM consumer registration exception
 */
XCEPT_DEFINE_EXCEPTION(smproxy, DQMConsumerRegistration)

/**
 * State transition error
 */
XCEPT_DEFINE_EXCEPTION(smproxy, StateTransition)


#endif // SMProxyServer_Exception_h


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
