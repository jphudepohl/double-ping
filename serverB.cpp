#include "face.hpp"
#include "security/key-chain.hpp"

// Enclosing code in ndn simplifies coding (can also use `using namespace ndn`)
namespace ndn {
// Additional nested namespace could be used to prevent/limit name contentions
namespace examples {

class ServerB : noncopyable
{
public:
  void
  run()
  {
    std::cout << "\n--- Server /serverB ---\n" << std::endl;

    // set up server
    m_face.setInterestFilter("/serverB",
                             bind(&ServerB::onInterest, this, _1, _2),
                             RegisterPrefixSuccessCallback(),
                             bind(&ServerB::onRegisterFailed, this, _1, _2));
    m_face.processEvents();
  }

private:
  // on receipt of Interest 1, send Interest 2 back
  void
  onInterest(const InterestFilter& filter, const Interest& interest1)
  {
    const time::steady_clock::TimePoint& receiveI1Time = time::steady_clock::now();

    std::cout << "<< Received Interest 1: " << interest1 << std::endl;
    std::cout << "At Time: " << receiveI1Time << std::endl;

    // store incoming Interest 1 name
    m_interestName = interest1.getName();

    // send Interest 2 back
    Interest interest2(Name("/serverA/interest2"));
    interest2.setInterestLifetime(time::milliseconds(1000));
    interest2.setMustBeFresh(true);

    const time::steady_clock::TimePoint& sendI2Time = time::steady_clock::now();
    time::nanoseconds makeI2Time = sendI2Time - receiveI1Time;

    m_face.expressInterest(interest2,
                           bind(&ServerB::onData, this,  _1, _2, sendI2Time),
                           bind(&ServerB::onTimeout, this, _1));

    std::cout << "\n>> Sending Interest 2: " << interest2 << std::endl;
    std::cout << "At Time: " << sendI2Time << std::endl;
    std::cout << "Make Interest 2 Time: " << makeI2Time << std::endl;

    // processEvents will block until the requested data received or timeout occurs
    // m_face.processEvents();  *****TODO: not sure if i need this*****
  }


  void
  onRegisterFailed(const Name& prefix, const std::string& reason)
  {
    std::cerr << "ERROR: Failed to register prefix \""
              << prefix << "\" in local hub's daemon (" << reason << ")"
              << std::endl;
    m_face.shutdown();
  }

  // on receipt of Data 2, send Data 1 back
  void
  onData(const Interest& interest, const Data& data2, const time::steady_clock::TimePoint& sendI2Time)
  {
    const time::steady_clock::TimePoint& receiveD2Time = time::steady_clock::now();
    time::nanoseconds rtt = receiveD2Time - sendI2Time;

    std::cout << "\n<< Received Data 2: " << data2 << "At Time: " << receiveD2Time << std::endl;
    std::cout << "Interest 2 -> Data 2 RTT: " << rtt << std::endl;

    m_interestName
      .append("testApp") // add "testApp" component to Interest name
      .appendVersion();  // add "version" component (current UNIX timestamp in milliseconds)

    static const std::string content = "HELLO KITTY";

    // Create Data packet
    shared_ptr<Data> data1 = make_shared<Data>();
    data1->setName(m_interestName);
    data1->setFreshnessPeriod(time::seconds(10));
    data1->setContent(reinterpret_cast<const uint8_t*>(content.c_str()), content.size());

    // Sign Data packet with default identity
    m_keyChain.sign(*data1);
    // m_keyChain.sign(data, <identityName>);
    // m_keyChain.sign(data, <certificate>);

    const time::steady_clock::TimePoint& sendD1Time = time::steady_clock::now();
    time::nanoseconds makeD1Time = sendD1Time - receiveD2Time;

    // Return Data packet to the requester
    std::cout << "\n>> Sending Data 1: " << *data1 << "At Time: " << sendD1Time << std::endl;
    std::cout << "Make Data 1 Time: " << makeD1Time << std::endl;

    m_face.put(*data1);
  }

  void
  onTimeout(const Interest& interest)
  {
    std::cout << "Timeout of Interest 2: " << interest << std::endl;
  }

private:
  Face m_face;
  KeyChain m_keyChain;
  Name m_interestName; // name of incoming interest 1
};

} // namespace examples
} // namespace ndn

int
main(int argc, char** argv)
{
  ndn::examples::ServerB serverB;
  try {
    serverB.run();
  }
  catch (const std::exception& e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
  }
  return 0;
}
