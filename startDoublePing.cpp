#include "face.hpp"
#include <fstream>

namespace ndn {
namespace examples {

class Consumer : noncopyable
{
public:
  void
  run(char* name)
  {
    Name pingPacketName(name);
    Interest interest(pingPacketName);
    interest.setInterestLifetime(time::milliseconds(1000)); // TODO: this might need to be longer
    interest.setMustBeFresh(true);

    const time::steady_clock::TimePoint& sendI1Time = time::steady_clock::now();

    m_face.expressInterest(interest,
                           bind(&Consumer::onData, this,  _1, _2, sendI1Time),
                           bind(&Consumer::onTimeout, this, _1));

    std::cout << "\n>> Sending Interest 1: " << interest << std::endl;

    // store time to print statistics
    auto si1_se = sendI1Time.time_since_epoch();
    si1 = time::duration_cast<time::microseconds>(si1_se).count();

    // processEvents will block until the requested data received or timeout occurs
    m_face.processEvents();
  }

  void
  printStatistics()
  {
    // sleep to make sure files are done being written to
    sleep(5);

    std::string line;
    std::ifstream infile;
    // get data from server A
    int ri2;
    int sd2;
    infile.open ("serverA.txt");
    if (infile.is_open()) {
      std::getline(infile,line);
      ri2 = std::stoi(line);
      std::getline(infile,line);
      sd2 = std::stoi(line);
    }
    infile.close();
    // get data from server B
    int ri1;
    int si2;
    int rd2;
    int sd1;
    infile.open ("serverB.txt");
    if (infile.is_open()) {
      std::getline(infile,line);
      ri1 = std::stoi(line);
      std::getline(infile,line);
      si2 = std::stoi(line);
      std::getline(infile,line);
      rd2 = std::stoi(line);
      std::getline(infile,line);
      sd1 = std::stoi(line);
    }
    infile.close();

    std::cout << "Double ping finished successfully. Printing statistics:\n";
    // print interest statistics
    float interest_rtt = (ri2 - si1)/1000.0;
    std::cout << "Interest RTT: " << interest_rtt << " ms\n";
    float makeI2 = (si2 - ri1)/1000.0;
    std::cout << "Make Interest 2 Time: " << makeI2 << " ms\n";
    float interest_travel = interest_rtt - makeI2;
    std::cout << "Interest Travel Time: " << interest_travel << " ms\n";

    // print data statistics
    float data_rtt = (rd1 - sd2)/1000.0;
    std::cout << "Data RTT: " << data_rtt << " ms\n";
    float makeD2 = (sd1 - rd2)/1000.0;
    std::cout << "Make Data 1 Time: " << makeD2 << " ms\n";
    float data_travel = data_rtt - makeD2;
    std::cout << "Data Travel Time: " << data_travel << " ms\n";
  }

private:
  void
  onData(const Interest& interest, const Data& data, const time::steady_clock::TimePoint& sendI1Time)
  {
    const time::steady_clock::TimePoint& receiveD1Time = time::steady_clock::now();

    std::cout << "\n<< Received Data 1: " << data << std::endl;

    // store time to use when print statistics
    auto rd1_se = receiveD1Time.time_since_epoch();
    rd1 = time::duration_cast<time::microseconds>(rd1_se).count();
  }

  void
  onTimeout(const Interest& interest)
  {
    std::cout << "Timeout of Interest 1: " << interest << std::endl;
  }

private:
  Face m_face;
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
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " [name]\n"
      "The first part of [name] should match the prefix that Server B is advertising.\n";
    return 1;
  }
  char* name = argv[1];
  ndn::examples::Consumer consumer;
  try {
    consumer.run(name);
    consumer.printStatistics();
  }
  catch (const std::exception& e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
  }
  return 0;
}
