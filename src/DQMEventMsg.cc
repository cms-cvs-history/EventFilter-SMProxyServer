// $Id: DQMEventMsg.cc,v 1.1.2.3 2011/02/24 10:59:26 mommsen Exp $
/// @file: DQMEventMsg.cc

#include "EventFilter/SMProxyServer/interface/DQMEventMsg.h"


namespace smproxy
{

  DQMEventMsg::DQMEventMsg() :
  _faulty(true)
  {}
  
  
  DQMEventMsg::DQMEventMsg(const DQMEventMsgView& dqmEventMsgView) :
  _faulty(false)
  {
    _buf.reset( new DQMEventMsgBuffer(dqmEventMsgView.size()) );
    std::copy(
      dqmEventMsgView.startAddress(),
      dqmEventMsgView.startAddress()+dqmEventMsgView.size(),
      &(*_buf)[0]
    );
    _dqmKey.runNumber = dqmEventMsgView.runNumber();
    _dqmKey.lumiSection = dqmEventMsgView.lumiSection();
    _dqmKey.topLevelFolderName = dqmEventMsgView.topFolderName();
  }
  
  
  void DQMEventMsg::tagForDQMEventConsumers(const stor::QueueIDs& queueIDs)
  {
    _queueIDs = queueIDs;
  }
  
  
  const stor::QueueIDs& DQMEventMsg::getDQMEventConsumerTags() const
  {
    return _queueIDs;
  }
  
  
  const stor::DQMKey& DQMEventMsg::dqmKey() const
  {
    return _dqmKey;
  }

  
  size_t DQMEventMsg::memoryUsed() const
  {
    return sizeof(_buf);
  }
  
  
  unsigned long DQMEventMsg::totalDataSize() const
  {
    return _buf->size();
  }
  
  
  unsigned char* DQMEventMsg::dataLocation() const
  {
    return &(*_buf)[0];
  }
  
  
  bool DQMEventMsg::empty() const
  {
    return (_buf.get() == 0);
  }


  bool DQMEventMsg::faulty() const
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
