#include "face.hpp"

// Enclosing code in ndn simplifies coding (can also use `using namespace ndn`)
namespace ndn {
// Additional nested namespace could be used to prevent/limit name contentions
namespace examples {

class Consumer : noncopyable
{
public:
  void
  run()
  {
    Interest interest(Name("/serverB/interest1"));
    interest.setInterestLifetime(time::milliseconds(1000)); // TODO: this might need to be longer
    interest.setMustBeFresh(true);

    const time::steady_clock::TimePoint& sendI1Time = time::steady_clock::now();

    m_face.expressInterest(interest,
                           bind(&Consumer::onData, this,  _1, _2, sendI1Time),
                           bind(&Consumer::onTimeout, this, _1));

    std::cout << "\n>> Sending Interest 1: " << interest << std::endl;
    std::cout << "At Time: " << sendI1Time << std::endl;

    // processEvents will block until the requested data received or timeout occurs
    m_face.processEvents();
  }

private:
  void
  onData(const Interest& interest, const Data& data, const time::steady_clock::TimePoint& sendI1Time)
  {
    const time::steady_clock::TimePoint& receiveD1Time = time::steady_clock::now();
    time::nanoseconds rtt = receiveD1Time - sendI1Time;

    std::cout << "\n<< Received Data 1: " << data << "At Time: " << receiveD1Time << std::endl;
    std::cout << "I1 -> D1 RTT: " << rtt << std::endl;
  }

  void
  onTimeout(const Interest& interest)
  {
    std::cout << "Timeout of Interest 1: " << interest << std::endl;
  }

private:
  Face m_face;
};

} // namespace examples
} // namespace ndn

int
main(int argc, char** argv)
{
  ndn::examples::Consumer consumer;
  try {
    consumer.run();
  }
  catch (const std::exception& e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
  }
  return 0;
}
