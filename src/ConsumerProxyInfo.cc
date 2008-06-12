/**
 * $Id: ConsumerProxyInfo.cc,v 1.1 2008/04/14 15:42:28 biery Exp $
 */

#include "EventFilter/SMProxyServer/interface/ConsumerProxyInfo.h"
#include "EventFilter/StorageManager/interface/ConsumerPipe.h"

using namespace stor;

/**
 * Constructor.
 */
ConsumerProxyInfo::ConsumerProxyInfo(uint32 localConsumerId,
                                     double rateRequest,
                                     const std::vector<std::string>& smList,
                                     int maxRegistrationAttempts,
                                     int maxHeaderFetchAttempts):
  localConsumerId_(localConsumerId),
  smList_(smList),
  maxRegistrationAttempts_(maxRegistrationAttempts),
  maxHeaderFetchAttempts_(maxHeaderFetchAttempts),
  needsRegistration_(true),
  registrationAttemptCount_(0),
  successfulRegistrationCount_(0),
  needsHeaders_(true),
  headerAttemptCount_(0),
  successfulHeaderCount_(0),
  lastEventSuccessTime_(0.0),
  nextSMEventIndex_(0),
  runningEventAttemptCount_(0)
{
  // initial the lists of data that are based on the SM list
  for (uint32 idx = 0; idx < smList_.size(); ++idx) {
    remoteConsumerIds_.push_back(ConsumerPipe::NULL_CONSUMER_ID);
    headerSuccessFlags_.push_back(false);
  }

  // determine the amount of time that we need to wait between
  // events.  The request rate specified to this constructor is
  // converted to an interval that is used internally, and the interval
  // is required to be somewhat reasonable.
  if (rateRequest < (1.0 / ConsumerPipe::MAX_ACCEPT_INTERVAL))
  {
    minTimeBetweenEvents_ = ConsumerPipe::MAX_ACCEPT_INTERVAL;
  }
  else
  {
    minTimeBetweenEvents_ = 1.0 / rateRequest;  // seconds
  }

  // a calculated time between individual events is a little
  // limiting in the sense that it can't handle a short burst followed
  // by a quiet time (and accept all of the events).  So, we also use
  // a longer time frame (the last 50 events) to test whether we are
  // ready to accept another event.
  rateRequestCounter_.reset(new RollingSampleCounter(50,1,60,RollingSampleCounter::INCLUDE_SAMPLES_IMMEDIATELY));
}

/**
 * Adds the specified storage manager to the list of storage
 * managers that we're communicating with.
 */
void ConsumerProxyInfo::addSM(std::string smURL)
{
  boost::mutex::scoped_lock sl(dataMutex_);

  // add the SM only if it is unique
  bool foundSM = false;
  for (int idx = 0; idx < static_cast<int>(smList_.size()); ++idx) {
    if (smList_[idx] == smURL) {
      foundSM = true;
      break;
    }
  }
  if (! foundSM) {
    smList_.push_back(smURL);

    // initialize dependent list elements
    remoteConsumerIds_.push_back(ConsumerPipe::NULL_CONSUMER_ID);
    headerSuccessFlags_.push_back(false);

    // reset the registration and header fetch flags
    needsRegistration_ = true;
    registrationAttemptCount_ = 0;
    needsHeaders_ = true;
    headerAttemptCount_ = 0;
  }
}

/**
 * Updates the rate request for the consumer.
 */
void ConsumerProxyInfo::updateRateRequest(double newRate)
{
  if (newRate < (1.0 / ConsumerPipe::MAX_ACCEPT_INTERVAL))
  {
    minTimeBetweenEvents_ = ConsumerPipe::MAX_ACCEPT_INTERVAL;
  }
  else
  {
    minTimeBetweenEvents_ = 1.0 / newRate;  // seconds
  }
}

/**
 * Returns the list of URLs for the storage managers that we still
 * need to register the consumer with.  If registration has already
 * been done for all of the storage managers, an empty list is returned.
 *
 * @see ConsumerProxyInfo::needsRegistration()
 */
std::vector<std::string> ConsumerProxyInfo::getSMRegistrationList()
{
  boost::mutex::scoped_lock sl(dataMutex_);

  // build the list of SMs that we haven't registered with yet
  std::vector<std::string> regList;
  for (uint32 idx = 0; idx < smList_.size(); ++idx) {
    if (remoteConsumerIds_[idx] == ConsumerPipe::NULL_CONSUMER_ID) {
      regList.push_back(smList_[idx]);
    }
  }

  // assume that the caller is using this information to attempt
  // to register with storage managers
  ++registrationAttemptCount_;

  // if we've exhausted the allowed number of attempts to register,
  // we'll arbitrarily say that registration is done
  if (registrationAttemptCount_ >= maxRegistrationAttempts_) {
    needsRegistration_ = false;
  }

  return regList;
}

/**
 * Indicates that the consumer was successfully registered with the
 * specified storage manager, and specifies the consumer ID assigned
 * by the remote storage manager.
 */
void ConsumerProxyInfo::setRegistrationSuccess(std::string smURL,
                                               uint32 remoteConsumerId)
{
  boost::mutex::scoped_lock sl(dataMutex_);

  // find the index of the specified SM
  int smIndex = -1;
  for (int idx = 0; idx < static_cast<int>(smList_.size()); ++idx) {
    if (smList_[idx] == smURL) {
      smIndex = idx;
      break;
    }
  }

  // update the status of the SM, if we found it
  if (smIndex >= 0 && smIndex < static_cast<int>(smList_.size())) {

    // update the count of successful registrations if we have not
    // already been told about the specified SM
    if (remoteConsumerIds_[smIndex] == ConsumerPipe::NULL_CONSUMER_ID) {
      ++successfulRegistrationCount_;
    }

    // assign the remote consumer ID for the specified storage manager
    remoteConsumerIds_[smIndex] = remoteConsumerId;
  }

  // check if we have successfully registered with all SMs
  bool allSuccessful = true;
  for (uint32 idx = 0; idx < remoteConsumerIds_.size(); ++idx) {
    if (remoteConsumerIds_[idx] == ConsumerPipe::NULL_CONSUMER_ID) {
      allSuccessful = false;
      break;
    }
  }
  if (allSuccessful) {
    needsRegistration_ = false;
  }
}

/**
 * Returns the remote consumer ID for the specified storage manager.
 */
uint32 ConsumerProxyInfo::getRemoteConsumerId(std::string smURL)
{
  boost::mutex::scoped_lock sl(dataMutex_);

  // find the index of the specified SM
  int smIndex = -1;
  for (int idx = 0; idx < static_cast<int>(smList_.size()); ++idx) {
    //std::cout << "getRemoteConsumerId " << smList_[idx]
    //          << " " << remoteConsumerIds_[idx] << std::endl;
    if (smList_[idx] == smURL) {
      smIndex = idx;
      break;
    }
  }
  //std::cout << "getRemoteConsumerId for URL " << smURL
  //          << ", index = " << smIndex << std::endl;

  if (smIndex >= 0 && smIndex < static_cast<int>(remoteConsumerIds_.size())) {
    return remoteConsumerIds_[smIndex];
  }
  else {
    return ConsumerPipe::NULL_CONSUMER_ID;
  }
}

/**
 * Returns the list of storage managers corresponding to the ones
 * from which a header needs to be fetched for the consumer.
 * For each storage manager, the SM URL is returned along with the
 * remote consumer ID (the ID that the SM assigned to the local consumer).
 * If a header has been successfully retrieved for all SMs, an
 * empty list is returned.
 *
 * @see ConsumerProxyInfo::needsHeader()
 */
std::vector< std::pair<std::string, uint32> >
ConsumerProxyInfo::getSMHeaderList()
{
  boost::mutex::scoped_lock sl(dataMutex_);

  // return an empty list if we're not ready for headers
  std::vector< std::pair<std::string, uint32> > headerList;
  if (! isReadyForHeaders()) {
    return headerList;
  }

  // build the list of SMs that we have registered with,
  // but haven't fetched a header from
  for (uint32 idx = 0; idx < smList_.size(); ++idx) {
    if (remoteConsumerIds_[idx] != ConsumerPipe::NULL_CONSUMER_ID &&
        ! headerSuccessFlags_[idx]) {
      std::pair<std::string, uint32> headerPair(smList_[idx],
                                                remoteConsumerIds_[idx]);
      headerList.push_back(headerPair);
    }
  }

  // assume that the caller is using this information to attempt
  // to fetch headers from storage managers, but we only start
  // counting once all registrations have succeeded or the
  // allowed number of registration attempts have been exhausted
  if (! needsRegistration()) {
    ++headerAttemptCount_;
  }

  // if we've exhausted the allowed number of attempts to fetch the headers,
  // we'll arbitrarily say that header fetching is done
  if (headerAttemptCount_ >= maxHeaderFetchAttempts_) {
    needsHeaders_ = false;
  }

  return headerList;
}

/**
 * Indicates that a header was successfully fetched from the specified
 * storage manager.
 */
void ConsumerProxyInfo::setHeaderFetchSuccess(std::string smURL)
{
  boost::mutex::scoped_lock sl(dataMutex_);

  // find the index of the specified SM
  int smIndex = -1;
  for (int idx = 0; idx < static_cast<int>(smList_.size()); ++idx) {
    if (smList_[idx] == smURL) {
      smIndex = idx;
      break;
    }
  }

  // update the status of the SM, if we found it
  if (smIndex >= 0 && smIndex < static_cast<int>(smList_.size())) {

    // update the count of successful header fetches if we have not
    // already been told about the specified SM
    if (! headerSuccessFlags_[smIndex]) {
      ++successfulHeaderCount_;

      // if this is the first header success, set the "nextSM" index
      // to this SM
      if (successfulHeaderCount_ == 1) {
        nextSMEventIndex_ = smIndex;
      }
    }

    // mark the header fetch successful for the SM
    headerSuccessFlags_[smIndex] = true;
  }

  // check if we have successfully fetched a header from all SMs
  bool allSuccessful = true;
  for (uint32 idx = 0; idx < headerSuccessFlags_.size(); ++idx) {
    if (! headerSuccessFlags_[idx]) {
      allSuccessful = false;
      break;
    }
  }
  if (allSuccessful) {
    needsHeaders_ = false;
  }
}

/**
 * Tests whether a header was successfully fetched from the specified
 * storage manager.
 */
bool ConsumerProxyInfo::hasHeaderFromSM(std::string smURL)
{
  boost::mutex::scoped_lock sl(dataMutex_);

  // look for the specified SM
  bool haveHeader = false;
  for (int idx = 0; idx < static_cast<int>(smList_.size()); ++idx) {
    if (smList_[idx] == smURL) {
      haveHeader = headerSuccessFlags_[idx];
      break;
    }
  }

  return haveHeader;
}

/**
 * Returns the amount of time (in seconds) to wait before attempting
 * to fetch the next event from any remote storage manager.  If there
 * is no need to wait, a value of zero is returned.
 */
double ConsumerProxyInfo::getTimeToWaitForEvent(double currentTime)
{
  boost::mutex::scoped_lock sl(dataMutex_);

  double timeToWait;
  //rateRequestCounter_->dumpData(std::cout);
  if (! rateRequestCounter_->hasValidResult()) {
    double timeDiff = currentTime - lastEventSuccessTime_;
    //std::cout << "### getT2W " << minTimeBetweenEvents_
    //          << " " << timeDiff
    //          << std::endl;
    timeToWait = 0.99 * (minTimeBetweenEvents_ - timeDiff);
  }
  else {
    timeToWait = ((minTimeBetweenEvents_ *
                   rateRequestCounter_->getSampleCount()) -
                  rateRequestCounter_->getDuration(currentTime));
    //if (timeToWait <= 0.0) {
    //  std::cout << "### getT2W " << minTimeBetweenEvents_
    //            << " " << rateRequestCounter_->getSampleCount()
    //            << " " << rateRequestCounter_->getDuration(currentTime)
    //            << std::endl;
    //}
  }
  if (timeToWait < 0.0) {
    timeToWait = 0.0;
  }
  return timeToWait;
}

/**
 * Returns the storage manager that should be contacted next for an
 * event.  In fact, what is returned is a data pair consisting of
 * the SM URL and the remote consumer ID that should be used when
 * communicating with the remote storage manager.  If there is no
 * SM that is ready to be contacted, the data pair will consist
 * of an empty string and the ConsumerPipe::NULL_CONSUMER_ID.
 */
std::pair<std::string, uint32> ConsumerProxyInfo::getNextSMForEvent()
{
  boost::mutex::scoped_lock sl(dataMutex_);

  // return an empty data pair if we're not ready for events
  std::pair<std::string, uint32> nextSM("", ConsumerPipe::NULL_CONSUMER_ID);
  if (! isReadyForEvents()) {
    return nextSM;
  }

  // if we've run through all of the available storage managers without
  // a successful event fetch, take a break
  if (runningEventAttemptCount_ >= successfulHeaderCount_) {
    double now = BaseCounter::getCurrentTime();
    lastEventSuccessTime_ = now;
    rateRequestCounter_->addSample(1.0, now);
    runningEventAttemptCount_ = 0;
    return nextSM;
  }

  // assign the next SM and remote consumer ID to the data pair
  nextSM.first = smList_[nextSMEventIndex_];
  nextSM.second = remoteConsumerIds_[nextSMEventIndex_];

  // update the nextSM index to point to the next available SM
  int lastSMIndex = nextSMEventIndex_;
  ++nextSMEventIndex_;
  if (nextSMEventIndex_ >= static_cast<int>(headerSuccessFlags_.size())) {
    nextSMEventIndex_ = 0;
  }
  while (! headerSuccessFlags_[nextSMEventIndex_] &&
         nextSMEventIndex_ != lastSMIndex) {
    ++nextSMEventIndex_;
    if (nextSMEventIndex_ >= static_cast<int>(headerSuccessFlags_.size())) {
      nextSMEventIndex_ = 0;
    }
  }

  // we assume that the caller is going to use this information to
  // make an event request
  ++runningEventAttemptCount_;

  return nextSM;
}

/**
 * Indicates that an event was successfully fetched from the specified
 * storage manager.
 */
void ConsumerProxyInfo::setEventFetchSuccess(std::string smURL,
                                             double currentTime)
{
  boost::mutex::scoped_lock sl(dataMutex_);

  lastEventSuccessTime_ = currentTime;
  rateRequestCounter_->addSample(1.0, currentTime);
  runningEventAttemptCount_ = 0;
}
