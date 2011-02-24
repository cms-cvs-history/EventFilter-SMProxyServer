// $Id: EventMsg.cc,v 1.1.2.2 2011/02/07 13:13:44 mommsen Exp $
/// @file: EventMsg.cc

#include "EventFilter/SMProxyServer/interface/EventMsg.h"


namespace smproxy
{

  EventMsg::EventMsg() :
  _faulty(true)
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
  }
  
  
  void EventMsg::tagForEventConsumers(const stor::QueueIDs& queueIDs)
  {
    _queueIDs = queueIDs;
  }
  
  
  const stor::QueueIDs& EventMsg::getEventConsumerTags() const
  {
    return _queueIDs;
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
