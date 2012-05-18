#ifndef __ADARA_PARSER_H
#define __ADARA_PARSER_H

#include <string>
#include <stdint.h>
#include <time.h>

#include "MantidDataHandling/ADARA.h"
#include "MantidDataHandling/ADARAPackets.h"

namespace Poco {
  namespace Net {
    class StreamSocket;
  }
}

namespace ADARA {

class Parser {
public:
	Parser(unsigned int buffer_size = 1024 * 1024,
	       unsigned int max_pkt_size = 8 * 1024 * 1024);

	virtual ~Parser();

	/* Returns false if we hit EOF or a callback asked to stop. We return
	 * true if we got we got EAGAIN/EINTR from reading the fd. We throw
	 * exceptions on error, but may hold those until we complete all
	 * packets in the buffer. The max_read parameter, if non-zero,
	 * limits the amount of maximum amount of data read and parsed
	 * from the file descriptor.
	 */
	bool read(int fd, unsigned int max_read = 0);

        // Similar semantics as above: returns true if a timeout is set on the
        // socket and we hit the timeout before reading max_read bytes.
        // Returns false if the socket has been shut down and throws an assert
        // on errors
        bool read(Poco::Net::StreamSocket &stream, unsigned int max_read = 0);

	/* Flush the internal buffers and get ready to restart parsing.
	 */
	void reset(void);

protected:
	/* This function gets called for every packet that fits in the
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
	 */
	virtual bool rxPacket(const Packet &pkt);
	virtual bool rxUnknownPkt(const Packet &pkt);
	virtual bool rxOversizePkt(const PacketHeader *hdr,
				   const uint8_t *chunk,
				   unsigned int chunk_offset,
				   unsigned int chunk_len);

	/* These member functions are passed a constant object representing
	 * the packet being processed. They should make copy of the object
	 * if they wish to keep it around, as the passed object will be
	 * destroyed upon return.
	 *
	 * These functions return true to interrupt parsing, or false
	 * to continue.
	 */
	virtual bool rxPacket(const RawDataPkt &pkt);
	virtual bool rxPacket(const RTDLPkt &pkt);
	virtual bool rxPacket(const BankedEventPkt &pkt);
	virtual bool rxPacket(const BeamMonitorPkt &pkt);
	virtual bool rxPacket(const PixelMappingPkt &pkt);
	virtual bool rxPacket(const RunStatusPkt &pkt);
	virtual bool rxPacket(const RunInfoPkt &pkt);
	virtual bool rxPacket(const TransCompletePkt &pkt);
	virtual bool rxPacket(const ClientHelloPkt &pkt);
	virtual bool rxPacket(const StatsResetPkt &pkt);
	virtual bool rxPacket(const SyncPkt &pkt);
	virtual bool rxPacket(const HeartbeatPkt &pkt);
	virtual bool rxPacket(const DeviceDescriptorPkt &pkt);
	virtual bool rxPacket(const VariableU32Pkt &pkt);
	virtual bool rxPacket(const VariableDoublePkt &pkt);
	virtual bool rxPacket(const VariableStringPkt &pkt);

private:
	bool parseBuffer(void);

	uint8_t *	m_buffer;
	unsigned int	m_size;
	unsigned int	m_max_size;
	unsigned int	m_len;

	unsigned int	m_oversize_len;
	unsigned int	m_oversize_offset;
};

} /* namespacce ADARA */

#endif /* __ADARA_PARSER_H */
