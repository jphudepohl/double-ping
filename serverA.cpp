#include "face.hpp"
#include "security/key-chain.hpp"
#include <fstream>

namespace ndn {
namespace examples {

class ServerA : noncopyable
{
public:
  void
  run()
  {
    std::cout << "\n--- Server /serverA ---\n" << std::endl;

    // set up server
    m_face.setInterestFilter("/serverA",
                             bind(&ServerA::onInterest, this, _1, _2),
                             RegisterPrefixSuccessCallback(),
                             bind(&ServerA::onRegisterFailed, this, _1, _2));

    m_face.processEvents();
  }

private:
  // on receipt of Interest 2, send Data 2 back
  void
  onInterest(const InterestFilter& filter, const Interest& interest)
  {
    const time::steady_clock::TimePoint& receiveI2Time = time::steady_clock::now();
    auto ri2_se = receiveI2Time.time_since_epoch();
    ri2 = time::duration_cast<time::microseconds>(ri2_se).count();

    std::cout << "<< Received Interest 2: " << interest << std::endl;
    std::cout << "At Time: " << receiveI2Time << std::endl;

    // Create new name, based on Interest's name
    Name dataName(interest.getName());
    dataName
      .append("testApp") // add "testApp" component to Interest name
      .appendVersion();  // add "version" component (current UNIX timestamp in milliseconds)

    static const std::string content = "HELLO KITTY";

    // Create Data packet
    shared_ptr<Data> data = make_shared<Data>();
    data->setName(dataName);
    data->setFreshnessPeriod(time::seconds(10));
    data->setContent(reinterpret_cast<const uint8_t*>(content.c_str()), content.size());

    // Sign Data packet with default identity
    m_keyChain.sign(*data);
    // m_keyChain.sign(data, <identityName>);
    // m_keyChain.sign(data, <certificate>);

    const time::steady_clock::TimePoint& sendD2Time = time::steady_clock::now();
    auto sd2_se = sendD2Time.time_since_epoch();
    sd2 = time::duration_cast<time::microseconds>(sd2_se).count();

    // Return Data packet to the requester
    std::cout << "\n>> Sending Data 2: " << *data << "At Time: " << sendD2Time << std::endl;
    
    m_face.put(*data);

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
    myfile.open ("serverA.txt");
    myfile << ri2 << "\n";
    myfile << sd2;
    myfile.close();
  }

private:
  Face m_face;
  KeyChain m_keyChain;
  int ri2;
  int sd2;
};

} // namespace examples
} // namespace ndn

int
main(int argc, char** argv)
{
  ndn::examples::ServerA serverA;
  try {
    serverA.run();
  }
  catch (const std::exception& e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
  }
  return 0;
}
