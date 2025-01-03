#pragma once

#include <cstdint>
#include <map>
#include <stdexcept>
#include <string>

#include "ADARA.h"
#include "ADARAPackets.h"
#include "MantidLiveData/DllConfig.h"

namespace ADARA {

/** A class for parsing the data stream from SMS and instantiating packet
    objects.  Although not technically an abstract class, this class
    doesn't actually do anything with the instantiated packet objects.
    It is expected that specific implementations will inherit from this
    class and override the rxPacket() functions for the packets they care
    about.

    Copyright &copy; 2012 Oak Ridge National Laboratory
 **/
class MANTID_LIVEDATA_DLL Parser {
public:
  /// Constructor
  Parser(uint32_t initial_buffer_size = 1024 * 1024, uint32_t max_pkt_size = 8 * 1024 * 1024);

  /// Destructor
  virtual ~Parser();

  int64_t last_bytes_read;
  int64_t last_last_bytes_read;
  int64_t last_pkts_parsed;
  int64_t last_last_pkts_parsed;
  uint64_t last_total_bytes;
  uint64_t last_last_total_bytes;
  uint32_t last_total_packets;
  uint32_t last_last_total_packets;
  uint32_t last_read_count;
  uint32_t last_last_read_count;
  uint32_t last_loop_count;
  uint32_t last_last_loop_count;
  double last_parse_elapsed_total;
  double last_last_parse_elapsed_total;
  double last_read_elapsed_total;
  double last_last_read_elapsed_total;
  double last_parse_elapsed;
  double last_last_parse_elapsed;
  double last_read_elapsed;
  double last_last_read_elapsed;
  double last_elapsed;
  double last_last_elapsed;

protected:
  /** @name Buffer Manipulation Functions
   * The ADARA::Parser class maintains an internal buffer that
   * subclasses and direct users must fill with stream data for
   * parsing.
   *
   * bufferFillAddress() returns the address at which to begin
   * placing additional data. bufferFillLength() returns the
   * maximum amount of data that can be appended at that address.
   * The address is guaranteed to be non-NULL if the length is
   * non-zero, but will be NULL if length is zero.
   * Users must not cache the return values from these functions
   * over calls to bufferBytesAppended() or parse().
   *
   * Once data has been placed in the specified buffer, the user
   * must call bufferBytesAppended() to inform the class how much
   * new data has been placed in the buffer.
   **/
  /**@{*/
  uint8_t *bufferFillAddress() const {
    if (bufferFillLength())
      return m_buffer + m_len;
    return nullptr;
  }

  unsigned int bufferFillLength() const { return m_size - m_len; }

  void bufferBytesAppended(unsigned int count) {
    if (bufferFillLength() < count) {
      const char *msg = "attempting to append too much data";
      throw std::logic_error(msg);
    }

    m_len += count;
  }
  /**@}*/

  /** ADARA::Parser::bufferParse() parses the packets in the internal
   * buffer, and calls the appropriate virtual functions for each one.
   * The caller may specify the maximum number of packets to parse in
   * a batch, with zero indicating parse until the buffer is exhausted.
   *
   * bufferParse() returns a positive integer indicating the number of
   * packets parsed if none of the callbacks returned true (requesting
   * stop), a negative number indicating packets parsed before a
   * callback requested a stop, or zero if no packets were completed.
   *
   * Partial packet chunks will be counted as completed when the last
   * fragment is processed.
   **/
  int bufferParse(std::string &log_info, unsigned int max_packets = 0);

  /** Flush the internal buffers and get ready to restart parsing.
   **/
  virtual void reset();

  /** @name Generic rxPacket Functions
   * The rxPacket function gets called for every packet that fits in the
   * internal buffer; oversize packets will be sent to rxOversizePkt().
   * The default implementation will create an appropriate object
   * for the packet and call the delivery function with that object.
   *
   * This function returns true to interrupt parsing, or false
   * to continue.
   *
   * Derived classes my efficiently ignore packet types by overriding
   * this handler. They would just return when the packet type is
   * one they do not care about, and call the base class function
   * to handle the rest.
   **/
  /**@{*/
  virtual bool rxPacket(const Packet &pkt);
  virtual bool rxUnknownPkt(const Packet &pkt);
  virtual bool rxOversizePkt(const PacketHeader *hdr, const uint8_t *chunk, unsigned int chunk_offset,
                             unsigned int chunk_len);
  /**@}*/

  /** @name Specific rxPacket Functions
   * These member functions are passed a constant object representing
   * the packet being processed. They should make copy of the object
   * if they wish to keep it around, as the passed object will be
   * destroyed upon return.
   *
   * These functions return true to interrupt parsing, or false
   * to continue.
   *
   * Note that the implementations of these functions do nothing. It
   * is expected that a derived class will override them with
   * implementations appropriate for the application.
   **/
  /**@{*/
  virtual bool rxPacket(const RawDataPkt &pkt);
  virtual bool rxPacket(const MappedDataPkt &pkt);
  virtual bool rxPacket(const RTDLPkt &pkt);
  virtual bool rxPacket(const SourceListPkt &pkt);
  virtual bool rxPacket(const BankedEventPkt &pkt);
  virtual bool rxPacket(const BeamMonitorPkt &pkt);
  virtual bool rxPacket(const PixelMappingPkt &pkt);
  virtual bool rxPacket(const RunStatusPkt &pkt);
  virtual bool rxPacket(const RunInfoPkt &pkt);
  virtual bool rxPacket(const TransCompletePkt &pkt);
  virtual bool rxPacket(const ClientHelloPkt &pkt);
  virtual bool rxPacket(const AnnotationPkt &pkt);
  virtual bool rxPacket(const SyncPkt &pkt);
  virtual bool rxPacket(const HeartbeatPkt &pkt);
  virtual bool rxPacket(const GeometryPkt &pkt);
  virtual bool rxPacket(const BeamlineInfoPkt &pkt);
  virtual bool rxPacket(const BeamMonitorConfigPkt &pkt);
  virtual bool rxPacket(const DetectorBankSetsPkt &pkt);
  virtual bool rxPacket(const DataDonePkt &pkt);
  virtual bool rxPacket(const DeviceDescriptorPkt &pkt);
  virtual bool rxPacket(const VariableU32Pkt &pkt);
  virtual bool rxPacket(const VariableDoublePkt &pkt);
  virtual bool rxPacket(const VariableStringPkt &pkt);
  /**@}*/

  /* Collect a log string with statistics on "discarded" packet types,
   * i.e. packets that for one reason or another were _Not_ parsed
   * or processed.
   *
   * Since we don't have a common logging approach in this software suite
   * (<sigh/>), we just fill up a happy logging string with info
   * and return it for the caller's logger du jour.
   */
  void getDiscardedPacketsLogString(std::string &log_info);

  /* Reset the collected "discarded packet" statistics.
   */
  void resetDiscardedPacketsStats();

private:
  uint8_t *m_buffer;
  unsigned int m_size;
  unsigned int m_max_size;
  unsigned int m_len;

  unsigned int m_restart_offset;
  unsigned int m_oversize_len;
  unsigned int m_oversize_offset;

  std::map<PacketType::Type, uint64_t> m_discarded_packets;
};

} // namespace ADARA
