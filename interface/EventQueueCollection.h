// $Id: EventQueueCollection.h,v 1.3 2009/07/20 13:06:10 mommsen Exp $
/// @file: EventQueueCollection.h 

#ifndef SMProxyServer_EventQueueCollection_h
#define SMProxyServer_EventQueueCollection_h

#include "EventFilter/StorageManager/interface/I2OChain.h"
#include "EventFilter/StorageManager/interface/QueueCollection.h"
#include "EventFilter/StorageManager/interface/QueueID.h"
#include "IOPool/Streamer/interface/EventMessage.h"

#include <boost/shared_ptr.hpp>
#include <vector>

namespace smproxy {

  /**
   * A collection of ConcurrentQueue<EventMsgSharedPtr>.
   *
   * $Author: mommsen $
   * $Revision: 1.3 $
   * $Date: 2009/07/20 13:06:10 $
   */

  class EventMsg
  {
  public:
    EventMsg() {};
    EventMsg(const EventMsgView& eventMsgView)
    {
      _buf.reset( new EventMsgBuffer(eventMsgView.size()) );
      std::copy(
        eventMsgView.startAddress(),
        eventMsgView.startAddress()+eventMsgView.size(),
        &(*_buf)[0]
      );
    }

    void tagForEventConsumers(const std::vector<stor::QueueID>& queueIDs)
    { _queueIDs = queueIDs; }

    std::vector<stor::QueueID> getEventConsumerTags() const
    { return _queueIDs; }

    unsigned long totalDataSize() const
    { return _buf->size(); }

  private:
    typedef std::vector<unsigned char> EventMsgBuffer;
    boost::shared_ptr<EventMsgBuffer> _buf;

    std::vector<stor::QueueID> _queueIDs;
  };
  
  typedef stor::QueueCollection<EventMsg> EventQueueCollection;
  
} // namespace smproxy

#endif // SMProxyServer_EventQueueCollection_h 



// emacs configuration
// Local Variables: -
// mode: c++ -
// c-basic-offset: 2 -
// indent-tabs-mode: nil -
// End: -
