// $Id: EventQueueCollection.h,v 1.1.2.2 2011/01/21 15:54:56 mommsen Exp $
/// @file: EventQueueCollection.h 

#ifndef EventFilter_SMProxyServer_EventQueueCollection_h
#define EventFilter_SMProxyServer_EventQueueCollection_h

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
   * $Revision: 1.1.2.2 $
   * $Date: 2011/01/21 15:54:56 $
   */

  class EventMsg
  {
  public:
    EventMsg() : _faulty(true) {};
    EventMsg(const EventMsgView& eventMsgView) :
    _faulty(false)
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

    unsigned int fragmentCount() const
    { return 1; }

    unsigned long totalDataSize() const
    { return _buf->size(); }

    unsigned long dataSize(int fragmentIndex) const
    { return _buf->size(); }

    unsigned char* dataLocation(int fragmentIndex) const
    { return &(*_buf)[0]; }

    bool empty() const
    { return (_buf.get() == 0); }

    bool faulty() const
    { return _faulty; }

  private:
    typedef std::vector<unsigned char> EventMsgBuffer;
    boost::shared_ptr<EventMsgBuffer> _buf;
    bool _faulty;

    std::vector<stor::QueueID> _queueIDs;
  };
  
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
