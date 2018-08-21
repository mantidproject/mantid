#ifndef MANTID_DATAHANDLING_BIT_STREAM_H_
#define MANTID_DATAHANDLING_BIT_STREAM_H_

#include <array>
#include <vector>
#include <fstream>
#include <typeinfo>
#include <cxxabi.h>
#include <limits>
#include <functional>
#include <cstring>;

/**
  VectorByteStream.h
  Helper to read bytewise and bitwise information from a binary file.

  @author Joachim Coenen, Jülich Centre for Neutron Science
  @date 2011-11-17

  Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

template <typename T>
std::string n2hexstr(T w) {
  static const char* digits = "0123456789ABCDEF";
  std::string result(sizeof(w)*2, '0');

  uint8_t (*wtmp_p)[sizeof(w)] = reinterpret_cast<uint8_t (*)[sizeof(w)]>(&w);

  auto wtmp = *wtmp_p;
  for (size_t i = 0; i < sizeof(w); i++) {
    const auto j = i*2;
    result[j]   = digits[(wtmp[i] & 0xF0) >> 4 ];
    result[j+1] = digits[ wtmp[i] & 0x0F ];
  }
  return result;// + "  " + std::to_string(sizeof(w)) + " " + type_name(typeid(T).name());
}

template <typename T>
std::string n2binstr(T w) {
  static const char* digits = "01";
  std::string result(sizeof(w)*9, '|');

  uint8_t (*wtmp_p)[sizeof(w)] = reinterpret_cast<uint8_t (*)[sizeof(w)]>(&w);

  auto wtmp = *wtmp_p;
  for (size_t i = 0; i < sizeof(w); i++) {
    const auto j = i*9;
    result[j+0]   = digits[(wtmp[i] & 0b10000000) >> 7 ];
    result[j+1]   = digits[(wtmp[i] & 0b01000000) >> 6 ];
    result[j+2]   = digits[(wtmp[i] & 0b00100000) >> 5 ];
    result[j+3]   = digits[(wtmp[i] & 0b00010000) >> 4 ];
    result[j+4]   = digits[(wtmp[i] & 0b00001000) >> 3 ];
    result[j+5]   = digits[(wtmp[i] & 0b00000100) >> 2 ];
    result[j+6]   = digits[(wtmp[i] & 0b00000010) >> 1 ];
    result[j+7]   = digits[(wtmp[i] & 0b00000001) >> 0 ];
  }
  return result;
}

template <typename T>
inline T convert_endianness(T value) {
  const auto n = sizeof(value);
  uint8_t *data = reinterpret_cast<uint8_t*>(&value);
  for (size_t i = 0; i < n / 2; i++) {
    auto j = n - i - 1;
    // g_log.notice() << "[i] = 0x" << n2hexstr(data[i])<< ", [j] = 0x" << n2hexstr(data[j]) << "\n";
    data[i] = data[i] ^ data[j];
    // g_log.notice() << "[i] = 0x" << n2hexstr(data[i])<< ", [j] = 0x" << n2hexstr(data[j]) << "\n";
    data[j] = data[i] ^ data[j];
    // g_log.notice() << "[i] = 0x" << n2hexstr(data[i])<< ", [j] = 0x" << n2hexstr(data[j]) << "\n";
    data[i] = data[i] ^ data[j];
    // g_log.notice() << "[i] = 0x" << n2hexstr(data[i])<< ", [j] = 0x" << n2hexstr(data[j]) << "\n";
    // g_log.notice() << "--------^^  --------^^\n";
  }
  return value;
}

enum class endian {
    little = __ORDER_LITTLE_ENDIAN__,
    big    = __ORDER_BIG_ENDIAN__,
    native = __BYTE_ORDER__
};

namespace  {
static endian getMachineEndianess() {
  constexpr const uint32_t mixInt = 0x0000FFFFu;
  constexpr const std::array<uint8_t, 4> mixArray = { 0x00u, 0x00u, 0xFFu, 0xFFu };
  const endian machineEndianess = (*reinterpret_cast<const uint32_t *const>(&mixArray) == mixInt) ? endian::big: endian::little;
  return machineEndianess;
}
}

static const endian MACHINE_ENDIANESS = getMachineEndianess();

template<size_t bytecount, int bitsLeft = bytecount*8>
struct DataChunk {
  static_assert(bytecount*8 >= bitsLeft, "bitsLeft must be smaller or equal to bytecount * 8 ");
  static_assert(bitsLeft >= 0, "bitcount must be greater than zero");
//private:
  using Buffer = uint64_t;
  Buffer buffer;

public:
  template<std::size_t bitcount, typename T>
  inline DataChunk<bytecount, bitsLeft-bitcount> readBits (T &result) const {
    const size_t shiftAmount = sizeof(buffer)*8 - bitcount;
    const Buffer bufferMask = ~((Buffer(0x1) << shiftAmount) - Buffer(1));
    const Buffer maskedBuffer = buffer & bufferMask;
    Buffer shiftedBuffer = maskedBuffer >> shiftAmount;
    result = *reinterpret_cast<T *const>(&shiftedBuffer);
    return {buffer << bitcount};
  }

  template<std::size_t bitcount>
  inline DataChunk<bytecount, bitsLeft-bitcount> skipBits () const {
    return { buffer << bitcount };
  }
};

template<size_t bytecount>
struct DataChunk<bytecount, 0> {
  // empty struct to ensure no further bits are read from this DataChunk.
//private:
  using Buffer = uint64_t;
  Buffer buffer;
};

class FileByteStream {
public:
  explicit FileByteStream (const std::string &filename, const endian endianess)
    : stream(filename, std::ios_base::binary), endianess(endianess)
  {
    stream.exceptions(std::ifstream::eofbit);
  }

  const endian endianess;
private:
  std::ifstream stream;

  template<typename T>
  inline char *getResultPointer(T *const result, const std::size_t &bytecount) const {
    return reinterpret_cast<char*>(result) + sizeof(T)-bytecount;
  }

 public:
  template<typename T>
  inline FileByteStream& readRaw (T &result, const std::size_t &bytecount) {
    stream.read(getResultPointer(&result, bytecount), bytecount);
    return *this;
  }

  template<std::size_t bytecount, typename T>
  inline FileByteStream& readRaw (T& result) {
    static_assert(sizeof(T) >= bytecount, "byte count of result needs to be greater or equal to bytecount");
    stream.read(getResultPointer(&result, bytecount), bytecount);
    return *this;
  }

  template<typename T>
  inline FileByteStream& readRaw (T& result) {
    return readRaw<sizeof(T)>(result);
  }

  template<std::size_t bytecount, typename T>
  inline FileByteStream& read (T& result) {
    readRaw<bytecount>(result);
    if (endianess != MACHINE_ENDIANESS /*&& endianess != endian::native*/) {
      result = convert_endianness(result);
    }
    return *this;
  }

  template<typename T>
  inline FileByteStream& read (T& result) {
    return read<sizeof(result)>(result);
  }

  template<typename T>
  inline T read (T&& result) {
    read<sizeof(result)>(result);
    return result;
  }

  template<size_t bytecount>
  const DataChunk<bytecount> extractDataChunk() {
    DataChunk<bytecount> dc = {};
    read<bytecount>(dc.buffer);
    dc.buffer <<= (sizeof(dc.buffer) - bytecount)*8;
    return dc;
  }

  inline uint8_t peek () {
    return static_cast<uint8_t>(stream.peek());
  }

  bool eof() const { return stream.eof(); }

  template<size_t bytecount>
  inline FileByteStream& skip () {
    stream.ignore(bytecount);
    return *this;
  }

  template<typename T>
  FileByteStream& operator>>(T &val) {
    return read(val);
  }

};

#endif /* MANTID_DATAHANDLING_BIT_STREAM_H_ */
