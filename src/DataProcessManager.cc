// $Id: DataProcessManager.cc,v 1.10 2008/04/16 16:43:13 biery Exp $

#include "EventFilter/SMProxyServer/interface/DataProcessManager.h"
#include "EventFilter/StorageManager/interface/SMCurlInterface.h"
#include "EventFilter/StorageManager/interface/ConsumerPipe.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "FWCore/Utilities/interface/DebugMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "IOPool/Streamer/interface/BufferArea.h"
#include "IOPool/Streamer/interface/OtherMessage.h"
#include "IOPool/Streamer/interface/ConsRegMessage.h"

#include "boost/bind.hpp"
#include "boost/lexical_cast.hpp"

#include "curl/curl.h"
#include <wait.h>

using namespace std;
using namespace edm;

using boost::thread;
using boost::bind;

namespace 
{
  const int voidptr_size = sizeof(void*);
}

namespace stor
{

  DataProcessManager::DataProcessManager():
    cmd_q_(edm::getEventBuffer(voidptr_size,50)),
    alreadyRegistered_(false),
    alreadyRegisteredDQM_(false),
    headerRefetchRequested_(false),
    buf_(2000),
    headerRetryInterval_(5),
    dqmServiceManager_(new stor::DQMServiceManager()),
    receivedEvents_(0),
    receivedDQMEvents_(0),
    samples_(100)
  {
    // for performance measurements
    pmeter_ = new stor::SMPerformanceMeter();
    init();
  } 

  DataProcessManager::~DataProcessManager()
  {
    delete pmeter_;
  }

  void DataProcessManager::init()
  {
    regpage_ =  "/registerConsumer";
    DQMregpage_ = "/registerDQMConsumer";
    eventpage_ = "/geteventdata";
    DQMeventpage_ = "/getDQMeventdata";
    headerpage_ = "/getregdata";
    consumerName_ = ConsumerPipe::PROXY_SERVER_NAME;
    //consumerPriority_ = "PushMode"; // this means push mode!
    consumerPriority_ = "Normal";
    DQMconsumerName_ = ConsumerPipe::PROXY_SERVER_NAME;
    //DQMconsumerPriority_ =  "PushMode"; // this means push mode!
    DQMconsumerPriority_ =  "Normal";

    double maxEventRequestRate = 10.0; // just a default until set in config action
    // over-ride the max rate (greatly reducing it) since we now
    // only use a direct request from the proxy server as a keep-alive
    // (no events flow through that link)
    maxEventRequestRate /= 100.0;

    const double MAX_REQUEST_INTERVAL = 60.0;  // seconds
    if (maxEventRequestRate < (1.0 / MAX_REQUEST_INTERVAL)) {
      minEventRequestInterval_ = MAX_REQUEST_INTERVAL;
    }
    else {
      minEventRequestInterval_ = 1.0 / maxEventRequestRate;  // seconds
    }
    consumerId_ = (time(0) & 0xffffff);  // temporary - will get from ES later

    //double maxDQMRequestRate = pset.getUntrackedParameter<double>("maxDQMEventRequestRate",1.0);
    double maxDQMRequestRate = 10.0; // TODO fixme: set this in the XML
    const double MAX_DQM_REQUEST_INTERVAL = 300.0;  // seconds
    if (maxDQMRequestRate < (1.0 / MAX_DQM_REQUEST_INTERVAL)) {
      minDQMEventRequestInterval_ = MAX_DQM_REQUEST_INTERVAL;
    }
    else {
      minDQMEventRequestInterval_ = 1.0 / maxDQMRequestRate;  // seconds
    }
    DQMconsumerId_ = (time(0) & 0xffffff);  // temporary - will get from ES later

    alreadyRegistered_ = false;
    alreadyRegisteredDQM_ = false;
    headerRefetchRequested_ = false;

    edm::ParameterSet ps = ParameterSet();
    // TODO fixme: only request event types that are requested by connected consumers?

    // 16-Apr-2008, KAB: set maxEventRequestRate in the parameterSet that
    // we send to the storage manager now that we have the fair share
    // algorithm working in the SM.
    Entry maxRateEntry("maxEventRequestRate",
                       static_cast<double>(1.0 / minEventRequestInterval_),
                       false);
    ps.insert(true, "maxEventRequestRate", maxRateEntry);

    consumerPSetString_ = ps.toString();
    // TODO fixme: only request folders that connected consumers want?
    consumerTopFolderName_ = "*";
    //consumerTopFolderName_ = "C1";
    receivedEvents_ = 0;
    receivedDQMEvents_ = 0;
    pmeter_->init(samples_);
    stats_.fullReset();

    // make an entry in the consumerMap for ourself (with a special consumerID)
    boost::shared_ptr<ConsumerProxyInfo>
      infoPtr(new ConsumerProxyInfo(ConsumerPipe::NULL_CONSUMER_ID,
                                    (1.0 / minEventRequestInterval_),
                                    smList_, 1024, 1024));
    consumerMap_[ConsumerPipe::NULL_CONSUMER_ID] = infoPtr;

    // initialize the counters that we use for statistics
    ltEventFetchTimeCounter_.reset(new ForeverCounter());
    stEventFetchTimeCounter_.reset(new RollingIntervalCounter(180,5,20));
    ltDQMFetchTimeCounter_.reset(new ForeverCounter());
    stDQMFetchTimeCounter_.reset(new RollingIntervalCounter(180,5,20));
  }

  void DataProcessManager::setMaxEventRequestRate(double rate)
  {
    if(rate <= 0.0) return; // TODO make sure config is checked!

    // over-ride the input rate (greatly reducing it) since we now
    // only use a direct request from the proxy server as a keep-alive
    // (no events flow through that link)
    rate /= 100.0;

    const double MAX_REQUEST_INTERVAL = 60.0;  // seconds
    if (rate < (1.0 / MAX_REQUEST_INTERVAL)) {
      minEventRequestInterval_ = MAX_REQUEST_INTERVAL;
    }
    else {
      minEventRequestInterval_ = 1.0 / rate;  // seconds
    }

    // update the rate stored in the proxy server's ConsumerProxyInfo
    boost::shared_ptr<ConsumerProxyInfo> infoPtr =
      consumerMap_[ConsumerPipe::NULL_CONSUMER_ID];
    infoPtr->updateRateRequest(1.0 / minEventRequestInterval_);

    // 16-Apr-2008, KAB: set maxEventRequestRate in the parameterSet that
    // we send to the storage manager now that we have the fair share
    // algorithm working in the SM.
    edm::ParameterSet ps = ParameterSet();
    Entry maxRateEntry("maxEventRequestRate",
                       (1.0 / minEventRequestInterval_),
                       false);
    ps.insert(true, "maxEventRequestRate", maxRateEntry);
    consumerPSetString_ = ps.toString();
  }

  void DataProcessManager::setMaxDQMEventRequestRate(double rate)
  {
    const double MAX_REQUEST_INTERVAL = 300.0;  // seconds
    if(rate <= 0.0) return; // TODO make sure config is checked!
    if (rate < (1.0 / MAX_REQUEST_INTERVAL)) {
      minDQMEventRequestInterval_ = MAX_REQUEST_INTERVAL;
    }
    else {
      minDQMEventRequestInterval_ = 1.0 / rate;  // seconds
    }
  }

  void DataProcessManager::run(DataProcessManager* t)
  {
    t->processCommands();
  }

  void DataProcessManager::start()
  {
    // called from a different thread to start things going

    me_.reset(new boost::thread(boost::bind(DataProcessManager::run,this)));
  }

  void DataProcessManager::stop()
  {
    // called from a different thread - trigger completion to the
    // data process manager loop

    edm::EventBuffer::ProducerBuffer cb(*cmd_q_);
    MsgCode mc(cb.buffer(),MsgCode::DONE);
    mc.setCode(MsgCode::DONE);
    cb.commit(mc.codeSize());
  }

  void DataProcessManager::join()
  {
    // invoked from a different thread - block until "me_" is done
    if(me_) me_->join();
  }

  void DataProcessManager::debugOutput(std::string text)
  {
    char nowString[32];
    sprintf(nowString, "%16.4f", BaseCounter::getCurrentTime());
    std::cout << "*** " << nowString << " " << text << std::endl;
  }

  void DataProcessManager::processCommands()
  {
    // called with this data process manager's own thread.
    // first register with the SM for each subfarm
    bool doneWithRegistration = false;
    // TODO fixme: improve method of hardcored fixed retries
    unsigned int count = 0; // keep of count of tries and quit after 255
    unsigned int maxcount = 255;
    bool doneWithDQMRegistration = false;
    unsigned int countDQM = 0; // keep of count of tries and quit after 255
    bool alreadysaid = false;
    bool alreadysaidDQM = false;

    bool gotOneHeader = false;
    unsigned int countINIT = 0; // keep of count of tries and quit after 255
    bool alreadysaidINIT = false;

    std::string workString;
    bool DoneWithJob = false;
    while(!DoneWithJob)
    {
      // work loop
      // if a header re-fetch has been requested, reset the header vars
      if (headerRefetchRequested_) {
        headerRefetchRequested_ = false;
        gotOneHeader = false;
        countINIT = 0;
      }
      // register as event consumer to all SM senders
      if(!alreadyRegistered_) {
        debugOutput("Not already registered");
        if(!doneWithRegistration)
        {
          debugOutput("Before reg wait");
          waitBetweenRegTrys();
          debugOutput("After reg wait");
          bool success = registerWithAllSM(ConsumerPipe::NULL_CONSUMER_ID);
          if(success) doneWithRegistration = true;
          ++count;
          workString = "After registerWithAllSM, success = ";
          try {
            workString.append(boost::lexical_cast<std::string>(success));
            workString.append(", count = ");
            workString.append(boost::lexical_cast<std::string>(count));
          }
          catch (boost::bad_lexical_cast& blcExcpt) {
            workString.append("???");
          }
          debugOutput(workString);
        }
        // TODO fixme: decide what to do after max tries
        if(count >= maxcount) edm::LogInfo("processCommands") << "Could not register with all SM Servers"
           << " after " << maxcount << " tries";
        if(doneWithRegistration && !alreadysaid) {
          edm::LogInfo("processCommands") << "Registered with all SM Event Servers";
          alreadysaid = true;
        }
        if(doneWithRegistration) alreadyRegistered_ = true;
      }
      // now register as DQM consumers
      if(!alreadyRegisteredDQM_) {
        if(!doneWithDQMRegistration)
        {
          waitBetweenRegTrys();
          bool success = registerWithAllDQMSM();
          if(success) doneWithDQMRegistration = true;
          ++countDQM;
        }
        // TODO fixme: decide what to do after max tries
        if(count >= maxcount) edm::LogInfo("processCommands") << "Could not register with all SM DQMEvent Servers"
          << " after " << maxcount << " tries";
        if(doneWithDQMRegistration && !alreadysaidDQM) {
          edm::LogInfo("processCommands") << "Registered with all SM DQMEvent Servers";
          alreadysaidDQM = true;
        }
        if(doneWithDQMRegistration) alreadyRegisteredDQM_ = true;
      }
      // now get one INIT header (product registry) and save it
      // as long as at least one SMsender registered with
      // TODO fixme: use the data member for got header to go across runs
      if(!gotOneHeader)
      {
        debugOutput("Before header wait");
        waitBetweenRegTrys();
        debugOutput("After header wait");
        bool success = getAnyHeaderFromSM(ConsumerPipe::NULL_CONSUMER_ID);
        if(success) gotOneHeader = true;
        ++countINIT;
        workString = "After getAnyHeaderFromSM, success = ";
        try {
          workString.append(boost::lexical_cast<std::string>(success));
          workString.append(", count = ");
          workString.append(boost::lexical_cast<std::string>(countINIT));
        }
        catch (boost::bad_lexical_cast& blcExcpt) {
          workString.append("???");
        }
        debugOutput(workString);
      }
      if(countINIT >= maxcount) edm::LogInfo("processCommands") << "Could not get product registry!"
          << " after " << maxcount << " tries";
      if(gotOneHeader && !alreadysaidINIT) {
        edm::LogInfo("processCommands") << "Got the product registry";
        alreadysaidINIT = true;
      }
      if(alreadyRegistered_ && gotOneHeader && haveHeader()) {

        // loop over all consumers to check if they need registration,
        // headers, or events (including the special NULL_CONSUMER_ID
        // consumer that we use to send keep-alive requests to the SM)
        std::vector<uint32> disconnectedList;
        double time2wait = 0.0;
        double sleepTime = static_cast<double>(headerRetryInterval_);
        ConsumerInfo_map::const_iterator infoIter;
        for (infoIter = consumerMap_.begin();
             infoIter != consumerMap_.end();
             ++infoIter) {
          unsigned int localConsumerId = infoIter->first;
          boost::shared_ptr<ConsumerProxyInfo> infoPtr = infoIter->second;
          //workString = "Consumer ";
          //try {
          //  workString.append(boost::lexical_cast<std::string>(localConsumerId));
          //}
          //catch (boost::bad_lexical_cast& blcExcpt) {
          //  workString.append("??? (1)");
          //}
          //debugOutput(workString);

          // first, check if the consumer is still active.  If idle,
          // move on to next consumer; if disconnected, add it to a list
          // for later removal and go to next consumer.
          // (Only check downstream consumers, not ourself!)
          if (localConsumerId != ConsumerPipe::NULL_CONSUMER_ID) {
            boost::shared_ptr<ConsumerPipe> consPtr =
              eventServer_->getConsumer(localConsumerId);
            if (consPtr.get() == NULL) {
              workString = "Consumer ";
              try {
                workString.append(boost::lexical_cast<std::string>(localConsumerId));
                workString.append(" is disconnected");
              }
              catch (boost::bad_lexical_cast& blcExcpt) {
                workString.append("??? (2)");
              }
              debugOutput(workString);
              disconnectedList.push_back(localConsumerId);
              continue;
            }
            else if (! consPtr->isActive()) {
              workString = "Consumer ";
              try {
                workString.append(boost::lexical_cast<std::string>(localConsumerId));
                workString.append(" is inactive");
              }
              catch (boost::bad_lexical_cast& blcExcpt) {
                workString.append("??? (3)");
              }
              debugOutput(workString);
              continue;
            }
          }

          // check if the consumer needs to register with one or more SMs;
          // if so, do that first
          if (infoPtr->needsRegistration()) {
            bool fullSuccess = registerWithAllSM(localConsumerId);
            workString = "Consumer ";
            try {
              workString.append(boost::lexical_cast<std::string>(localConsumerId));
              workString.append(" registration status = ");
              workString.append(boost::lexical_cast<std::string>(fullSuccess));
            }
            catch (boost::bad_lexical_cast& blcExcpt) {
              workString.append("??? (4)");
            }
            debugOutput(workString);
            // if we didn't succeed in registering with all SMs, assume
            // that we may need to sleep a bit between loops
            if (! fullSuccess) {
              time2wait = static_cast<double>(headerRetryInterval_); 
              if (time2wait < sleepTime && time2wait >= 0.0) {
                sleepTime = time2wait;
              }
            }
          }

          // check if the consumer needs headers and if it is ready to
          // to fetch them.  (Note that headers might be fetched
          // from a subset of SMs even if we haven't registered with all
          // SMs.  Those with registrations are queried for the header.)
          if (infoPtr->needsHeaders() && infoPtr->isReadyForHeaders()) {
            bool fullSuccess = getAnyHeaderFromSM(localConsumerId);
            workString = "Consumer ";
            try {
              workString.append(boost::lexical_cast<std::string>(localConsumerId));
              workString.append(" header status = ");
              workString.append(boost::lexical_cast<std::string>(fullSuccess));
            }
            catch (boost::bad_lexical_cast& blcExcpt) {
              workString.append("??? (5)");
            }
            debugOutput(workString);
            // if we didn't succeed in fetching headers from all SMs, assume
            // that we may need to sleep a bit between loops
            if (! fullSuccess) {
              time2wait = static_cast<double>(headerRetryInterval_); 
              if (time2wait < sleepTime && time2wait >= 0.0) {
                sleepTime = time2wait;
              }
            }
          }

          // check if the consumer is ready for events (in general)
          if (infoPtr->isReadyForEvents()) {

            // check if the consumer is ready for the next event
            time2wait = infoPtr->getTimeToWaitForEvent();
            //workString = "Consumer ";
            //try {
            //  workString.append(boost::lexical_cast<std::string>(localConsumerId));
            //  workString.append(" initial time to wait = ");
            //  workString.append(boost::lexical_cast<std::string>(time2wait));
            //}
            //catch (boost::bad_lexical_cast& blcExcpt) {
            //  workString.append("??? (6)");
            //}
            //debugOutput(workString);
            if (time2wait <= 0.0) {

              // fetch an event from the next available SM
              // If that succeeds, we stop, for now.
              // If that fails, we try a different SM until we run out of SMs
              bool keepTrying = true;
              while (keepTrying) {

                // fetch the next SM (and remote consumer ID) from the
                // ConsumerProxyInfo object
                std::pair<std::string, unsigned int> smPair =
                  infoPtr->getNextSMForEvent();
                std::string smURL = smPair.first;
                uint32 remoteConsumerId = smPair.second;

                // if we've exhausted the list of available SMs, stop trying
                if (smPair.second == ConsumerPipe::NULL_CONSUMER_ID) {
                  keepTrying = false;
                  continue;
                }

                // attempt to fetch an event; stop trying if successful
                //debugOutput("Before getOneEventFromSM");
                bool success = getOneEventFromSM(smURL, remoteConsumerId);
                //workString = "Consumer ";
                //try {
                //  workString.append(boost::lexical_cast<std::string>(localConsumerId));
                //  workString.append(" event status = ");
                //  workString.append(boost::lexical_cast<std::string>(success));
                //  workString.append(" from ");
                //  workString.append(smURL);
                //}
                //catch (boost::bad_lexical_cast& blcExcpt) {
                //  workString.append("??? (8)");
                //}
                //debugOutput(workString);
                if (success) {
                  infoPtr->setEventFetchSuccess(smPair.first);
                  keepTrying = false;
                }
              }
            }

            // re-fetch the time to wait (since it may have changed depending
            // on whether the event fetch worked) and update our sleep time
            time2wait = infoPtr->getTimeToWaitForEvent();
            //workString = "Consumer ";
            //try {
            //  workString.append(boost::lexical_cast<std::string>(localConsumerId));
            //  workString.append(" final time to wait = ");
            //  workString.append(boost::lexical_cast<std::string>(time2wait));
            //}
            //catch (boost::bad_lexical_cast& blcExcpt) {
            //  workString.append("??? (9)");
            //}
            //debugOutput(workString);
            if (time2wait < sleepTime && time2wait >= 0.0) {
              sleepTime = time2wait;
            }
          }
        }

        // remove any disconnected consumers from the consumer map
        for (uint32 idx = 0; idx < disconnectedList.size(); ++idx) {
          consumerMap_.erase(disconnectedList[idx]);
        }

        //workString = "Final sleepTime = ";
        //try {
        //  workString.append(boost::lexical_cast<std::string>(sleepTime));
        //}
        //catch (boost::bad_lexical_cast& blcExcpt) {
        //  workString.append("??? (9)");
        //}
        //debugOutput(workString);
        // sleep for the desired amount of time
        if(sleepTime > 0.0) usleep(static_cast<int>(1000000 * sleepTime));
      }
      if(alreadyRegisteredDQM_) {
        //debugOutput("Before getDQMEventFromAllSM");
        getDQMEventFromAllSM();
        //debugOutput("After getDQMEventFromAllSM");
      }

      // check for any commands - empty() does not block
      if(!cmd_q_->empty())
      {
        // the next line blocks until there is an entry in cmd_q
        edm::EventBuffer::ConsumerBuffer cb(*cmd_q_);
        MsgCode mc(cb.buffer(),cb.size());

        if(mc.getCode()==MsgCode::DONE) DoneWithJob = true;
        // right now we will ignore all messages other than DONE
      }

    } // done with process loop   
    edm::LogInfo("processCommands") << "Received done - stopping";
    if(dqmServiceManager_.get() != NULL) dqmServiceManager_->stop();
  }

  void DataProcessManager::addSM2Register(std::string smURL)
  {
    // This smURL is the URN of the StorageManager without the page extension
    // Check if already in the list
    bool alreadyInList = false;
    if(smList_.size() > 0) {
       for(unsigned int i = 0; i < smList_.size(); ++i) {
         if(smURL.compare(smList_[i]) == 0) {
            alreadyInList = true;
            break;
         }
       }
    }
    if(alreadyInList) return;
    smList_.push_back(smURL);
    smRegMap_.insert(std::make_pair(smURL,0));

    std::cout << "### Adding SM URL: " << smURL << std::endl;
    // add the SM to the ProxyInfo objects in the consumer map, and protect
    // against other operations on the map elsewhere in this class.
    {
      boost::mutex::scoped_lock sl(consumerMapMutex_);

      ConsumerInfo_map::const_iterator infoIter;
      for (infoIter = consumerMap_.begin();
           infoIter != consumerMap_.end();
           ++infoIter) {
        boost::shared_ptr<ConsumerProxyInfo> infoPtr = infoIter->second;
        infoPtr->addSM(smURL);
      }
    }
  }

  void DataProcessManager::addDQMSM2Register(std::string DQMsmURL)
  {
    // Check if already in the list
    bool alreadyInList = false;
    if(DQMsmList_.size() > 0) {
       for(unsigned int i = 0; i < DQMsmList_.size(); ++i) {
         if(DQMsmURL.compare(DQMsmList_[i]) == 0) {
            alreadyInList = true;
            break;
         }
       }
    }
    if(alreadyInList) return;
    DQMsmList_.push_back(DQMsmURL);
    DQMsmRegMap_.insert(std::make_pair(DQMsmURL,0));
    struct timeval lastRequestTime;
    lastRequestTime.tv_sec = 0;
    lastRequestTime.tv_usec = 0;
    lastDQMReqMap_.insert(std::make_pair(DQMsmURL,lastRequestTime));
  }

  void DataProcessManager::addLocalConsumer(boost::shared_ptr<ConsumerPipe> consPtr)
  {
    // make an entry in the consumerMap for the consumer, and protect
    // against other operations on the map elsewhere in this class.
    boost::mutex::scoped_lock sl(consumerMapMutex_);

    boost::shared_ptr<ConsumerProxyInfo>
      infoPtr(new ConsumerProxyInfo(consPtr->getConsumerId(),
                                    consPtr->getRateRequest(),
                                    smList_, 255, 255));
    consumerMap_[consPtr->getConsumerId()] = infoPtr;
  }

  bool DataProcessManager::registerWithAllSM(unsigned int localConsumerId)
  {
    boost::shared_ptr<ConsumerProxyInfo> infoPtr =
      consumerMap_[localConsumerId];
    // keep the old-style registration for the proxy server itself (for now)
    if (localConsumerId == ConsumerPipe::NULL_CONSUMER_ID) {
      // One try at registering with the SM on each subfarm
      // return true if registered with all SM 
      // Only make one attempt and return so we can make this thread stop
      if(smList_.size() == 0) return false;
      bool allRegistered = true;
      for(unsigned int i = 0; i < smList_.size(); ++i) {
        if(smRegMap_[smList_[i] ] > 0) continue; // already registered
        int consumerid = registerWithSM(smList_[i], localConsumerId);
        if(consumerid > 0) smRegMap_[smList_[i] ] = consumerid;
        else allRegistered = false;

        if (consumerid != static_cast<int>(ConsumerPipe::NULL_CONSUMER_ID)) {
          infoPtr->setRegistrationSuccess(smList_[i], consumerid);
        }
      }
      return allRegistered;
    }
    else {
      if (! infoPtr->needsRegistration()) {return false;}
      std::vector<std::string> regList = infoPtr->getSMRegistrationList();
      for (unsigned int idx = 0; idx < regList.size(); ++idx) {
        unsigned int remoteConsumerId =
          registerWithSM(regList[idx], localConsumerId);
        //std::cout << "### remoteConsumerId(" << idx << ") = "
        //          << remoteConsumerId << std::endl;
        if (remoteConsumerId != ConsumerPipe::NULL_CONSUMER_ID) {
          infoPtr->setRegistrationSuccess(regList[idx], remoteConsumerId);
        }
      }
      //std::cout << "### isReadyForHeaders = " << infoPtr->isReadyForHeaders()
      //          << std::endl;
      //std::cout << "### needsRegistration = " << infoPtr->needsRegistration()
      //          << std::endl;
      return (infoPtr->isReadyForHeaders() && ! infoPtr->needsRegistration());
    }
  }

  bool DataProcessManager::registerWithAllDQMSM()
  {
    // One try at registering with the SM on each subfarm
    // return true if registered with all SM 
    // Only make one attempt and return so we can make this thread stop
    if(DQMsmList_.size() == 0) return false;
    bool allRegistered = true;
    for(unsigned int i = 0; i < DQMsmList_.size(); ++i) {
      if(DQMsmRegMap_[DQMsmList_[i] ] > 0) continue; // already registered
      int consumerid = registerWithDQMSM(DQMsmList_[i]);
      if(consumerid > 0) DQMsmRegMap_[DQMsmList_[i] ] = consumerid;
      else allRegistered = false;
    }
    return allRegistered;
  }

  int DataProcessManager::registerWithSM(std::string smURL,
                                         unsigned int localConsumerId)
  {
    // Use this same registration method for both event data and DQM data
    // return with consumerID or 0 for failure
    stor::ReadData data;
    std::cout << "### Attempting to register with " << smURL
              << " for localConsumerId " << localConsumerId << std::endl;

    data.d_.clear();
    CURL* han = curl_easy_init();
    if(han==0)
    {
      edm::LogError("registerWithSM") << "Could not create curl handle";
      // this is a fatal error isn't it? Are we catching this? TODO
      throw cms::Exception("registerWithSM","DataProcessManager")
          << "Unable to create curl handle\n";
    }
    // set the standard http request options
    std::string url2use = smURL + regpage_;
    setopt(han,CURLOPT_URL,url2use.c_str());
    setopt(han,CURLOPT_WRITEFUNCTION,func);
    setopt(han,CURLOPT_WRITEDATA,&data);

    // build the registration request message to send to the storage manager
    const int BUFFER_SIZE = 2000;
    char msgBuff[BUFFER_SIZE];
    boost::shared_ptr<ConsRegRequestBuilder> requestMessage;
    if (localConsumerId == ConsumerPipe::NULL_CONSUMER_ID)
    {
      requestMessage.reset(new ConsRegRequestBuilder(msgBuff, BUFFER_SIZE,
                                                     consumerName_,
                                                     consumerPriority_,
                                                     consumerPSetString_));
    }
    else
    {
      boost::shared_ptr<ConsumerProxyInfo> proxyInfoPtr =
        consumerMap_[ConsumerPipe::NULL_CONSUMER_ID];
      unsigned int proxyConsumerId = proxyInfoPtr->getRemoteConsumerId(smURL);

      std::map< uint32, boost::shared_ptr<ConsumerPipe> > consumerTable = 
        eventServer_->getConsumerTable();
      boost::shared_ptr<ConsumerPipe> consPtr = consumerTable[localConsumerId];

      edm::ParameterSet ps = ParameterSet();
      Entry maxRateEntry = Entry("maxEventRequestRate",
                                 consPtr->getRateRequest(),
                                 false);
      ps.insert(true, "maxEventRequestRate", maxRateEntry);

      edm::ParameterSet subPSet = ParameterSet();
      Entry selectionsEntry = Entry("SelectEvents",
                                    consPtr->getTriggerSelection(),
                                    true);
      subPSet.insert(true, "SelectEvents", selectionsEntry);
      Entry selectionsPSEntry = Entry("SelectEvents", subPSet, false);
      ps.insert(true, "SelectEvents", selectionsPSEntry);

      std::string localPSetString = ps.toString();
      requestMessage.
        reset(new ConsRegRequestBuilder(msgBuff, BUFFER_SIZE,
                                        consPtr->getConsumerName(),
                                        consPtr->getPriority(),
                                        localPSetString,
                                        localConsumerId,
                                        proxyConsumerId));
    }

    // add the request message as a http post
    setopt(han, CURLOPT_POSTFIELDS, requestMessage->startAddress());
    setopt(han, CURLOPT_POSTFIELDSIZE, requestMessage->size());
    struct curl_slist *headers=NULL;
    headers = curl_slist_append(headers, "Content-Type: application/octet-stream");
    headers = curl_slist_append(headers, "Content-Transfer-Encoding: binary");
    setopt(han, CURLOPT_HTTPHEADER, headers);

    // send the HTTP POST, read the reply, and cleanup before going on
    CURLcode messageStatus = curl_easy_perform(han);
    curl_slist_free_all(headers);
    curl_easy_cleanup(han);
    //std::cout << "Message status for consumer " << localConsumerId
    //          << " is " << messageStatus << std::endl;

    if(messageStatus!=0)
    {
      cerr << "curl perform failed for registration" << endl;
      edm::LogError("registerWithSM") << "curl perform failed for registration. "
        << "Could not register: probably XDAQ not running on Storage Manager"
        << " at " << smURL;
      return 0;
    }
    uint32 registrationStatus = ConsRegResponseBuilder::ES_NOT_READY;
    int consumerId = 0;
    //std::cout << "Data length for consumer " << localConsumerId
    //          << " is " << data.d_.length() << std::endl;
    if(data.d_.length() > 0)
    {
      int len = data.d_.length();
      FDEBUG(9) << "registerWithSM received len = " << len << std::endl;
      buf_.resize(len);
      for (int i=0; i<len ; i++) buf_[i] = data.d_[i];

      try {
        ConsRegResponseView respView(&buf_[0]);
        registrationStatus = respView.getStatus();
        consumerId = respView.getConsumerId();
        //std::cout << "ID for consumer " << localConsumerId
        //          << " is " << consumerId << std::endl;
        if (eventServer_.get() != NULL) {
          if (localConsumerId == ConsumerPipe::NULL_CONSUMER_ID) {
            eventServer_->setStreamSelectionTable(respView.getStreamSelectionTable());
          }
        }
      }
      catch (cms::Exception excpt) {
        const unsigned int MAX_DUMP_LENGTH = 1000;
        edm::LogError("registerWithSM") << "========================================";
        edm::LogError("registerWithSM") << "Exception decoding the registerWithSM response!";
        if (data.d_.length() <= MAX_DUMP_LENGTH) {
          edm::LogError("registerWithSM") << "Here is the raw text that was returned:";
          edm::LogError("registerWithSM") << data.d_;
        }
        else {
          edm::LogError("registerWithSM") << "Here are the first " << MAX_DUMP_LENGTH <<
            " characters of the raw text that was returned:";
          edm::LogError("registerWithSM") << (data.d_.substr(0, MAX_DUMP_LENGTH));
        }
        edm::LogError("registerWithSM") << "========================================";
        return 0;
      }
    }
    if(registrationStatus == ConsRegResponseBuilder::ES_NOT_READY) return 0;
    FDEBUG(5) << "Consumer ID = " << consumerId << endl;
    return consumerId;
  }

  int DataProcessManager::registerWithDQMSM(std::string smURL)
  {
    // Use this same registration method for both event data and DQM data
    // return with consumerID or 0 for failure
    stor::ReadData data;

    data.d_.clear();
    CURL* han = curl_easy_init();
    if(han==0)
    {
      edm::LogError("registerWithDQMSM") << "Could not create curl handle";
      // this is a fatal error isn't it? Are we catching this? TODO
      throw cms::Exception("registerWithDQMSM","DataProcessManager")
          << "Unable to create curl handle\n";
    }
    // set the standard http request options
    std::string url2use = smURL + DQMregpage_;
    setopt(han,CURLOPT_URL,url2use.c_str());
    setopt(han,CURLOPT_WRITEFUNCTION,func);
    setopt(han,CURLOPT_WRITEDATA,&data);

    // build the registration request message to send to the storage manager
    const int BUFFER_SIZE = 2000;
    char msgBuff[BUFFER_SIZE];
    ConsRegRequestBuilder requestMessage(msgBuff, BUFFER_SIZE, DQMconsumerName_,
                                         DQMconsumerPriority_,
                                         consumerTopFolderName_);

    // add the request message as a http post
    setopt(han, CURLOPT_POSTFIELDS, requestMessage.startAddress());
    setopt(han, CURLOPT_POSTFIELDSIZE, requestMessage.size());
    struct curl_slist *headers=NULL;
    headers = curl_slist_append(headers, "Content-Type: application/octet-stream");
    headers = curl_slist_append(headers, "Content-Transfer-Encoding: binary");
    setopt(han, CURLOPT_HTTPHEADER, headers);

    // send the HTTP POST, read the reply, and cleanup before going on
    CURLcode messageStatus = curl_easy_perform(han);
    curl_slist_free_all(headers);
    curl_easy_cleanup(han);

    if(messageStatus!=0)
    {
      cerr << "curl perform failed for DQM registration" << endl;
      edm::LogError("registerWithDQMSM") << "curl perform failed for registration. "
        << "Could not register with DQM: probably XDAQ not running on Storage Manager"
        << " at " << smURL;
      return 0;
    }
    uint32 registrationStatus = ConsRegResponseBuilder::ES_NOT_READY;
    int consumerId = 0;
    if(data.d_.length() > 0)
    {
      int len = data.d_.length();
      FDEBUG(9) << "registerWithDQMSM received len = " << len << std::endl;
      buf_.resize(len);
      for (int i=0; i<len ; i++) buf_[i] = data.d_[i];

      try {
        ConsRegResponseView respView(&buf_[0]);
        registrationStatus = respView.getStatus();
        consumerId = respView.getConsumerId();
      }
      catch (cms::Exception excpt) {
        const unsigned int MAX_DUMP_LENGTH = 1000;
        edm::LogError("registerWithDQMSM") << "========================================";
        edm::LogError("registerWithDQMSM") << "Exception decoding the registerWithSM response!";
        if (data.d_.length() <= MAX_DUMP_LENGTH) {
          edm::LogError("registerWithDQMSM") << "Here is the raw text that was returned:";
          edm::LogError("registerWithDQMSM") << data.d_;
        }
        else {
          edm::LogError("registerWithDQMSM") << "Here are the first " << MAX_DUMP_LENGTH <<
            " characters of the raw text that was returned:";
          edm::LogError("registerWithDQMSM") << (data.d_.substr(0, MAX_DUMP_LENGTH));
        }
        edm::LogError("registerWithDQMSM") << "========================================";
        return 0;
      }
    }
    if(registrationStatus == ConsRegResponseBuilder::ES_NOT_READY) return 0;
    FDEBUG(5) << "Consumer ID = " << consumerId << endl;
    return consumerId;
  }

  bool DataProcessManager::getAnyHeaderFromSM(unsigned int localConsumerId)
  {
    boost::shared_ptr<ConsumerProxyInfo> infoPtr =
      consumerMap_[localConsumerId];
    // keep the old-style registration for the proxy server itself (for now)
    if (localConsumerId == ConsumerPipe::NULL_CONSUMER_ID) {
      // Try the list of SM in order of registration to get one Header
      bool gotOneHeader = false;
      if(smList_.size() > 0) {
        for(unsigned int i = 0; i < smList_.size(); ++i) {
          if(smRegMap_[smList_[i] ] > 0 &&
             ! infoPtr->hasHeaderFromSM(smList_[i])) {
            std::cout << "Attempt1 for " << smList_[i] << std::endl;
            bool success = getHeaderFromSM(smList_[i], localConsumerId,
                                           smRegMap_[smList_[i]]);
            // 18-Apr-2008, KAB - should get header from every SM because the
            // SM executes necessary operations in response to a header request
            if(success) { // should cleam this up!
              gotOneHeader = true;

              std::cout << "Success1 for " << smList_[i] << std::endl;
              infoPtr->setHeaderFetchSuccess(smList_[i]);

              return gotOneHeader;
            }
          }
        }
      } else {
        // this is a problem (but maybe not with non-blocking processing loop)
        return false;
      }
      return gotOneHeader;
    }
    else {
      if (! infoPtr->needsHeaders()) {return false;}
      std::vector< std::pair<std::string, unsigned int> > hdrList =
        infoPtr->getSMHeaderList();
      for (unsigned int idx = 0; idx < hdrList.size(); ++idx) {
        std::string smURL = hdrList[idx].first;
        unsigned int remoteConsumerId = hdrList[idx].second;
        //std::cout << "### remoteConsumerId(" << idx << ") = "
        //          << remoteConsumerId << std::endl;
        if (getHeaderFromSM(smURL, localConsumerId, remoteConsumerId)) {
          std::cout << "Success2 for " << smURL << std::endl;
          infoPtr->setHeaderFetchSuccess(smURL);
        }
      }
      //std::cout << "### isReadyForEvents = " << infoPtr->isReadyForEvents()
      //          << std::endl;
      //std::cout << "### needsHeaders = " << infoPtr->needsHeaders()
      //          << std::endl;
      return (infoPtr->isReadyForEvents() && ! infoPtr->needsHeaders());
    }
  }

  bool DataProcessManager::getHeaderFromSM(std::string smURL,
                                           unsigned int localConsumerId,
                                           unsigned int remoteConsumerId)
  {
    // One single try to get a header from this SM URL
    stor::ReadData data;
    std::cout << "### Attempting to get header from " << smURL
              << " for localConsumerId " << localConsumerId << std::endl;

    data.d_.clear();
    CURL* han = curl_easy_init();
    if(han==0)
    {
      edm::LogError("getHeaderFromSM") << "Could not create curl handle";
      // this is a fatal error isn't it? Are we catching this? TODO
      throw cms::Exception("getHeaderFromSM","DataProcessManager")
          << "Unable to create curl handle\n";
    }
    // set the standard http request options
    std::string url2use = smURL + headerpage_;
    setopt(han,CURLOPT_URL,url2use.c_str());
    setopt(han,CURLOPT_WRITEFUNCTION,func);
    setopt(han,CURLOPT_WRITEDATA,&data);

    // send our consumer ID as part of the header request
    char msgBuff[100];
    OtherMessageBuilder requestMessage(&msgBuff[0], Header::HEADER_REQUEST,
                                       sizeof(char_uint32));
    uint8 *bodyPtr = requestMessage.msgBody();
    convert(remoteConsumerId, bodyPtr);
    setopt(han, CURLOPT_POSTFIELDS, requestMessage.startAddress());
    setopt(han, CURLOPT_POSTFIELDSIZE, requestMessage.size());
    struct curl_slist *headers=NULL;
    headers = curl_slist_append(headers, "Content-Type: application/octet-stream");
    headers = curl_slist_append(headers, "Content-Transfer-Encoding: binary");
    setopt(han, CURLOPT_HTTPHEADER, headers);

    // send the HTTP POST, read the reply, and cleanup before going on
    CURLcode messageStatus = curl_easy_perform(han);
    curl_slist_free_all(headers);
    curl_easy_cleanup(han);

    if(messageStatus!=0)
    { 
      cerr << "curl perform failed for header" << endl;
      edm::LogError("getHeaderFromSM") << "curl perform failed for header. "
        << "Could not get header from an already registered Storage Manager"
        << " at " << smURL;
      return false;
    }
    if(data.d_.length() == 0)
    { 
      return false;
    }

    if (localConsumerId != ConsumerPipe::NULL_CONSUMER_ID)
    {
      return true;
    }

    // rely on http transfer string of correct length!
    int len = data.d_.length();
    FDEBUG(9) << "getHeaderFromSM received registry len = " << len << std::endl;

    // check that we've received a valid INIT message
    // or a set of INIT messages.  Save everything that we receive.
    bool addedNewInitMsg = false;
    try
    {
      HeaderView hdrView(&data.d_[0]);
      if (hdrView.code() == Header::INIT)
      {
        InitMsgView initView(&data.d_[0]);
        if (initMsgCollection_->testAndAddIfUnique(initView))
        {
          addedNewInitMsg = true;
        }
      }
      else if (hdrView.code() == Header::INIT_SET)
      {
        OtherMessageView otherView(&data.d_[0]);
        bodyPtr = otherView.msgBody();
        uint32 fullSize = otherView.bodySize();
        while ((unsigned int) (bodyPtr-otherView.msgBody()) < fullSize)
        {
          InitMsgView initView(bodyPtr);
          if (initMsgCollection_->testAndAddIfUnique(initView))
          {
            addedNewInitMsg = true;
          }
          bodyPtr += initView.size();
        }
      }
      else
      {
        throw cms::Exception("getHeaderFromSM", "DataProcessManager");
      }
    }
    catch (cms::Exception excpt)
    {
      const unsigned int MAX_DUMP_LENGTH = 1000;
      edm::LogError("getHeaderFromSM") << "========================================";
      edm::LogError("getHeaderFromSM") << "Exception decoding the getRegistryData response!";
      if (data.d_.length() <= MAX_DUMP_LENGTH) {
        edm::LogError("getHeaderFromSM") << "Here is the raw text that was returned:";
        edm::LogError("getHeaderFromSM") << data.d_;
      }
      else {
        edm::LogError("getHeaderFromSM") << "Here are the first " << MAX_DUMP_LENGTH <<
          " characters of the raw text that was returned:";
        edm::LogError("getHeaderFromSM") << (data.d_.substr(0, MAX_DUMP_LENGTH));
      }
      edm::LogError("getHeaderFromSM") << "========================================";
      throw excpt;
    }

    // check if any currently connected consumers have trigger
    // selection requests that match more than one INIT message
    if (addedNewInitMsg && eventServer_.get() != NULL)
    {
      std::map< uint32, boost::shared_ptr<ConsumerPipe> > consumerTable = 
        eventServer_->getConsumerTable();
      std::map< uint32, boost::shared_ptr<ConsumerPipe> >::const_iterator 
        consumerIter;
      for (consumerIter = consumerTable.begin();
           consumerIter != consumerTable.end();
           consumerIter++)
      {
        boost::shared_ptr<ConsumerPipe> consPtr = consumerIter->second;
        try
        {
          Strings consumerSelection = consPtr->getTriggerRequest();
          initMsgCollection_->getElementForSelection(consumerSelection);
        }
        catch (const edm::Exception& excpt)
        {
          // store a warning message in the consumer pipe to be
          // sent to the consumer at the next opportunity
          consPtr->setRegistryWarning(excpt.what());
        }
        catch (const cms::Exception& excpt)
        {
          // store a warning message in the consumer pipe to be
          // sent to the consumer at the next opportunity
          std::string errorString;
          errorString.append(excpt.what());
          errorString.append("\n");
          errorString.append(initMsgCollection_->getSelectionHelpString());
          errorString.append("\n\n");
          errorString.append("*** Please select trigger paths from one and ");
          errorString.append("only one HLT output module. ***\n");
          consPtr->setRegistryWarning(errorString);
        }
      }
    }

    return true;
  }

  void DataProcessManager::waitBetweenRegTrys()
  {
    // for now just a simple wait for a fixed time
    sleep(headerRetryInterval_);
    return;
  }

  bool DataProcessManager::haveRegWithEventServer()
  {
    // registered with any of the SM event servers
    if(smList_.size() > 0) {
      for(unsigned int i = 0; i < smList_.size(); ++i) {
        if(smRegMap_[smList_[i] ] > 0) return true;
      }
    }
    return false;
  }

  bool DataProcessManager::haveRegWithDQMServer()
  {
    // registered with any of the SM DQM servers
    if(DQMsmList_.size() > 0) {
      for(unsigned int i = 0; i < DQMsmList_.size(); ++i) {
        if(DQMsmRegMap_[DQMsmList_[i] ] > 0) return true;
      }
    }
    return false;
  }

  bool DataProcessManager::haveHeader()
  {
    if(initMsgCollection_->size() > 0) return true;
    return false;
  }

  bool DataProcessManager::getOneEventFromSM(std::string smURL,
                                             unsigned int remoteConsumerId)
  {
    //std::cout << "### getOneEvent " << smURL << " " << remoteConsumerId
    //          << std::endl;

    // One single try to get a event from this SM URL
    stor::ReadData data;

    // start a measurement of how long the HTTP POST takes
    eventFetchTimer_.stop();
    eventFetchTimer_.reset();
    eventFetchTimer_.start();

    data.d_.clear();
    CURL* han = curl_easy_init();
    if(han==0)
    {
      edm::LogError("getOneEventFromSM") << "Could not create curl handle";
      // this is a fatal error isn't it? Are we catching this? TODO
      throw cms::Exception("getOneEventFromSM","DataProcessManager")
          << "Unable to create curl handle\n";
    }
    // set the standard http request options
    std::string url2use = smURL + eventpage_;
    setopt(han,CURLOPT_URL,url2use.c_str());
    setopt(han,CURLOPT_WRITEFUNCTION,stor::func);
    setopt(han,CURLOPT_WRITEDATA,&data);

    // send our consumer ID as part of the event request
    // The event request body consists of the consumerId and the
    // number of INIT messages in our collection.  The latter is used
    // to determine if we need to re-fetch the INIT message collection.
    char msgBuff[100];
    OtherMessageBuilder requestMessage(&msgBuff[0], Header::EVENT_REQUEST,
                                       2 * sizeof(char_uint32));
    uint8 *bodyPtr = requestMessage.msgBody();

    convert(remoteConsumerId, bodyPtr);
    bodyPtr += sizeof(char_uint32);
    convert((uint32) initMsgCollection_->size(), bodyPtr);
    setopt(han, CURLOPT_POSTFIELDS, requestMessage.startAddress());
    setopt(han, CURLOPT_POSTFIELDSIZE, requestMessage.size());
    struct curl_slist *headers=NULL;
    headers = curl_slist_append(headers, "Content-Type: application/octet-stream");
    headers = curl_slist_append(headers, "Content-Transfer-Encoding: binary");
    stor::setopt(han, CURLOPT_HTTPHEADER, headers);

    //setopt(han, CURLOPT_FORBID_REUSE, 1);
    //setopt(han, CURLOPT_VERBOSE, 1);
    //setopt(han, CURLOPT_TCP_NODELAY, 1);

    //debugOutput("getOneEventFromSM   AAA");
    // send the HTTP POST, read the reply, and cleanup before going on
    CURLcode messageStatus = curl_easy_perform(han);
    curl_slist_free_all(headers);
    curl_easy_cleanup(han);
    //std::cout << "Message status for consumer " << remoteConsumerId
    //          << " is " << messageStatus << std::endl;
    //debugOutput("getOneEventFromSM   BBB");

    if(messageStatus!=0)
    { 
      cerr << "curl perform failed for event" << endl;
      edm::LogError("getOneEventFromSM") << "curl perform failed for event. "
        << "Could not get event from an already registered Storage Manager"
        << " at " << smURL;

      // keep statistics for all HTTP POSTS
      eventFetchTimer_.stop();
      ltEventFetchTimeCounter_->addSample(eventFetchTimer_.realTime());
      stEventFetchTimeCounter_->addSample(eventFetchTimer_.realTime());

      return false;
    }

    // rely on http transfer string of correct length!
    int len = data.d_.length();
    FDEBUG(9) << "getOneEventFromSM received len = " << len << std::endl;
    if(data.d_.length() == 0)
    { 
      // keep statistics for all HTTP POSTS
      eventFetchTimer_.stop();
      ltEventFetchTimeCounter_->addSample(eventFetchTimer_.realTime());
      stEventFetchTimeCounter_->addSample(eventFetchTimer_.realTime());

      return false;
    }

    buf_.resize(len);
    for (int i=0; i<len ; i++) buf_[i] = data.d_[i];

    // keep statistics for all HTTP POSTS
    eventFetchTimer_.stop();
    ltEventFetchTimeCounter_->addSample(eventFetchTimer_.realTime());
    stEventFetchTimeCounter_->addSample(eventFetchTimer_.realTime());

    // first check if done message
    OtherMessageView msgView(&buf_[0]);

    if (msgView.code() == Header::DONE) {
      // TODO fixme:just print message for now
      std::cout << " SM " << smURL << " has halted" << std::endl;
      return false;
    } else if (msgView.code() == Header::NEW_INIT_AVAILABLE) {
      std::cout << "Received NEW_INIT_AVAILABLE message" << std::endl;
      headerRefetchRequested_ = true;
      return false;
    } else {
      // 05-Feb-2008, KAB:  catch (and rethrow) any exceptions decoding
      // the event data so that we can display the returned HTML and
      // (hopefully) give the user a hint as to the cause of the problem.
      try {
        HeaderView hdrView(&buf_[0]);
        if (hdrView.code() != Header::EVENT) {
          throw cms::Exception("getOneEventFromSM", "DataProcessManager");
        }
        EventMsgView eventView(&buf_[0]);
        ++receivedEvents_;
        addMeasurement((unsigned long)data.d_.length());
        if(eventServer_.get() != NULL) {
          eventServer_->processEvent(eventView);
          return true;
        }
      }
      catch (cms::Exception excpt) {
        const unsigned int MAX_DUMP_LENGTH = 1000;
        edm::LogError("getOneEventFromSM") << "========================================";
        edm::LogError("getOneEventFromSM") << "Exception decoding the getEventData response!";
        if (data.d_.length() <= MAX_DUMP_LENGTH) {
          edm::LogError("getOneEventFromSM") << "Here is the raw text that was returned:";
          edm::LogError("getOneEventFromSM") << data.d_;
        }
        else {
          edm::LogError("getOneEventFromSM") << "Here are the first " << MAX_DUMP_LENGTH <<
            " characters of the raw text that was returned:";
          edm::LogError("getOneEventFromSM") << (data.d_.substr(0, MAX_DUMP_LENGTH));
        }
        edm::LogError("getOneEventFromSM") << "========================================";
        throw excpt;
      }
    }
    return false;
  }

  void DataProcessManager::getDQMEventFromAllSM()
  {
    // Try the list of SM in order of registration to get one event
    // so long as we have the header from SM already
    if(smList_.size() > 0) {
      double time2wait = 0.0;
      double sleepTime = 300.0;
      bool gotOneEvent = false;
      bool gotOne = false;
      for(unsigned int i = 0; i < smList_.size(); ++i) {
        if(DQMsmRegMap_[smList_[i] ] > 0) {   // is registered
          gotOne = getOneDQMEventFromSM(smList_[i], time2wait);
          if(gotOne) {
            gotOneEvent = true;
          } else {
            if(time2wait < sleepTime && time2wait >= 0.0) sleepTime = time2wait;
          }
        }
      }
      // check if we need to sleep (to enforce the allowed request rate)
      // we don't want to ping the StorageManager app too often
      // TODO fixme: Cannot sleep for DQM as this is a long time usually
      //             and we block the event request poll if we sleep!
      //             have to find out how to ensure the correct poll rate
      if(!gotOneEvent) {
        //if(sleepTime > 0.0) usleep(static_cast<int>(1000000 * sleepTime));
      }
    }
  }

  double DataProcessManager::getDQMTime2Wait(std::string smURL)
  {
    // calculate time since last ping of this SM in seconds
    struct timeval now;
    struct timezone dummyTZ;
    gettimeofday(&now, &dummyTZ);
    double timeDiff = (double) now.tv_sec;
    timeDiff -= (double) lastDQMReqMap_[smURL].tv_sec;
    timeDiff += ((double) now.tv_usec / 1000000.0);
    timeDiff -= ((double) lastDQMReqMap_[smURL].tv_usec / 1000000.0);
    if (timeDiff < minDQMEventRequestInterval_)
    {
      return (minDQMEventRequestInterval_ - timeDiff);
    }
    else
    {
      return 0.0;
    }
  }

  void DataProcessManager::setDQMTime2Now(std::string smURL)
  {
    struct timeval now;
    struct timezone dummyTZ;
    gettimeofday(&now, &dummyTZ);
    lastDQMReqMap_[smURL] = now;
  }

  bool DataProcessManager::getOneDQMEventFromSM(std::string smURL, double& time2wait)
  {
    // See if we will exceed the request rate, if so just return false
    // Return values: 
    //    true = we have an event; false = no event (whatever reason)
    // time2wait values:
    //    0.0 = we pinged this SM this time; >0 = did not ping, wait this time
    // if every SM returns false we sleep some time
    time2wait = getDQMTime2Wait(smURL);
    if(time2wait > 0.0) {
      return false;
    } else {
      setDQMTime2Now(smURL);
    }

    // One single try to get a event from this SM URL
    stor::ReadData data;

    // start a measurement of how long the HTTP POST takes
    dqmFetchTimer_.stop();
    dqmFetchTimer_.reset();
    dqmFetchTimer_.start();

    data.d_.clear();
    CURL* han = curl_easy_init();
    if(han==0)
    {
      edm::LogError("getOneDQMEventFromSM") << "Could not create curl handle";
      // this is a fatal error isn't it? Are we catching this? TODO
      throw cms::Exception("getOneDQMEventFromSM","DataProcessManager")
          << "Unable to create curl handle\n";
    }
    // set the standard http request options
    std::string url2use = smURL + DQMeventpage_;
    setopt(han,CURLOPT_URL,url2use.c_str());
    setopt(han,CURLOPT_WRITEFUNCTION,stor::func);
    setopt(han,CURLOPT_WRITEDATA,&data);

    // send our consumer ID as part of the event request
    char msgBuff[100];
    OtherMessageBuilder requestMessage(&msgBuff[0], Header::DQMEVENT_REQUEST,
                                       sizeof(char_uint32));
    uint8 *bodyPtr = requestMessage.msgBody();
    convert(DQMsmRegMap_[smURL], bodyPtr);
    setopt(han, CURLOPT_POSTFIELDS, requestMessage.startAddress());
    setopt(han, CURLOPT_POSTFIELDSIZE, requestMessage.size());
    struct curl_slist *headers=NULL;
    headers = curl_slist_append(headers, "Content-Type: application/octet-stream");
    headers = curl_slist_append(headers, "Content-Transfer-Encoding: binary");
    stor::setopt(han, CURLOPT_HTTPHEADER, headers);

    //debugOutput("getOneDQMEventFromSM   AAA");
    // send the HTTP POST, read the reply, and cleanup before going on
    CURLcode messageStatus = curl_easy_perform(han);
    curl_slist_free_all(headers);
    curl_easy_cleanup(han);
    //debugOutput("getOneDQMEventFromSM   BBB");

    if(messageStatus!=0)
    { 
      cerr << "curl perform failed for DQM event" << endl;
      edm::LogError("getOneDQMEventFromSM") << "curl perform failed for DQM event. "
        << "Could not get DQMevent from an already registered Storage Manager"
        << " at " << smURL;

      // keep statistics for all HTTP POSTS
      dqmFetchTimer_.stop();
      ltDQMFetchTimeCounter_->addSample(eventFetchTimer_.realTime());
      stDQMFetchTimeCounter_->addSample(eventFetchTimer_.realTime());

      return false;
    }

    // rely on http transfer string of correct length!
    int len = data.d_.length();
    FDEBUG(9) << "getOneDQMEventFromSM received len = " << len << std::endl;
    if(data.d_.length() == 0)
    { 
      // keep statistics for all HTTP POSTS
      dqmFetchTimer_.stop();
      ltDQMFetchTimeCounter_->addSample(eventFetchTimer_.realTime());
      stDQMFetchTimeCounter_->addSample(eventFetchTimer_.realTime());

      return false;
    }

    buf_.resize(len);
    for (int i=0; i<len ; i++) buf_[i] = data.d_[i];

    // keep statistics for all HTTP POSTS
    dqmFetchTimer_.stop();
    ltDQMFetchTimeCounter_->addSample(eventFetchTimer_.realTime());
    stDQMFetchTimeCounter_->addSample(eventFetchTimer_.realTime());

    // first check if done message
    OtherMessageView msgView(&buf_[0]);

    if (msgView.code() == Header::DONE) {
      // TODO fixme:just print message for now
      std::cout << " SM " << smURL << " has halted" << std::endl;
      return false;
    } else {
      DQMEventMsgView dqmEventView(&buf_[0]);
      ++receivedDQMEvents_;
      addMeasurement((unsigned long)data.d_.length());
      if(dqmServiceManager_.get() != NULL) {
          dqmServiceManager_->manageDQMEventMsg(dqmEventView);
          return true;
      }
    }
    return false;
  }

//////////// ***  Performance //////////////////////////////////////////////////////////
  void DataProcessManager::addMeasurement(unsigned long size)
  {
    // for bandwidth performance measurements
    if(pmeter_->addSample(size))
    {
       stats_ = pmeter_->getStats();
    }
  }

  double DataProcessManager::getSampleCount(STATS_TIME_FRAME timeFrame,
                                            STATS_TIMING_TYPE timingType,
                                            double currentTime)
  {
    if (timeFrame == SHORT_TERM) {
      if (timingType == DQMEVENT_FETCH) {
        return stDQMFetchTimeCounter_->getSampleCount(currentTime);
      }
      else {
        return stEventFetchTimeCounter_->getSampleCount(currentTime);
      }
    }
    else {
      if (timingType == DQMEVENT_FETCH) {
        return ltDQMFetchTimeCounter_->getSampleCount();
      }
      else {
        return ltEventFetchTimeCounter_->getSampleCount();
      }
    }
  }

  double DataProcessManager::getAverageValue(STATS_TIME_FRAME timeFrame,
                                             STATS_TIMING_TYPE timingType,
                                             double currentTime)
  {
    if (timeFrame == SHORT_TERM) {
      if (timingType == DQMEVENT_FETCH) {
        return stDQMFetchTimeCounter_->getValueAverage(currentTime);
      }
      else {
        return stEventFetchTimeCounter_->getValueAverage(currentTime);
      }
    }
    else {
      if (timingType == DQMEVENT_FETCH) {
        return ltDQMFetchTimeCounter_->getValueAverage();
      }
      else {
        return ltEventFetchTimeCounter_->getValueAverage();
      }
    }
  }

  double DataProcessManager::getDuration(STATS_TIME_FRAME timeFrame,
                                         STATS_TIMING_TYPE timingType,
                                         double currentTime)
  {
    if (timeFrame == SHORT_TERM) {
      if (timingType == DQMEVENT_FETCH) {
        return stDQMFetchTimeCounter_->getDuration(currentTime);
      }
      else {
        return stEventFetchTimeCounter_->getDuration(currentTime);
      }
    }
    else {
      if (timingType == DQMEVENT_FETCH) {
        return ltDQMFetchTimeCounter_->getDuration(currentTime);
      }
      else {
        return ltEventFetchTimeCounter_->getDuration(currentTime);
      }
    }
  }
}
