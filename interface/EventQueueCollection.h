// $Id: EventQueueCollection.h,v 1.1.2.3 2011/01/24 12:43:17 mommsen Exp $
/// @file: EventQueueCollection.h 

#ifndef EventFilter_SMProxyServer_EventQueueCollection_h
#define EventFilter_SMProxyServer_EventQueueCollection_h

#include "EventFilter/SMProxyServer/interface/EventMsg.h"
#include "EventFilter/StorageManager/interface/QueueCollection.h"

#include <boost/shared_ptr.hpp>

namespace smproxy {

  /**
   * A collection of ConcurrentQueue<EventMsgSharedPtr>.
   *
   * $Author: mommsen $
   * $Revision: 1.1.2.3 $
   * $Date: 2011/01/24 12:43:17 $
   */
  
  typedef stor::QueueCollection<EventMsg> EventQueueCollection;
  typedef boost::shared_ptr<EventQueueCollection> EventQueueCollectionPtr;
  
} // namespace smproxy

#endif // EventFilter_SMProxyServer_EventQueueCollection_h 



// emacs configuration
// Local Variables: -
// mode: c++ -
// c-basic-offset: 2 -
// indent-tabs-mode: nil -
// End: -
