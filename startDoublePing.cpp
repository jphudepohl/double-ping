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
                           bind(&Client::onData, this,  _1, _2, time::steady_clock::now()),
                           bind(&Client::onTimeout, this, _1));

    std::cout << "\n>> Sending Interest 1: " << interest1 << std::endl;
    
    ++m_numSent;
    ++m_nextSeq;
    ++m_numOutstanding;

    if (m_numSent < m_numPings) {
      m_scheduler.scheduleEvent(time::seconds(2), 
                                bind(&Client::performPing, this));
    }
    else {
      finish();
    }
  }

  void
  onData(const Interest& interest, const Data& data, const time::steady_clock::TimePoint& sendI1Time)
  {
    const time::steady_clock::TimePoint& receiveD1Time = time::steady_clock::now();

    std::cout << "\n<< Received Data 1: " << data << std::endl;

    // store time to use when print statistics
    auto si1_se = sendI1Time.time_since_epoch();
    si1 = time::duration_cast<time::microseconds>(si1_se).count();
    auto rd1_se = receiveD1Time.time_since_epoch();
    rd1 = time::duration_cast<time::microseconds>(rd1_se).count();

    writeToFile();

    finish();
  }

  void
  onTimeout(const Interest& interest)
  {
    std::cout << "Timeout of Interest 1: " << interest << std::endl;

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
    myfile << si1 << "\n";
    myfile << rd1 << "\n";
    myfile.close();
  }

  void
  printStatistics()
  {
    std::cout << "Double ping finished successfully. Printing statistics:\n";

    std::string line;
    
    std::ifstream finCA;
    finCA.open ("clientA.txt");
    std::ifstream finSA;
    finSA.open ("serverA.txt");
    int ri2;
    int sd2;
    std::ifstream finSB;
    finSB.open ("serverB.txt");
    int ri1;
    int si2;
    int rd2;
    int sd1;

    for (int count = 1; count <= m_numSent; count++) {

      // get data from client A
      if (finCA.is_open()) {
        std::getline(finCA,line);
        si1 = std::stoi(line);
        std::getline(finCA,line);
        rd1 = std::stoi(line);
      }
      // get data from server A
      if (finSA.is_open()) {
        std::getline(finSA,line);
        ri2 = std::stoi(line);
        std::getline(finSA,line);
        sd2 = std::stoi(line);
      }
      // get data from server B
      if (finSB.is_open()) {
        std::getline(finSB,line);
        ri1 = std::stoi(line);
        std::getline(finSB,line);
        si2 = std::stoi(line);
        std::getline(finSB,line);
        rd2 = std::stoi(line);
        std::getline(finSB,line);
        sd1 = std::stoi(line);
      }

      std::cout << "--- Double Ping " << count << " ---" << std::endl;
      // print interest statistics
      float interest_rtt = (ri2 - si1)/1000.0;
      std::cout << "Interest RTT: " << interest_rtt << " ms" << std::endl;
      float makeI2 = (si2 - ri1)/1000.0;
      std::cout << "Make Interest 2 Time: " << makeI2 << " ms" << std::endl;
      float interest_travel = interest_rtt - makeI2;
      std::cout << "Interest Travel Time: " << interest_travel << " ms" << std::endl;
      // print data statistics
      float data_rtt = (rd1 - sd2)/1000.0;
      std::cout << "Data RTT: " << data_rtt << " ms" << std::endl;
      float makeD2 = (sd1 - rd2)/1000.0;
      std::cout << "Make Data 1 Time: " << makeD2 << " ms" << std::endl;
      float data_travel = data_rtt - makeD2;
      std::cout << "Data Travel Time: " << data_travel << " ms" << std::endl;
    }

    // delete log files
    finCA.close();
    finSA.close();
    finSB.close();
    //remove("clientA.txt");
    //remove("serverA.txt");
    //remove("serverB.txt");
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
  
  int si1;
  int rd1;
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
