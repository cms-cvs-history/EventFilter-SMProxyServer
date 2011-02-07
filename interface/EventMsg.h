// $Id: EventMsg.h,v 1.1.2.1 2011/02/04 13:47:09 mommsen Exp $
/// @file: EventMsg.h 

#ifndef EventFilter_SMProxyServer_EventMsg_h
#define EventFilter_SMProxyServer_EventMsg_h

#include "EventFilter/StorageManager/interface/I2OChain.h"
#include "EventFilter/StorageManager/interface/QueueID.h"
#include "IOPool/Streamer/interface/EventMessage.h"

#include <boost/shared_ptr.hpp>
#include <vector>

namespace smproxy {

  /**
   * A class for storing an EventMsgView and providing the basic 
   * APIs required for SMPS
   *
   * $Author: mommsen $
   * $Revision: 1.1.2.1 $
   * $Date: 2011/02/04 13:47:09 $
   */

  class EventMsg
  {
  public:
    
    EventMsg();
    EventMsg(const EventMsgView&);

    /**
      Tag the event for the passed list of queueIDs
     */
    void tagForEventConsumers(const std::vector<stor::QueueID>&);

    /**
      Return the QueueIDs for which the event is tagged
     */
    const std::vector<stor::QueueID>& getEventConsumerTags() const;

    /**
      Returns the total memory occupied by the event message
     */
    size_t memoryUsed() const;
    
    /**
      Returns the size of the event message
     */
    unsigned long totalDataSize() const;

    /**
      Returns the start adderess of the event message
     */
    unsigned char* dataLocation() const;

    /**
      Returns true if no event message is managed by *this
     */
    bool empty() const;

    /**
      Returns true if the event message is faulty
     */
    bool faulty() const;


  private:
    typedef std::vector<unsigned char> EventMsgBuffer;
    boost::shared_ptr<EventMsgBuffer> _buf;
    bool _faulty;

    std::vector<stor::QueueID> _queueIDs;
  };
  
} // namespace smproxy

#endif // EventFilter_SMProxyServer_EventMsg_h 



// emacs configuration
// Local Variables: -
// mode: c++ -
// c-basic-offset: 2 -
// indent-tabs-mode: nil -
// End: -