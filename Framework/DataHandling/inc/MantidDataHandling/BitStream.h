#ifndef MANTID_DATAHANDLING_BIT_STREAM_H_
#define MANTID_DATAHANDLING_BIT_STREAM_H_

#include <array>
#include <fstream>
#include <sys/stat.h>
#include <vector>

/**
  BitStream.h
  Helper to read bytewise and bitwise information from a binary file.

  @author Joachim Coenen, Jülich Centre for Neutron Science
  @date 2018-08-17

  Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

template <typename T> inline T convert_endianness(T value) {
  const auto n = sizeof(value);
  uint8_t *data = reinterpret_cast<uint8_t *>(&value);
  for (size_t i = 0; i < n / 2; i++) {
    auto j = n - i - 1;
    data[i] = data[i] ^ data[j];
    data[j] = data[i] ^ data[j];
    data[i] = data[i] ^ data[j];
  }
  return value;
}

enum class endian {
#ifdef _WIN32
  little = 0,
  big = 1,
  native = little
#else
  little = __ORDER_LITTLE_ENDIAN__,
  big = __ORDER_BIG_ENDIAN__,
  native = __BYTE_ORDER__
#endif
};

namespace {
static endian getMachineEndianess() {
  constexpr const uint32_t mixInt = 0x0000FFFFu;
  constexpr const std::array<uint8_t, 4> mixArray = {0x00u, 0x00u, 0xFFu,
                                                     0xFFu};
  const endian machineEndianess =
      (*reinterpret_cast<const uint32_t *const>(&mixArray) == mixInt)
          ? endian::big
          : endian::little;
  return machineEndianess;
}
} // namespace

static const endian MACHINE_ENDIANESS = getMachineEndianess();

template <size_t bytecount, int bitsLeft = bytecount * 8> struct DataChunk {
  static_assert(bytecount * 8 >= bitsLeft,
                "bitsLeft must be smaller or equal to bytecount * 8 ");
  static_assert(bitsLeft >= 0, "bitcount must be greater than zero");
  // private:
  using Buffer = uint64_t;
  Buffer buffer;

public:
  template <std::size_t bitcount, typename T>
  inline DataChunk<bytecount, bitsLeft - bitcount> readBits(T &result) const {
    const size_t shiftAmount = sizeof(buffer) * 8 - bitcount;
    const Buffer bufferMask = ~((Buffer(0x1) << shiftAmount) - Buffer(1));
    const Buffer maskedBuffer = buffer & bufferMask;
    Buffer shiftedBuffer = maskedBuffer >> shiftAmount;
    result = *reinterpret_cast<T *const>(&shiftedBuffer);
    return {buffer << bitcount};
  }

  template <std::size_t bitcount>
  inline DataChunk<bytecount, bitsLeft - bitcount> skipBits() const {
    return {buffer << bitcount};
  }
};

template <size_t bytecount> struct DataChunk<bytecount, 0> {
  // empty struct to ensure no further bits are read from this DataChunk.
  // private:
  using Buffer = uint64_t;
  Buffer buffer;
};

class FileByteStream {
public:
  explicit FileByteStream(const std::string &filename, const endian endianess)
      : stream_(filename, std::ios_base::binary), endianess(endianess),
        fileSize_(getFileSize(filename)) {
    stream_.exceptions(std::ifstream::eofbit);
  }

  const endian endianess;

private:
  std::ifstream stream_;
  const uint64_t fileSize_;

  long getFileSize(std::string filename) {
    struct stat stat_buf;
    int rc = stat(filename.c_str(), &stat_buf);
    return rc == 0 ? stat_buf.st_size : -1;
  }

  template <typename T>
  inline char *getResultPointer(T *const result,
                                const std::size_t &bytecount) const {
    return reinterpret_cast<char *>(result) + sizeof(T) -
           std::min(sizeof(T), bytecount);
  }

public:
  template <typename T>
  inline FileByteStream &readRaw(T &result, const std::size_t &bytecount) {
    stream_.read(getResultPointer(&result, bytecount), bytecount);
    return *this;
  }

  template <std::size_t bytecount, typename T>
  inline FileByteStream &readRaw(T &result) {
    static_assert(
        sizeof(T) >= bytecount,
        "byte count of result needs to be greater or equal to bytecount");
    stream_.read(getResultPointer(&result, bytecount), bytecount);
    return *this;
  }

  template <typename T> inline FileByteStream &readRaw(T &result) {
    return readRaw<sizeof(T)>(result);
  }

  template <typename T> inline T readRaw(T &&result) {
    readRaw<sizeof(result)>(result);
    return result;
  }

  template <std::size_t bytecount, typename T>
  inline FileByteStream &read(T &result) {
    readRaw<bytecount>(result);
    if (endianess != MACHINE_ENDIANESS /*&& endianess != endian::native*/) {
      result = convert_endianness(result);
    }
    return *this;
  }

  template <typename T> inline FileByteStream &read(T &result) {
    return read<sizeof(result)>(result);
  }

  template <typename T> inline T read(T &&result) {
    read<sizeof(result)>(result);
    return result;
  }

  template <size_t bytecount> const DataChunk<bytecount> extractDataChunk() {
    DataChunk<bytecount> dc = {};
    read<bytecount>(dc.buffer);
    dc.buffer <<= (sizeof(dc.buffer) - bytecount) * 8;
    return dc;
  }

  inline uint8_t peek() { return static_cast<uint8_t>(stream_.peek()); }

  bool eof() const { return stream_.eof(); }

  template <size_t bytecount> inline FileByteStream &skip() {
    stream_.ignore(bytecount);
    return *this;
  }

  std::streamsize gcount() const { return stream_.gcount(); }
  template <typename T> FileByteStream &operator>>(T &val) { return read(val); }

  uint64_t fileSize() const { return fileSize_; }
};

class VectorByteStream {
public:
  explicit VectorByteStream(const std::vector<uint8_t> &vector,
                            const endian endianess)
      : endianess(endianess), pos(vector.begin()), end(vector.end()) {}

  const endian endianess;

private:
  typename std::vector<uint8_t>::const_iterator pos;
  typename std::vector<uint8_t>::const_iterator end;

  template <typename T>
  inline char *getResultPointer(T *const result,
                                const std::size_t &bytecount) const {
    return reinterpret_cast<char *>(result) + sizeof(T) - bytecount;
  }

  inline void streamread(char *dest, const std::size_t &bytecount) {
    std::copy(pos, pos + bytecount, dest);
    pos += bytecount;
  }

  inline void streamignore(const std::size_t &bytecount) { pos += bytecount; }

  inline uint streampeek() const { return *pos; }

  inline bool streameof() const { return pos >= end; }

public:
  template <typename T>
  inline VectorByteStream &readRaw(T &result, const std::size_t &bytecount) {

    streamread(getResultPointer(&result, bytecount), bytecount);
    return *this;
  }

  template <std::size_t bytecount, typename T>
  inline VectorByteStream &readRaw(T &result) {
    static_assert(
        sizeof(T) >= bytecount,
        "byte count of result needs to be greater or equal to bytecount");
    streamread(getResultPointer(&result, bytecount), bytecount);
    return *this;
  }

  template <typename T> inline VectorByteStream &readRaw(T &result) {
    return readRaw<sizeof(T)>(result);
  }

  template <typename T> inline T readRaw(T &&result) {
    readRaw<sizeof(result)>(result);
    return result;
  }

  template <std::size_t bytecount, typename T>
  inline VectorByteStream &read(T &result) {
    readRaw<bytecount>(result);
    if (endianess != MACHINE_ENDIANESS /*&& endianess != endian::native*/) {
      result = convert_endianness(result);
    }
    return *this;
  }

  template <typename T> inline VectorByteStream &read(T &result) {
    return read<sizeof(result)>(result);
  }

  template <typename T> inline T read(T &&result) {
    read<sizeof(result)>(result);
    return result;
  }

  template <size_t bytecount> const DataChunk<bytecount> extractDataChunk() {
    DataChunk<bytecount> dc = {};
    read<bytecount>(dc.buffer);
    dc.buffer <<= (sizeof(dc.buffer) - bytecount) * 8;
    return dc;
  }

  inline uint8_t peek() { return static_cast<uint8_t>(streampeek()); }

  bool eof() const { return streameof(); }

  template <size_t bytecount> inline VectorByteStream &skip() {
    streamignore(bytecount);
    return *this;
  }

  template <typename T> VectorByteStream &operator>>(T &val) {
    return read(val);
  }
};

#endif /* MANTID_DATAHANDLING_BIT_STREAM_H_ */
