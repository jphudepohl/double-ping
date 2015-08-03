#include "face.hpp"
#include "security/key-chain.hpp"
#include <fstream>

namespace ndn {
namespace examples {

class ServerA : noncopyable
{
public:
  void
  run(char* prefixA)
  {
    std::cout << "\n=== Server A: " << prefixA << " ===\n" << std::endl;

    // set up server
    m_face.setInterestFilter(Name(prefixA),
                             bind(&ServerA::onInterest, this, _1, _2),
                             RegisterPrefixSuccessCallback(),
                             bind(&ServerA::onRegisterFailed, this, _1, _2));

    m_face.processEvents();
  }

private:
  // on receipt of Interest 2, send Data 2 back
  void
  onInterest(const InterestFilter& filter, const Interest& interest2)
  {
    // store receipt of Interest 2 time
    const time::steady_clock::TimePoint& receiveI2Time = time::steady_clock::now();

    std::string iName = interest2.getName().toUri();
    size_t delimiter = iName.find_last_of("/");
    std::string prefix = iName.substr(0,delimiter);
    std::string seq = iName.substr(delimiter+1);

    std::cout << "Received Interest 2 : " << prefix << " - Ping Reference = " << seq << std::endl;

    // create new name based on interest's name
    Name dataName(interest2.getName());
    dataName
      .append("testApp") // add "testApp" component to Interest name
      .appendVersion();  // add "version" component (current UNIX timestamp in milliseconds)

    // random content
    static const std::string content = "HELLO KITTY";

    // create data packet
    shared_ptr<Data> data = make_shared<Data>();
    data->setName(dataName);
    data->setFreshnessPeriod(time::seconds(10));
    data->setContent(reinterpret_cast<const uint8_t*>(content.c_str()), content.size());

    // sign data packet with default identity
    m_keyChain.sign(*data);

    // store send Data 2 time
    const time::steady_clock::TimePoint& sendD2Time = time::steady_clock::now();
    
    // return data packet to requester
    m_face.put(*data);

    std::cout << "Sending Data 2      : " << prefix << " - Ping Reference = " << seq << std::endl;

    // convert stored times to ints
    auto receiveI2_se = receiveI2Time.time_since_epoch();
    m_receiveI2 = time::duration_cast<time::microseconds>(receiveI2_se).count();
    auto sendD2_se = sendD2Time.time_since_epoch();
    m_sendD2 = time::duration_cast<time::microseconds>(sendD2_se).count();

    writeToFile();
  }

  void
  onRegisterFailed(const Name& prefix, const std::string& reason)
  {
    std::cerr << "ERROR: Failed to register prefix \""
              << prefix << "\" in local hub's daemon (" << reason << ")"
              << std::endl;
    m_face.shutdown();
  }

  void
  writeToFile()
  {
    std::ofstream myfile;
    myfile.open ("serverA.txt", std::ofstream::app);
    myfile << m_receiveI2 << "\n";
    myfile << m_sendD2 << "\n";
    myfile.close();
  }

private:
  Face m_face;
  KeyChain m_keyChain;
  int m_receiveI2; // time that Machine A received Interest 2
  int m_sendD2; // time that Machine A sent Data 2
};

} // namespace examples
} // namespace ndn

int
main(int argc, char** argv)
{
  // command line argument is prefix of server to advertise
  // print error message if user does not provide prefix
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " [prefix of Server A]" << std::endl;
    return 1;
  }
  char* prefixA = argv[1];
  ndn::examples::ServerA serverA;
  try {
    serverA.run(prefixA);
  }
  catch (const std::exception& e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
  }
  return 0;
}
