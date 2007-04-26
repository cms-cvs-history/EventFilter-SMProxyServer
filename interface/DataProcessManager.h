#ifndef SMPS_DATA_PROCESS_MANAGER_HPP
#define SMPS_DATA_PROCESS_MANAGER_HPP
// $Id$

#include "EventFilter/StorageManager/interface/EventServer.h"
#include "EventFilter/StorageManager/interface/DQMEventServer.h"

#include "IOPool/Streamer/interface/EventBuffer.h"
#include "IOPool/Streamer/interface/EventMessage.h"
#include "IOPool/Streamer/interface/Messages.h"

#include "boost/shared_ptr.hpp"
#include "boost/thread/thread.hpp"

#include <sys/time.h>
#include <string>
#include <vector>

namespace stor
{

  class DataProcessManager
  {
  public:
    typedef std::vector<char> Buf;
    typedef std::map<std::string, unsigned int> RegConsumer_map;

    DataProcessManager();

    ~DataProcessManager();

    void start();
    void stop();
    void join();

    void setEventServer(boost::shared_ptr<EventServer>& es)
    {
      eventServer_ = es;
    }
    boost::shared_ptr<EventServer>& getEventServer() { return eventServer_; }

    void setDQMEventServer(boost::shared_ptr<DQMEventServer>& es)
    {
      DQMeventServer_ = es;
    }
    boost::shared_ptr<DQMEventServer>& getDQMEventServer() { return DQMeventServer_; }

    edm::EventBuffer& getCommandQueue() { return *cmd_q_; }

    void setConsumerName(std::string s) { consumerName_ = s; }
    void setDQMConsumerName(std::string s) { DQMconsumerName_ = s; }

    void addSM2Register(std::string smURL);
    void addDQMSM2Register(std::string DQMsmURL);
    bool haveRegWithDQMServer();
    bool haveRegWithEventServer();
    bool haveHeader();
    unsigned int headerSize();
    std::vector<unsigned char> getHeader();

  private:
    void init();
    void processCommands();
    static void run(DataProcessManager*);

    bool registerWithAllSM();
    bool registerWithAllDQMSM();
    int registerWithSM(std::string smURL);
    int registerWithDQMSM(std::string smURL);
    bool getAnyHeaderFromSM();
    bool getHeaderFromSM(std::string smURL);
    void waitBetweenRegTrys();

    edm::EventBuffer* cmd_q_;

    unsigned int  ser_prods_size_;
    std::vector<unsigned char> serialized_prods_;
    std::vector<unsigned char> buf_;

    std::vector<std::string> smList_;
    RegConsumer_map smRegMap_;
    std::vector<std::string> DQMsmList_;
    RegConsumer_map DQMsmRegMap_;
    std::string regpage_;
    std::string DQMregpage_;
    std::string headerpage_;
    char subscriptionurl_[2048];

    std::string consumerName_;
    std::string consumerPriority_;
    std::string consumerPSetString_;
    int headerRetryInterval_; // seconds
    std::string DQMconsumerName_;
    std::string DQMconsumerPriority_;
    std::string consumerTopFolderName_;

    boost::shared_ptr<EventServer> eventServer_;
    boost::shared_ptr<DQMEventServer> DQMeventServer_;

    boost::shared_ptr<boost::thread> me_;
  };
}

#endif
