// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//  BitStream.h
//  Helper to read bytewise and bitwise information from a binary file.
//  @author Joachim Coenen, Thomas Mueller Jülich Centre for Neutron Science
//  @date 2022-01-10
#ifndef MANTID_DATAHANDLING_BIT_STREAM_H_
#define MANTID_DATAHANDLING_BIT_STREAM_H_

#include <boost/endian/conversion.hpp>
#include <fstream>
#include <sys/stat.h>
#include <vector>

class FileByteStream {
public:
  explicit FileByteStream(const std::string &filename)
      : stream_(filename, std::ios_base::binary), fileSize_(static_cast<size_t>(getFileSize(filename))) {
    stream_.exceptions(std::ifstream::eofbit);
  }

private:
  std::ifstream stream_;
  const uint64_t fileSize_;

  long getFileSize(std::string filename) {
    struct stat stat_buf;
    int rc = stat(filename.c_str(), &stat_buf);
    return rc == 0 ? stat_buf.st_size : -1;
  }

  template <typename T> inline char *getResultPointer(T *const result, const std::size_t &bytecount) const {
    return reinterpret_cast<char *>(result) + sizeof(T) - std::min(sizeof(T), bytecount);
  }

public:
  template <typename T> inline FileByteStream &readRaw(T &result, const std::size_t &bytecount) {
    stream_.read(getResultPointer(&result, bytecount), bytecount);
    return *this;
  }

  template <std::size_t bytecount, typename T> inline FileByteStream &readRaw(T &result) {
    static_assert(sizeof(T) >= bytecount, "byte count of result needs to be greater or equal to bytecount");
    stream_.read(getResultPointer(&result, bytecount), bytecount);
    return *this;
  }

  template <typename T> inline FileByteStream &readRaw(T &result) { return readRaw<sizeof(T)>(result); }

  template <typename T> inline T readRaw(T &&result) {
    readRaw<sizeof(result)>(result);
    return result;
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
  explicit VectorByteStream(const std::vector<uint8_t> &vector) : pos(vector.begin()), end(vector.end()) {}

private:
  typename std::vector<uint8_t>::const_iterator pos;
  typename std::vector<uint8_t>::const_iterator end;

  template <typename T> inline char *getResultPointer(T *const result, const std::size_t &bytecount) const {
    return reinterpret_cast<char *>(result) + sizeof(T) - bytecount;
  }

  inline void streamread(char *dest, const std::size_t &bytecount) {
    std::copy(pos, pos + static_cast<long>(bytecount), dest);
    pos += static_cast<long>(bytecount);
  }

  inline void streamignore(const std::size_t &bytecount) { pos += static_cast<long>(bytecount); }

  inline unsigned streampeek() const { return *pos; }

  inline bool streameof() const { return pos >= end; }

public:
  template <typename T> inline VectorByteStream &readRaw(T &result, const std::size_t &bytecount) {

    streamread(getResultPointer(&result, bytecount), bytecount);
    return *this;
  }

  template <std::size_t bytecount, typename T> inline VectorByteStream &readRaw(T &result) {
    static_assert(sizeof(T) >= bytecount, "byte count of result needs to be greater or equal to bytecount");
    streamread(getResultPointer(&result, bytecount), bytecount);
    return *this;
  }

  template <typename T> inline VectorByteStream &readRaw(T &result) { return readRaw<sizeof(T)>(result); }

  template <typename T> inline T readRaw(T &&result) {
    readRaw<sizeof(result)>(result);
    return result;
  }

  template <std::size_t bytecount, typename T> inline VectorByteStream &read(T &result) {
    readRaw<bytecount>(result);
    result = boost::endian::conditional_reverse<boost::endian::order::big, boost::endian::order::native>(result);
    return *this;
  }

  inline uint8_t peek() { return static_cast<uint8_t>(streampeek()); }

  bool eof() const { return streameof(); }

  template <size_t bytecount> inline VectorByteStream &skip() {
    streamignore(bytecount);
    return *this;
  }

  template <typename T> VectorByteStream &operator>>(T &val) { return read(val); }
};

#endif /* MANTID_DATAHANDLING_BIT_STREAM_H_ */
