#include <string.h>

#include "MantidLiveData/ADARA/ADARAParser.h"

using namespace ADARA;

/* ------------------------------------------------------------------------ */

Parser::Parser(unsigned int initial_buffer_size, unsigned int max_pkt_size)
    : m_size(initial_buffer_size), m_max_size(max_pkt_size), m_len(0),
      m_restart_offset(0), m_oversize_len(0) {
  m_buffer = new uint8_t[initial_buffer_size];
}

Parser::~Parser() { delete[] m_buffer; }

void Parser::reset(void) {
  m_len = 0;
  m_oversize_len = 0;
  m_restart_offset = 0;
}

int Parser::bufferParse(unsigned int max_packets) {
  unsigned int valid_len = m_len - m_restart_offset;
  uint8_t *p = m_buffer + m_restart_offset;
  unsigned int processed = 0;
  bool stopped = false;

  /* Is there anything to do? */
  if (!valid_len)
    return 0;

  /* If we don't care how many packets we process, then set the limit
   * above the range of possibility to avoid needing to check for zero
   * multiple times.
   */
  if (!max_packets)
    max_packets = m_size;

  /* If we're processing an oversize packet, then we will find its
   * data at the front of the buffer. We'll either consume our
   * entire buffer, or find the end of the oversize packet and
   * process the rest of the buffer as normal.
   */
  if (m_oversize_len) {
    unsigned int chunk_len;

    chunk_len = m_oversize_len;
    if (valid_len < chunk_len)
      chunk_len = valid_len;
    stopped = rxOversizePkt(NULL, p, m_oversize_offset, chunk_len);
    m_oversize_offset += chunk_len;
    m_oversize_len -= chunk_len;
    valid_len -= chunk_len;
    p += chunk_len;

    /* Did we finish this packet? */
    if (!m_oversize_len)
      processed++;
  }

  while (valid_len >= PacketHeader::header_length() &&
         processed < max_packets && !stopped) {
    PacketHeader hdr(p);

    if (hdr.payload_length() % 4)
      throw invalid_packet("Payload length not "
                           "multiple of 4");

    if (m_max_size < hdr.packet_length()) {
      /* This packet is over the maximum limit; we'll
       * call the oversize handler with this first
       * chunk, consuming our entire buffer.
       */
      stopped = rxOversizePkt(&hdr, p, 0, valid_len);
      m_oversize_len = hdr.packet_length() - valid_len;
      m_oversize_offset = valid_len;
      valid_len = 0;
      break;
    }

    if (m_size < hdr.packet_length()) {
      /* This packet is too big to possibly fit in our
       * current buffer, so we need to grow. Once we've
       * resized, return to our caller as we obviously
       * don't have the full packet yet.
       */
      unsigned int new_size = m_size;
      uint8_t *new_buffer;

      do {
        new_size *= 2;
      } while (new_size < hdr.packet_length());

      if (new_size > m_max_size)
        new_size = m_max_size;

      new_buffer = new uint8_t[new_size];
      memcpy(new_buffer, p, valid_len);

      delete[] m_buffer;
      m_buffer = new_buffer;
      m_size = new_size;

      /* We moved the data to the front of the buffer as
       * part of the resize; account for that.
       */
      m_restart_offset = 0;
      m_len = valid_len;
      return processed;
    }

    if (valid_len < hdr.packet_length())
      break;

    Packet pkt(p, hdr.packet_length());

    p += hdr.packet_length();
    valid_len -= hdr.packet_length();

    stopped = rxPacket(pkt);
    processed++;
  }

  /* We're done processing for this round. Update our position and/or
   * amount of buffered data so that we restart in the correct spot
   * on our next call.
   *
   * We only need to move data if we ran out of data to process --
   * ie, we processed fewer packets than requested without being
   * stopped by a callback. This moves any possible fragment of a
   * packet to the front, maximizing the room for more data. If this
   * occurs coincidentally with a stop request, the next call to
   * to bufferParse() will only see the fragment and stop, but that
   * should be rare.
   */
  if (valid_len) {
    if (!stopped && processed < max_packets) {
      if (p != m_buffer)
        memmove(m_buffer, p, valid_len);
      m_len = valid_len;
      m_restart_offset = 0;
    } else {
      /* We know that the offset will fit into an unsigned
       * int, as that is the type we use for the buffer size.
       */
      m_restart_offset = (unsigned int)(p - m_buffer);
    }
  } else {
    /* We used up the buffer. */
    m_len = 0;
    m_restart_offset = 0;
  }

  /* We need an 32 GB buffer before we can fit 2^31 packets, so
   * casting to int is safe here.
   */
  return stopped ? -(int)processed : (int)processed;
}

bool Parser::rxPacket(const Packet &pkt) {
#define MAP_TYPE(pkt_type, obj_type)                                           \
  case pkt_type: {                                                             \
    obj_type raw(pkt.packet(), pkt.packet_length());                           \
    return rxPacket(raw);                                                      \
  }

  switch (pkt.type()) {
    MAP_TYPE(PacketType::RAW_EVENT_V0, RawDataPkt);
    MAP_TYPE(PacketType::RTDL_V0, RTDLPkt);
    MAP_TYPE(PacketType::SOURCE_LIST_V0, SourceListPkt);
    MAP_TYPE(PacketType::BANKED_EVENT_V0, BankedEventPkt);
    MAP_TYPE(PacketType::BEAM_MONITOR_EVENT_V0, BeamMonitorPkt);
    MAP_TYPE(PacketType::PIXEL_MAPPING_V0, PixelMappingPkt);
    MAP_TYPE(PacketType::RUN_STATUS_V0, RunStatusPkt);
    MAP_TYPE(PacketType::RUN_INFO_V0, RunInfoPkt);
    MAP_TYPE(PacketType::TRANS_COMPLETE_V0, TransCompletePkt);
    MAP_TYPE(PacketType::CLIENT_HELLO_V0, ClientHelloPkt);
    MAP_TYPE(PacketType::STREAM_ANNOTATION_V0, AnnotationPkt);
    MAP_TYPE(PacketType::SYNC_V0, SyncPkt);
    MAP_TYPE(PacketType::HEARTBEAT_V0, HeartbeatPkt);
    MAP_TYPE(PacketType::GEOMETRY_V0, GeometryPkt);
    MAP_TYPE(PacketType::BEAMLINE_INFO_V0, BeamlineInfoPkt);
    MAP_TYPE(PacketType::DEVICE_DESC_V0, DeviceDescriptorPkt);
    MAP_TYPE(PacketType::VAR_VALUE_U32_V0, VariableU32Pkt);
    MAP_TYPE(PacketType::VAR_VALUE_DOUBLE_V0, VariableDoublePkt);
    MAP_TYPE(PacketType::VAR_VALUE_STRING_V0, VariableStringPkt);

    /* No default handler; we want the compiler to warn about
     * the unhandled PacketType values when we add new packets.
     */
  }

  return rxUnknownPkt(pkt);
#undef MAP_TYPE
}

bool Parser::rxUnknownPkt(const Packet &) {
  /* Default is to discard the data */
  return false;
}

bool Parser::rxOversizePkt(const PacketHeader *, const uint8_t *, unsigned int,
                           unsigned int) {
  /* Default is to discard the data */
  return false;
}

#define EXPAND_HANDLER(type)                                                   \
  bool Parser::rxPacket(const type &) { return false; }

EXPAND_HANDLER(RawDataPkt)
EXPAND_HANDLER(RTDLPkt)
EXPAND_HANDLER(SourceListPkt)
EXPAND_HANDLER(BankedEventPkt)
EXPAND_HANDLER(BeamMonitorPkt)
EXPAND_HANDLER(PixelMappingPkt)
EXPAND_HANDLER(RunStatusPkt)
EXPAND_HANDLER(RunInfoPkt)
EXPAND_HANDLER(TransCompletePkt)
EXPAND_HANDLER(ClientHelloPkt)
EXPAND_HANDLER(AnnotationPkt)
EXPAND_HANDLER(SyncPkt)
EXPAND_HANDLER(HeartbeatPkt)
EXPAND_HANDLER(GeometryPkt)
EXPAND_HANDLER(BeamlineInfoPkt)
EXPAND_HANDLER(DeviceDescriptorPkt)
EXPAND_HANDLER(VariableU32Pkt)
EXPAND_HANDLER(VariableDoublePkt)
EXPAND_HANDLER(VariableStringPkt)
