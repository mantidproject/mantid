#ifndef MANTID_DATAHANDLING_BIT_STREAM_H_
#define MANTID_DATAHANDLING_BIT_STREAM_H_

#include <array>
#include <vector>
#include <fstream>
#include <typeinfo>
#include <cxxabi.h>
#include <limits>
#include <functional>

/**
  LoadDNSEvent

  Algorithm used to generate a GroupingWorkspace from an .xml or .map file
  containing the
  detectors' grouping information.

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
std::string type_name(std::string tname)
{
    int status;
    char *demangled_name = abi::__cxa_demangle(tname.c_str(), NULL, NULL, &status);
    if(status == 0) {
        tname = demangled_name;
        std::free(demangled_name);
    }
    return tname;
}

template <typename T>
std::string n2hexstr(T w) {
  static const char* digits = "0123456789ABCDEF";
  std::string result(sizeof(w)*2, '0');

  uint8_t (*wtmp_p)[sizeof(w)] = (uint8_t (*)[sizeof(w)]) (&w);

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

  uint8_t (*wtmp_p)[sizeof(w)] = (uint8_t (*)[sizeof(w)]) (&w);

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
  return result + "  " + std::to_string(sizeof(w)) + " " + type_name(typeid(T).name());
}

template <typename T, typename S>
inline uint16_t rotll (T v, S b) {
  // adapted from here: https://gist.github.com/pabigot/7550454
  static_assert(std::is_integral<T>::value, "rotate of non-integral type");
  //static_assert(! std::is_signed<T>::value, "rotate of signed type");
  typedef typename std::make_unsigned<T>::type UnsignedT;
  constexpr auto num_bits = std::numeric_limits<UnsignedT>::digits;
  static_assert(0 == (num_bits & (num_bits - 1)), "rotate value bit length not power of two");
  constexpr auto count_mask = num_bits - 1;
  const auto mb = b & count_mask;
  const auto unsignedV = UnsignedT(v);
  const auto result = UnsignedT(UnsignedT(unsignedV + 0U) << mb) | UnsignedT(UnsignedT(unsignedV) >> (-mb & count_mask));
  return *(const T*)&result;
}

template <typename T, typename S>
inline uint16_t rotrr (T v, S b) {
  // adapted from here: https://gist.github.com/pabigot/7550454
  static_assert(std::is_integral<T>::value, "rotate of non-integral type");
  //static_assert(! std::is_signed<T>::value, "rotate of signed type");
  typedef typename std::make_unsigned<T>::type UnsignedT;
  constexpr auto num_bits = std::numeric_limits<UnsignedT>::digits;
  static_assert(0 == (num_bits & (num_bits - 1)), "rotate value bit length not power of two");
  constexpr auto count_mask = num_bits - 1;
  const auto mb = b & count_mask;
  const auto unsignedV = UnsignedT(v);
  const auto result = UnsignedT(UnsignedT(unsignedV + 0U) >> mb) | UnsignedT(UnsignedT(unsignedV) << (-mb & count_mask));
  return *(const T*)&result;
}

template <typename T>
/**
 * @brief a shift function, that ignares endianess.
 * @param w
 * @param n
 * @return the result
 */
T shiftMemR(T w, int n) {
  uint8_t (*wtmp_p)[sizeof(w)] = (uint8_t (*)[sizeof(w)]) (&w);
  auto wtmp = *wtmp_p;

  T r{0};
  uint8_t (*rtmp_p)[sizeof(r)] = (uint8_t (*)[sizeof(r)]) (&r);
  auto rtmp = *rtmp_p;

  for (size_t i = std::max(0, -n); i < sizeof(w)*8-std::max(0, n); i++) {
    const uint8_t val   =(wtmp[i >> 3] & (uint8_t(0b10000000u) >> (i % 8))) >> (7 - i % 8) ;
    const auto j = (i+n);
    rtmp[j >> 3] |= (val << (7 - j % 8));
  }

  return r;
}

template <typename T>
inline T repair_endianness(T value, size_t bitCount) {
  //return value;
  const auto shift = (8 - bitCount % 8) % 8;
  const auto m = bitCount / 8;
  uint8_t *data8 = (uint8_t*) &value;
  for (auto i = m-1; i <= 0; i--) {
    uint16_t *window16_ptr = (uint16_t*) &data8[i];
    uint16_t window16 = *window16_ptr;
    const uint16_t mask = shiftMemR(uint16_t(0xFFFFu), -(8u-shift));
    const uint16_t l = (shiftMemR(window16, shift) & mask);
    const uint16_t r = (window16 & uint16_t(~mask));
    *window16_ptr = l | r;
  }

  return value;
}
template <typename T>
inline T convert_endianness(T value, size_t bitCount) {
  //return value;
  repair_endianness(value, bitCount);

  const auto n = bitCount / 8 + 1;
  uint8_t *data = (uint8_t*) &value;
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

template <typename T>
inline T convert_endianness(T value) {
  const auto n = sizeof(value);
  uint8_t *data = (uint8_t*) &value;
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

template<typename T>
inline std::ostream &_innerLogTuple(std::ostream &out, const T &value) {
  out << value << ")";
  return out;
}

inline std::ostream &_innerLogTuple(std::ostream &out, const signed char &value) {
  return _innerLogTuple(out, std::to_string(value));
}

inline std::ostream &_innerLogTuple(std::ostream &out, const unsigned char &value) {
  return _innerLogTuple(out, std::to_string(value));
}

template<typename T, typename... Args>
inline std::ostream &_innerLogTuple(std::ostream &out, const T &value, const Args&... args);

template<typename... Args>
inline std::ostream &_innerLogTuple(std::ostream &out, const signed char &value, const Args&... args) {
  return _innerLogTuple(out, std::to_string(value), args...);
}

template<typename... Args>
inline std::ostream &_innerLogTuple(std::ostream &out, const unsigned char &value, const Args&... args) {
  return _innerLogTuple(out, std::to_string(value), args...);
}

template<typename T, typename... Args>
inline std::ostream &_innerLogTuple(std::ostream &out, const T &value, const Args&... args) {
  out << value << ", ";
  return _innerLogTuple(out, args...);
}

template<typename... Args>
inline std::ostream &logTuple(std::ostream &out, Args... args) {
  out << "(";
  return _innerLogTuple(out, args...);
}

enum class endian {
    little = __ORDER_LITTLE_ENDIAN__,
    big    = __ORDER_BIG_ENDIAN__,
    native = __BYTE_ORDER__
};

static constexpr uint32_t mixInt = 0x0000FFFFu;
static constexpr std::array<uint8_t, 4> mix = { 0x00u, 0x00u, 0xFFu, 0xFFu };
    // *(constexpr std::array<uint8_t, 4>*)&mixInt;
constexpr endian getEndianess() {
    return (((*(const uint32_t*)&mix) & 0x1u) == 1u) ? endian::big: endian::little;
}

class BitStream {
public:
  explicit BitStream (const std::string &filename, const endian endianess, std::function<std::ostream&(void)> out)
    : stream(filename, std::ios_base::binary), endianess(endianess), out(out), buffer(1024*1024)
  {
    stream.exceptions(std::ifstream::eofbit);
  }

  const endian endianess;
private:
  template<std::size_t n>
  using Buffer = std::array<uint8_t, n>;

  std::function<std::ostream&(void)> out;
  std::ifstream stream;
  //uint_least32_t buffer;
  std::vector<uint8_t> buffer;
  static constexpr auto bufferSize = /*buffer.size()*/1024*1024 * 8;

  std::size_t posInBuffer = bufferSize;

  inline bool isEndOfBuffer(std::size_t pos) { return pos >= bufferSize; }
  inline bool isEndOfBuffer() { return isEndOfBuffer(posInBuffer); }


  template<typename T, typename U>
  inline void copyBits(const T &from, U &to, const std::size_t &bitCount,
                const std::size_t &fromStart, const std::size_t &toStart) {

    std::size_t fromPos = fromStart;
    std::size_t toPos = toStart;
    long int bitCountLeft = bitCount;

    uint8_t leftOver = to[toPos / 8] & uint8_t(0xFFu << (8 - toPos % 8u));
    int16_t leftOverCount = 0;
    while (bitCountLeft > 0) {
      const uint8_t fromPosMod8 = fromPos % 8u;
      const uint8_t toPosMod8 = toPos % 8u;
      const uint8_t chunkSize = uint8_t(std::min<std::size_t>(8-fromPosMod8, bitCountLeft));

      const uint8_t buffMaskR = uint8_t(0xFFu << (8 - (fromPosMod8 + chunkSize)));
      const uint8_t buffMaskL = uint8_t(0xFFu >> fromPosMod8);
      const uint8_t buffMask = buffMaskR & buffMaskL;

      const uint8_t buff = from[fromPos / 8] & buffMask;

      const uint8_t buffLAligned = buff << fromPosMod8;
      const uint8_t buffShift = buffLAligned >> toPosMod8;
      leftOverCount = std::max<int16_t>(0, (toPosMod8 + chunkSize) - 8);
      const uint16_t shiftLeftOver = 8 - leftOverCount;



      to[toPos / 8] = leftOver | buffShift;
      leftOver = buff << shiftLeftOver;

      fromPos += chunkSize;
      toPos += chunkSize;
      bitCountLeft -= chunkSize;
    }

    if (leftOverCount > 0) {
      to[toPos / 8] = leftOver;
    }

    /*
    while (bitCountLeft > 0) {
      const uint8_t fromPosMod8 = fromPos % 8u;
      const uint8_t toPosMod8 = toPos % 8u;
      const uint8_t chunkSize = uint8_t(std::min<std::size_t>(std::min(8-fromPosMod8, 8-toPosMod8), bitCountLeft));

      const uint8_t buffMaskR = uint8_t(0xFFu << (8 - (fromPosMod8 + chunkSize)));
      const uint8_t buffMaskL = uint8_t(0xFFu >> fromPosMod8);
      const uint8_t buffMask = buffMaskR & buffMaskL;

      const uint8_t buff = from[fromPos / 8] & buffMask;

      const int16_t shiftAmount = toPosMod8 - fromPosMod8;
      const uint8_t buffShift = (shiftAmount > 0) ? uint8_t(buff     >> shiftAmount) : uint8_t(buff     << -shiftAmount);
      //const uint8_t maskShift = (shiftAmount > 0) ? uint8_t(buffMask >> shiftAmount) : uint8_t(buffMask << -shiftAmount);

      const uint8_t destMaskR = uint8_t(0xFFu >> (toPosMod8 + chunkSize));
      const uint8_t destMaskL = uint8_t(0xFFu << (8 - toPosMod8));
      const uint8_t destMask = destMaskR | destMaskL;

      //out() << n2binstr(buffMask) << " : " << std::to_string(fromPosMod8) << std::endl;
      //out() << n2binstr(maskShift) << " : " << std::to_string(toPosMod8) << std::endl;
      //out() << n2binstr(destMask) << " : ----------------" << std::endl;

      const uint8_t dest = to[toPos / 8] & destMask;

      to[toPos / 8] = dest | buffShift;

      fromPos += chunkSize;
      toPos += chunkSize;
      bitCountLeft -= chunkSize;

    }
    */

    if (bitCountLeft < 0) {
      throw std::runtime_error("bitCountLeft was < 0 (in copyBits)!!");
    }
  }

 public:
  template<typename T>
  inline BitStream& readRaw (T &result, const std::size_t &bitcount) {
    // array view of result
    std::array<uint8_t, sizeof(T)> *array = (std::array<uint8_t, sizeof(T)>*)&result;

    std::size_t posInArray = sizeof(result) * 8 - bitcount;
    long int bitCountLeft = bitcount;

    while (bitCountLeft > 0) {
      if (isEndOfBuffer()) {
        try {
          stream.read((char*)buffer.data(), buffer.size());
//          if (bitcount == 64) {
//            out() << "readSucc " << n2hexstr(buffer) << ",  " << stream.gcount()*8 << std::endl;
//          }
          posInBuffer = 0;
        } catch (std::ifstream::failure e) {
//          if (bitcount == 64) {
//            out() << "readFail " << n2hexstr(buffer) << ",  " << stream.gcount()*8 << std::endl;
//          }
          if (stream.gcount()*8 >= bitCountLeft) {
            auto bufTemp = buffer;
            posInBuffer = bufferSize - stream.gcount()*8;
            copyBits(bufTemp, buffer, stream.gcount()*8, 0, posInBuffer);
          } else {
            throw e;
          }
        }
      }

      const auto chunkSize = std::min<std::size_t>(bufferSize - posInBuffer, bitCountLeft);
//      out() << "chunkSize: " << chunkSize << "\n";
      copyBits(buffer, *array, chunkSize, posInBuffer, posInArray);

//      if (bitcount == 64) {
//        out() << n2hexstr(result) << " <-- " << n2hexstr(buffer) << ",  " << posInBuffer << ",  " << chunkSize << std::endl;
//      }
      posInBuffer += chunkSize;
      posInArray += chunkSize;
      bitCountLeft -= chunkSize;

      /*
      const auto chunkSize = std::min<std::size_t>(bufferSize - posInBuffer, bitCountLeft);

      copyBits(buffer, array, chunkSize, posInBuffer, posInArray);
      posInBuffer += chunkSize;
      posInArray += chunkSize;
      bitCountLeft -= chunkSize;
      */
    }

    if (bitCountLeft < 0) {
      throw std::runtime_error("bitCountLeft was < 0 (in read)!!");
    }

    return *this;
  }

  template<std::size_t bitcount, typename T>
  inline BitStream& readRaw (T& result) {
    static_assert(sizeof(T)*8 >= bitcount, "bit count of result needs to be greater or equal to bitcount");
    // array view of result
    std::array<uint8_t, sizeof(T)> *array = (std::array<uint8_t, sizeof(T)>*)&result;

    std::size_t posInArray = sizeof(result) * 8 - bitcount;
    long int bitCountLeft = bitcount;

    while (bitCountLeft > 0) {
      if (isEndOfBuffer()) {
        try {
          stream.read((char*)buffer.data(), buffer.size());
          posInBuffer = 0;
        } catch (std::ifstream::failure e) {
          if (stream.gcount()*8 >= bitCountLeft) {
            auto bufTemp = buffer;
            posInBuffer = bufferSize - stream.gcount()*8;
            copyBits(bufTemp, buffer, stream.gcount()*8, 0, posInBuffer);
          } else {
            throw e;
          }
        }
      }

      const auto chunkSize = std::min<std::size_t>(bufferSize - posInBuffer, bitCountLeft);
      copyBits(buffer, *array, chunkSize, posInBuffer, posInArray);

      posInBuffer += chunkSize;
      posInArray += chunkSize;
      bitCountLeft -= chunkSize;

    }

    if (bitCountLeft < 0) {
      throw std::runtime_error("bitCountLeft was < 0 (in read)!!");
    }

    return *this;
  }

  template<typename T>
  inline BitStream& readRaw (T& result) {
    return readRaw<sizeof(T)*8>(result);
  }

  template<std::size_t bitcount, typename T>
  inline BitStream& read (T& result) {
    readRaw<bitcount>(result);
    if (endianess != getEndianess() /*&& endianess != endian::native*/) {
      result = convert_endianness(result);
    }
    return *this;
  }

  template<typename T>
  inline BitStream& read (T& result) {
    return read<sizeof(T)*8>(result);
  }

  template<typename T>
  inline T read (T&& result) {
    read<sizeof(T)*8>(result);
    return result;
  }

  inline uint8_t peek () {
    std::size_t byteIndex = posInBuffer/8;
    uint8_t posInBufferMod8 = posInBuffer % 8;
    uint8_t byte1 = isEndOfBuffer() ? static_cast<uint8_t>(stream.peek()) : buffer[byteIndex];
    uint8_t result = byte1 << posInBufferMod8;
    uint8_t byte2 = isEndOfBuffer(posInBuffer+8) ? static_cast<uint8_t>(stream.peek()) : buffer[byteIndex+1];
    result |= byte2 >> (8 - posInBufferMod8);

    return result;
  }

  bool eof() const { return stream.eof() && (posInBuffer >= bufferSize); }

  template<std::size_t bitcount>
  inline BitStream& skip () {
    const constexpr auto n = std::min(8ul, bitcount);
    std::array<uint8_t, n/8 + ((n%8 > 0)? 1 : 0)> dummy;
    readRaw<n>(dummy);
    return skip<bitcount - n>();
//    //const std::size_t skipJumpableBytes = (bitcount - (8 - posInBuffer)) / 8;
//    const constexpr std::size_t skipJumpableBytes = std::max<int64_t>(0, int64_t(bitcount / 8) - buffer.size());
//    stream.ignore(skipJumpableBytes);
//    std::array<uint8_t, (bitcount - skipJumpableBytes*8)/8+1> dummy;
//    return readRaw(dummy, bitcount - skipJumpableBytes*8);
  }

  template<typename T>
  BitStream& operator>>(T &val) {
    return read(val);
  }

};

template<>
inline BitStream& BitStream::skip<0> () {
  return *this;
}

class byte_stream : std::ifstream {
public:
  uint64_t readCount = 0;
  explicit byte_stream (const char* filename, std::ios_base::openmode mode = std::ios_base::in)
    : std::ifstream(filename, mode) {}

  explicit byte_stream (const std::string& filename, std::ios_base::openmode mode = std::ios_base::in)
    : std::ifstream(filename, mode) {}

  template<typename T>
  byte_stream& read (T& s, std::streamsize n) {
    std::ifstream::read((char*)&s, n);
    readCount += n;
    return *this;
  }

  template<typename T>
  byte_stream& read (T& s) {
    return read(s, sizeof(s));
  }

  template<typename T>
  T read (T&& s) {
    T t = s;
    read(t);
    return t;
  }

  template<typename T>
  T &&read () {
    return read(T());
  }

  template<typename T>
  std::streamsize readsome (char* s, std::streamsize n) {
    std::streamsize m = std::ifstream::readsome((char*)s, n);
    readCount += m;
    return m;
  }

  template<typename T>
  std::streamsize ignore (std::streamsize n = 1, int delim = EOF) {
    std::streamsize m = std::ifstream::ignore(n, delim);
    readCount += m;
    return m;
  }

  int_type peek() {
    return std::ifstream::peek();
  }

  bool eof() const { return std::ifstream::eof(); }

  std::ios_base::iostate exceptions() const {
    return std::ifstream::exceptions();
  }

  void exceptions (std::ios_base::iostate except) {
    std::ifstream::exceptions(except);
  }

};


#endif /* MANTID_DATAHANDLING_BIT_STREAM_H_ */
