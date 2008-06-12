#ifndef SMPS_CONSUMER_PROXY_INFO_H
#define SMPS_CONSUMER_PROXY_INFO_H

/**
 * This class contains information about a consumer that the proxy server
 * uses when forwarding the subscription to all of the upstream storage
 * managers.  It helps manage the lists of registrations and headers, and
 * helps determine which SM to contact next for an event.
 *
 * $Id: ConsumerProxyInfo.h,v 1.13 2008/04/16 16:10:20 biery Exp $
 */

#include "IOPool/Streamer/interface/MsgTools.h"
#include "EventFilter/StorageManager/interface/BaseCounter.h"
#include "EventFilter/StorageManager/interface/RollingSampleCounter.h"
#include "boost/thread/mutex.hpp"
#include <vector>

namespace stor
{
  class ConsumerProxyInfo
  {
  public:
    ConsumerProxyInfo(uint32 localConsumerId, double rateRequest,
                      const std::vector<std::string>& smList,
                      int maxRegistrationAttempts, int maxHeaderFetchAttempts);

    uint32 getLocalConsumerId() { return localConsumerId_; }
    void addSM(std::string smURL);
    void updateRateRequest(double newRate);

    bool needsRegistration() { return needsRegistration_; }
    std::vector<std::string> getSMRegistrationList();
    void setRegistrationSuccess(std::string smURL, uint32 remoteConsumerId);
    bool isReadyForHeaders() { return successfulRegistrationCount_ > 0; }
    uint32 getRemoteConsumerId(std::string smURL);

    bool needsHeaders() { return needsHeaders_; }
    std::vector< std::pair<std::string, uint32> > getSMHeaderList();
    void setHeaderFetchSuccess(std::string smURL);
    bool hasHeaderFromSM(std::string smURL);
    bool isReadyForEvents() { return successfulHeaderCount_ > 0; }

    double getTimeToWaitForEvent(double now = BaseCounter::getCurrentTime());
    std::pair<std::string, uint32> getNextSMForEvent();
    void setEventFetchSuccess(std::string smURL,
                              double now = BaseCounter::getCurrentTime());

  private:
    uint32 localConsumerId_;
    std::vector<std::string> smList_;
    int maxRegistrationAttempts_;
    int maxHeaderFetchAttempts_;

    bool needsRegistration_;
    int registrationAttemptCount_;
    std::vector<uint32> remoteConsumerIds_;
    int successfulRegistrationCount_;

    bool needsHeaders_;
    int headerAttemptCount_;
    std::vector<bool> headerSuccessFlags_;
    int successfulHeaderCount_;

    double minTimeBetweenEvents_;
    double lastEventSuccessTime_;
    boost::shared_ptr<RollingSampleCounter> rateRequestCounter_;

    int nextSMEventIndex_;
    int runningEventAttemptCount_;

    boost::mutex dataMutex_;
  };
} 

#endif
