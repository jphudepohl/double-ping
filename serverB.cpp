#include "face.hpp"
#include "security/key-chain.hpp"
#include <fstream>

namespace ndn {
namespace examples {

class ServerB : noncopyable
{
public:
  ServerB()
  // *** V2 *** uncomment to make data packets in advance
  // : currentDataNum(0)
  {
    m_nextSeq = random::generateWord64();
  }

  void
  run(char* prefixB, char* name2)
  {
    // *** V2 *** uncomment to make data packets in advance
    // generateDataPackets();

    std::cout << "\n=== Server B: " << prefixB << " ===\n" << std::endl;

    // set up server
    m_face.setInterestFilter(Name(prefixB),
                             bind(&ServerB::onInterest, this, _1, _2, name2),
                             RegisterPrefixSuccessCallback(),
                             bind(&ServerB::onRegisterFailed, this, _1, _2));
    m_face.processEvents();
  }

private:
  // on receipt of Interest 1, send Interest 2 back
  void
  onInterest(const InterestFilter& filter, const Interest& interest1, char* name2)
  {
    // store time that received Interest 1
    const time::steady_clock::TimePoint& receiveI1Time = time::steady_clock::now();

    // store incoming Interest 1 name
    m_interestName = interest1.getName();

    std::string iName = m_interestName.toUri();
    size_t delimiter = iName.find_last_of("/");
    std::string prefix = iName.substr(0,delimiter);
    std::string seq = iName.substr(delimiter+1);

    std::cout << "Received Interest 1 : " << prefix << " - Ping Reference = " << seq << std::endl;

    // make Interest 2
    Name pingPacketName(name2);
    pingPacketName.append(std::to_string(m_nextSeq));
    Interest interest2(pingPacketName);
    interest2.setInterestLifetime(time::milliseconds(1000));
    interest2.setMustBeFresh(true);

    // store time that sent Interest 2
    const time::steady_clock::TimePoint& sendI2Time = time::steady_clock::now();

    // send Interest 2 back
    m_face.expressInterest(interest2,
                           bind(&ServerB::onData, this,  _1, _2, sendI2Time),
                           bind(&ServerB::onTimeout, this, _1));

    std::cout << "Sending Interest 2  : " << name2 << " - Ping Reference = " << m_nextSeq << std::endl;

    ++m_nextSeq;

    // convert times to ints
    auto receiveI1_se = receiveI1Time.time_since_epoch();
    m_receiveI1 = time::duration_cast<time::microseconds>(receiveI1_se).count();
    auto sendI2_se = sendI2Time.time_since_epoch();
    m_sendI2 = time::duration_cast<time::microseconds>(sendI2_se).count();
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
    // store time that received Data 2
    const time::steady_clock::TimePoint& receiveD2Time = time::steady_clock::now();

    std::string i2Name = interest.getName().toUri();
    size_t delimiter2 = i2Name.find_last_of("/");
    std::string prefix2 = i2Name.substr(0,delimiter2);
    std::string seq2 = i2Name.substr(delimiter2+1);

    std::cout << "Received Data 2     : " << prefix2 << " - Ping Reference = " << seq2 << std::endl;

    // *** V2 *** uncomment these lines
    // get data out of array here
    // shared_ptr<Data> data1 = arr[currentDataNum];
    // ++currentDataNum;

    // *** V2 *** comment next 15 lines out if making data in advance
    Name dataName = m_interestName;
    dataName
      .append("testApp") // add "testApp" component to interest name
      .appendVersion(); // add "version" component (current UNIX timestamp in milliseconds)

    // dummy content
    static const std::string content = "HELLO KITTY";

    // create data packet
    shared_ptr<Data> data1 = make_shared<Data>();
    data1->setName(dataName);
    data1->setFreshnessPeriod(time::seconds(10)); 
    data1->setContent(reinterpret_cast<const uint8_t*>(content.c_str()), content.size());

    m_keyChain.sign(*data1);
    // *** V2 *** comment out to here if making data in advance

    // store time that sent Data 1
    const time::steady_clock::TimePoint& sendD1Time = time::steady_clock::now();
    
    // return data packet to requester
    m_face.put(*data1);

    std::string i1Name = m_interestName.toUri();
    size_t delimiter1 = i1Name.find_last_of("/");
    std::string prefix1 = i1Name.substr(0,delimiter1);
    std::string seq1 = i1Name.substr(delimiter1+1);

    std::cout << "Sending Data 1      : " << prefix1 << " - Ping Reference = " << seq1 << std::endl;

    // store time to write to file
    auto receiveD2_se = receiveD2Time.time_since_epoch();
    m_receiveD2 = time::duration_cast<time::microseconds>(receiveD2_se).count();
    auto sendD1_se = sendD1Time.time_since_epoch();
    m_sendD1 = time::duration_cast<time::microseconds>(sendD1_se).count();

    writeToFile();
  }

  void
  onTimeout(const Interest& interest)
  {
    std::cout << "Timeout of Interest 2: " << interest << std::endl;
  }

  void
  writeToFile()
  {
    std::ofstream myfile;
    myfile.open ("serverB.txt", std::ofstream::app);
    myfile << m_receiveI1 << "\n";
    myfile << m_sendI2 << "\n";
    myfile << m_receiveD2 << "\n";
    myfile << m_sendD1 << "\n";
    myfile.close();
  }

/* *** V2 *** need this function to make data packets in advance
  void
  generateDataPackets()
  {
    uint64_t seq = 10488217288391642338; // this must match m_nextSeq in startDoublePing.cpp

    for (int i = 0; i < numDataPackets; i++) 
    {
      std::cout << i << std::endl;

      Name dataName("/ndn/edu/wustl/ping1"); // *** V2*** this is hard-coded, change to what makes sense
      dataName
        .append(std::to_string(seq)) // add sequence number
        .append("testApp") // add "testApp" component to Interest name
        .appendVersion();  // add "version" component (current UNIX timestamp in milliseconds)

      // dummy content
      static const std::string content = "HELLO KITTY";

      // create Data packet
      shared_ptr<Data> data1 = make_shared<Data>();
      data1->setName(dataName);
      data1->setFreshnessPeriod(time::seconds(10));
      data1->setContent(reinterpret_cast<const uint8_t*>(content.c_str()), content.size());

      m_keyChain.sign(*data1);

      arr[i] = data1;

      ++seq;
    }
  }*/

private:
  uint64_t m_nextSeq; // ping sequence number
  Face m_face;
  KeyChain m_keyChain;
  Name m_interestName; // name of incoming Interest 1
  int m_receiveI1; // time that Machine B received Interest 1
  int m_sendI2; // time that Machine B sent Interest 2
  int m_receiveD2; // time that Machine B received Data 2
  int m_sendD1; // time that Machine B sent Data 1

  // *** V2 *** uncomment these lines to make data packets in advance
  // static const int numDataPackets = 100;
  // shared_ptr<Data> arr[numDataPackets];
  // int currentDataNum;
};

} // namespace examples
} // namespace ndn

int
main(int argc, char** argv)
{
  // command line arguments: prefix of server to advertise and name of Interest 2
  // print error message if user does not provide prefix
  if (argc < 3) {
    std::cerr << "Usage: " << argv[0] << " [prefix of Server B] [name of Interest 2]\n"
      "The first part of [name of Interest 2] should match the prefix that Server A is advertising.\n";
    return 1;
  }
  char* prefixB = argv[1];
  char* name2 = argv[2];
  ndn::examples::ServerB serverB;
  try {
    serverB.run(prefixB, name2);
  }
  catch (const std::exception& e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
  }
  return 0;
}
