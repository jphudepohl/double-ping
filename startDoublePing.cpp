#include "face.hpp"
#include "util/scheduler.hpp"
#include "util/random.hpp"
#include <fstream>

namespace ndn {
namespace examples {

class Client : noncopyable
{
public:
  Client(char* name, int numPings)
    : m_name1(name)
    , m_numPings(numPings)
    , m_numSent(0)
    , m_numOutstanding(0)
    , m_face(m_ioService)
    , m_scheduler(m_ioService)
  {
    m_nextSeq = random::generateWord64();
  }

  void
  run()
  {
    performPing();

    m_ioService.run();
  }

private:
  void
  performPing()
  {
    Name pingPacketName(m_name1);
    pingPacketName.append(std::to_string(m_nextSeq));
    
    Interest interest1(pingPacketName);
    interest1.setInterestLifetime(time::milliseconds(1000)); // TODO: this might need to be longer
    interest1.setMustBeFresh(true);

    m_face.expressInterest(interest1,
                           bind(&Client::onData, this,  _1, _2, m_nextSeq, time::steady_clock::now()),
                           bind(&Client::onTimeout, this, _1, m_nextSeq));

    std::cout << "Sending Interest 1: " << m_name1 << " - Ping Reference = " << m_nextSeq << std::endl;
    
    ++m_numSent;
    ++m_nextSeq;
    ++m_numOutstanding;

    if (m_numSent < m_numPings) {
      m_scheduler.scheduleEvent(time::seconds(1), 
                                bind(&Client::performPing, this));
    }
    else {
      finish();
    }
  }

  void
  onData(const Interest& interest, const Data& data, uint64_t seq, const time::steady_clock::TimePoint& sendI1Time)
  {
    const time::steady_clock::TimePoint& receiveD1Time = time::steady_clock::now();

    std::cout << "Received Data 1: " << m_name1 << " - Ping Reference = " << seq << std::endl;

    // store time to use when print statistics
    auto sendI1_se = sendI1Time.time_since_epoch();
    m_sendI1 = time::duration_cast<time::microseconds>(sendI1_se).count();
    auto receiveD1_se = receiveD1Time.time_since_epoch();
    m_receiveD1 = time::duration_cast<time::microseconds>(receiveD1_se).count();

    writeToFile();

    finish();
  }

  void
  onTimeout(const Interest& interest, uint64_t seq)
  {
    std::cout << "Timeout of Interest 1: " << m_name1 << " - Ping Reference = " << seq << std::endl;

    finish();
  }

  void
  finish()
  {
    if (--m_numOutstanding >= 0) {
      return;
    }
    // wait so files done being written to
    sleep(5);
    // shut down
    m_face.shutdown();
    m_face.getIoService().stop();
    // output results to terminal
    printStatistics();
  }

  void
  writeToFile()
  {
    std::ofstream myfile;
    myfile.open ("clientA.txt", std::ofstream::app);
    myfile << m_sendI1 << "\n";
    myfile << m_receiveD1 << "\n";
    myfile.close();
  }

  void
  printStatistics()
  {
    std::cout << "\nDouble ping finished successfully. Printing statistics:" << std::endl;

    std::string line;
    
    std::ifstream finCA;
    finCA.open ("clientA.txt");
    int sI1;
    int rD1;
    std::ifstream finSA;
    finSA.open ("serverA.txt");
    int rI2;
    int sD2;
    std::ifstream finSB;
    finSB.open ("serverB.txt");
    int rI1;
    int sI2;
    int rD2;
    int sD1;

    // average statistics
    float total_Irtt = 0.0;
    float total_genI2 = 0.0;
    float total_Itravel = 0.0;
    float total_Drtt = 0.0;
    float total_genD1 = 0.0;
    float total_Dtravel = 0.0;

    for (int count = 1; count <= m_numSent; count++) {

      // get data from client A
      if (finCA.is_open()) {
        std::getline(finCA,line);
        sI1 = std::stoi(line);
        std::getline(finCA,line);
        rD1 = std::stoi(line);
      }
      // get data from server A
      if (finSA.is_open()) {
        std::getline(finSA,line);
        rI2 = std::stoi(line);
        std::getline(finSA,line);
        sD2 = std::stoi(line);
      }
      // get data from server B
      if (finSB.is_open()) {
        std::getline(finSB,line);
        rI1 = std::stoi(line);
        std::getline(finSB,line);
        sI2 = std::stoi(line);
        std::getline(finSB,line);
        rD2 = std::stoi(line);
        std::getline(finSB,line);
        sD1 = std::stoi(line);
      }

      float interest_rtt = (rI2 - sI1)/1000.0;
      float genI2 = (sI2 - rI1)/1000.0;
      float interest_travel = interest_rtt - genI2;
      float data_rtt = (rD1 - sD2)/1000.0;
      float genD2 = (sD1 - rD2)/1000.0;
      float data_travel = data_rtt - genD2;

      // update total variables for avg calculation
      if (count != 1) {
        total_Irtt += interest_rtt;
        total_genI2 += genI2;
        total_Itravel += interest_travel;
        total_Drtt += data_rtt;
        total_genD1 += genD2;
        total_Dtravel += data_travel;
      }

      std::cout << "--- Double Ping " << count << " ---" << std::endl;
      // print interest statistics
      std::cout << "Interest RTT = " << interest_rtt << " ms" << std::endl;
      std::cout << "Generate Interest 2 Time = " << genI2 << " ms" << std::endl;
      std::cout << "Interest Travel Time = " << interest_travel << " ms" << std::endl;
      // print data statistics
      std::cout << "Data RTT = " << data_rtt << " ms" << std::endl;
      std::cout << "Generate Data 1 Time = " << genD2 << " ms" << std::endl;
      std::cout << "Data Travel Time = " << data_travel << " ms" << std::endl;
    }

    int pingsToCount = m_numPings - 1; // don't count first ping

    // print average statistics
    std::cout << "\n=== Average Statistics (excluding first cycle) ===" << std::endl;
    std::cout << "Interest RTT = " << total_Irtt / pingsToCount << " ms" << std::endl;
    std::cout << "Generate Interest 2 Time = " << total_genI2 / pingsToCount  << " ms" << std::endl;
    std::cout << "Interest Travel Time = " << total_Itravel / pingsToCount << " ms" << std::endl;
    std::cout << "Data RTT = " << total_Drtt / pingsToCount << " ms" << std::endl;
    std::cout << "Generate Data 1 Time = " << total_genD1 / pingsToCount << " ms" << std::endl;
    std::cout << "Data Travel Time = " << total_Dtravel / pingsToCount << " ms" << std::endl;

    // delete log files
    finCA.close();
    finSA.close();
    finSB.close();
    remove("clientA.txt");
    remove("serverA.txt");
    remove("serverB.txt");
  }

private:
  char* m_name1; // name of interest 1 (command line argument)
  int m_numPings; // number of double ping cycles to perform (command line option)
  int m_numSent; // number of double ping cycles that have finished so far
  int m_numOutstanding; // number of interests sent for which no data has been received yet
  uint64_t m_nextSeq; 

  boost::asio::io_service m_ioService;
  Face m_face;
  Scheduler m_scheduler;
  
  int m_sendI1;
  int m_receiveD1;
};

} // namespace examples
} // namespace ndn

int
main(int argc, char** argv)
{
  // command line argument is prefix to ping
  // print error message if user does not provide prefix
  if (argc < 3) {
    std::cerr << "Usage: " << argv[0] << " [name of Interest 1] [number of interests to send]\n"
      "The first part of [name of Interest 1] should match the prefix that Server B is advertising.\n";
    return 1;
  }
  char* name = argv[1];
  int numPings = atoi(argv[2]);
  ndn::examples::Client client (name, numPings);
  try {
    client.run();
  }
  catch (const std::exception& e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
  }
  return 0;
}
