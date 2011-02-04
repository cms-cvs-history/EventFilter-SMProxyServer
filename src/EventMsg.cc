// $Id: EventMsg.cc,v 1.1.2.10 2011/01/27 16:33:12 mommsen Exp $
/// @file: EventMsg.cc

#include "EventFilter/SMProxyServer/interface/EventMsg.h"


namespace smproxy
{

  EventMsg::EventMsg() :
  _faulty(true),
  _droppedEventsCount(0)
  {}
  
  
  EventMsg::EventMsg(const EventMsgView& eventMsgView) :
  _faulty(false)
  {
    _buf.reset( new EventMsgBuffer(eventMsgView.size()) );
    std::copy(
      eventMsgView.startAddress(),
      eventMsgView.startAddress()+eventMsgView.size(),
      &(*_buf)[0]
    );
    _droppedEventsCount = eventMsgView.droppedEventsCount();
  }
  
  
  void EventMsg::tagForEventConsumers(const std::vector<stor::QueueID>& queueIDs)
  {
    _queueIDs = queueIDs;
  }
  
  
  const std::vector<stor::QueueID>& EventMsg::getEventConsumerTags() const
  {
    return _queueIDs;
  }
  
  
  unsigned int EventMsg::droppedEventsCount() const
  {
    return _droppedEventsCount;
  }
  
  
  void EventMsg::addDroppedEventsCount(size_t count)
  {
    EventHeader* header = (EventHeader*)dataLocation();
    _droppedEventsCount += count;
    if ( header->protocolVersion_ >= 9 )
      convert(_droppedEventsCount,header->droppedEventsCount_);
  }
  
  
  size_t EventMsg::memoryUsed() const
  {
    return sizeof(_buf);
  }
  
  
  unsigned long EventMsg::totalDataSize() const
  {
    return _buf->size();
  }
  
  
  unsigned char* EventMsg::dataLocation() const
  {
    return &(*_buf)[0];
  }
  
  
  bool EventMsg::empty() const
  {
    return (_buf.get() == 0);
  }


  bool EventMsg::faulty() const
  {
    return _faulty;
  }

} // namespace smproxy
  
/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
