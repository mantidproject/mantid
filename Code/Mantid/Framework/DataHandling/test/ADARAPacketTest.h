#ifndef MANTID_DATAHANDLING_ADARAPACKETTEST_H_
#define MANTID_DATAHANDLING_ADARAPACKETTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/ADARAParser.h"
#include "boost/shared_ptr.hpp"
#include "Poco/DOM/DOMParser.h"  // for parsing the XML device descriptions


class ADARAPacketTest : public CxxTest::TestSuite,
                               ADARA::Parser
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ADARAPacketTest *createSuite() { return new ADARAPacketTest(); }
  static void destroySuite( ADARAPacketTest *suite ) { delete suite; }

  ADARAPacketTest():
    ADARA::Parser( 1024 * 1024, 1024 * 1024)
    // set the initial buffer size equal to the max packet size.  This should ensure that the parser will
    // never resize its buffer.  See below for why this is important
  {
    // We know the parser's buffer is empty now and we've ensured that the address will
    // never change.  Thus, we can verify that the buffer is empty at any time in the
    // future by calling bufferFillAddress() and comparing it to this value.
    m_initialBufferAddr = bufferFillAddress();
  }

  void testDeviceDescriptorPacket()
  {
    boost::shared_ptr<ADARA::DeviceDescriptorPkt> pkt =
        basicPacketTests<ADARA::DeviceDescriptorPkt>( m_devDesPkt, sizeof(m_devDesPkt), 726785379, 0);
    if (pkt != NULL)
    {
      // Basic XML validation
      Poco::XML::DOMParser parser;
      TS_ASSERT_THROWS_NOTHING( parser.parseMemory( pkt->description().c_str(), pkt->description().length()));
    }
  }

  void testRunStatusPacketParser()
  {
    boost::shared_ptr<ADARA::RunStatusPkt> pkt =
        basicPacketTests<ADARA::RunStatusPkt>( m_runStatusPkt, sizeof(m_runStatusPkt), 728504568, 5625794);

    if( pkt != NULL)
    {
      TS_ASSERT_EQUALS( pkt->runNumber(), 13247);
      TS_ASSERT_EQUALS( pkt->runStart(), 728503297);
      TS_ASSERT_EQUALS( pkt->status(), ADARA::RunStatus::STATE);

      // TODO: Find a different RunStatus packet who's status is NOT STATE, then check
      // its file number
      //TS_ASSERT_EQUALS( pkt->fileNumber(), ?????);
    }
  }

  void testSyncPacketParser()
  {
    // the basic tests cover everything in the sync packet
    basicPacketTests<ADARA::SyncPkt>(m_syncPkt, sizeof(m_syncPkt), 728504568, 5617153);
  }

  void testVariableDoublePacketParser()
  {
    boost::shared_ptr<ADARA::VariableDoublePkt> pkt =
        basicPacketTests<ADARA::VariableDoublePkt>( m_variableDoublePkt, sizeof(m_variableDoublePkt), 728281149, 0);

    if( pkt != NULL)
    {
      TS_ASSERT_EQUALS( pkt->devId(),    2);
      TS_ASSERT_EQUALS( pkt->varId(),    1);
      TS_ASSERT_EQUALS( pkt->status(),   0);
      TS_ASSERT_EQUALS( pkt->severity(), 0);
      TS_ASSERT_EQUALS( pkt->value(),    5.0015);
      // Note: we're not allowing for any rounding errors here. Might have to for some values...
    }
  }


protected:
  // The rxPacket() functions just make a copy of the packet available in the public member
  // The test class will handle everything from there.
#define DEFINE_RX_PACKET( PktType) \
  virtual bool rxPacket( const PktType &pkt) { m_pkt.reset( new PktType( pkt)); return false; }

  DEFINE_RX_PACKET( ADARA::RawDataPkt)
  DEFINE_RX_PACKET( ADARA::RTDLPkt)
  DEFINE_RX_PACKET( ADARA::SourceListPkt)
  DEFINE_RX_PACKET( ADARA::BankedEventPkt)
  DEFINE_RX_PACKET( ADARA::BeamMonitorPkt)
  DEFINE_RX_PACKET( ADARA::PixelMappingPkt)
  DEFINE_RX_PACKET( ADARA::RunStatusPkt)
  DEFINE_RX_PACKET( ADARA::RunInfoPkt)
  DEFINE_RX_PACKET( ADARA::TransCompletePkt)
  DEFINE_RX_PACKET( ADARA::ClientHelloPkt)
  DEFINE_RX_PACKET( ADARA::AnnotationPkt)
  DEFINE_RX_PACKET( ADARA::SyncPkt)
  DEFINE_RX_PACKET( ADARA::HeartbeatPkt)
  DEFINE_RX_PACKET( ADARA::GeometryPkt)
  DEFINE_RX_PACKET( ADARA::BeamlineInfoPkt)
  DEFINE_RX_PACKET( ADARA::DeviceDescriptorPkt)
  DEFINE_RX_PACKET( ADARA::VariableU32Pkt)
  DEFINE_RX_PACKET( ADARA::VariableDoublePkt)
  DEFINE_RX_PACKET( ADARA::VariableStringPkt)

  // Call the base class rxPacket(const ADARA::Packet &pkt) which will eventually result
  // in the execution of one of the rxPacket() functions defined above
  virtual bool rxPacket(const ADARA::Packet &pkt) { return ADARA::Parser::rxPacket( pkt); }

private:

  //boost::shared_ptr<Mantid::DataHandling::SNSLiveEventDataListener> sns_l;
  unsigned char *m_initialBufferAddr;
  boost::shared_ptr <ADARA::Packet> m_pkt;

  // A template function that covers the basic tests all packet
  // types have to pass.
  // Returns a shared pointer to the packet so further tests can
  // be conducted.
  template <class T>
  boost::shared_ptr<T> basicPacketTests( const unsigned char *data, unsigned len,
                                         unsigned pulseHigh, unsigned pulseLow)
  {
    parseOnePacket( data, len);

    // verify that we can cast the packet to the type we expect it to be
    boost::shared_ptr<T> pkt = boost::dynamic_pointer_cast<T>(m_pkt);
    TS_ASSERT( pkt != NULL);

    // Make sure we have a valid packet before attempting the remaining tests
    if (pkt != NULL)
    {
      TS_ASSERT_EQUALS( pkt->packet_length(), len)
      TS_ASSERT_EQUALS( pkt->payload_length(), len - sizeof( ADARA::Header))

      TS_ASSERT( pulseIdCompare( pkt->pulseId(), pulseHigh, pulseLow))
    }

    return pkt;
  }

  //
  // Helper functions for basicPacketTests()
  //


  // Calls the necessary parser functions to update m_pkt
  // Expects a single packet.  If there's more than one packet in len bytes,
  // then this function will assert
  // m_pkt is updated by the rxPacket functions which are called (eventually) from bufferParse()
  void parseOnePacket( const unsigned char *data, unsigned len)
  {
    m_pkt.reset();
    unsigned bufferLen = bufferFillLength();
    TS_ASSERT( bufferLen > 0);
    TS_ASSERT( bufferLen > len);
    // Yes, len will always be greater than 0.  I want a specific warning if dataLen is 0

    unsigned char *bufferAddr = bufferFillAddress();
    TS_ASSERT( bufferAddr != 0);
    TS_ASSERT( bufferAddr == m_initialBufferAddr);  // verify that there's nothing in the buffer

    memcpy( bufferAddr, data, len);
    bufferBytesAppended( len);

    int packetsParsed = 0;
    TS_ASSERT_THROWS_NOTHING( (packetsParsed = bufferParse( 1)));
    TS_ASSERT( packetsParsed == 1);
    TS_ASSERT( m_pkt != boost::shared_ptr<ADARA::Packet>());  // verify m_pkt has been updated

    TS_ASSERT( bufferParse( 0) == 0); // try to parse again, make sure there's nothing to parse
    TS_ASSERT( bufferFillAddress() == m_initialBufferAddr);  // verify that there's nothing in the buffer
  }


  // Make it easy to compare the actual pulse ID value to the formatted
  // value that displayed in various parser utils
  bool pulseIdCompare( uint64_t pulseId, uint32_t high, uint32_t low)
  { return ( ((pulseId>>32) == high) && ((pulseId & 0xFFFFFFFF) == low) ); }

  // All of the sample packets that we need to run the test are defined in the following
  // header.  The packets can get pretty long, which is why I didn't want them cluttering
  // up this file.
  #include "ADARAPackets.h"

};

#endif /* MANTID_DATAHANDLING_ADARAPACKETTEST_H_ */
