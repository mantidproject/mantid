#ifndef MANTID_DATAHANDLING_LoadDNSEvent_H_
#define MANTID_DATAHANDLING_LoadDNSEvent_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/GroupingWorkspace.h"
#include "MantidTypes/Core/DateAndTime.h"


#include <array>
#include <fstream>

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid {
namespace DataHandling {

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

template <typename I> std::string n2hexstrX(I w, size_t hex_len = sizeof(I)<<1) {
  static const char* digits = "0123456789ABCDEF";
  std::string rc(hex_len,'0');
  for (size_t i=0, j=(hex_len-1)*4 ; i<hex_len; ++i,j-=4)
    rc[i] = digits[(w>>j) & 0x0f];
  return rc;
}

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
  return result + "  " + std::to_string(sizeof(w)) + " " + type_name(typeid(T).name());
}

template <typename T>
T convert_endianness(T value) {
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


class DLLExport LoadDNSEvent : public API::Algorithm {
public:
  ///
  const std::string name() const override {
    return "LoadDNSEvent";
  }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Load data from the new PSD detector to a Mantid EventWorkspace.";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override {
    return { };
  }
  /// Algorithm's category for identification
  const std::string category() const override {
    return "DataHandling";
  }

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;


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


  typedef uint64_t                 separator_t;
  typedef std::array<uint16_t, 21> buffer_header_t;
  typedef std::array<uint16_t, 3>  event_t;

  struct event_tx {
    enum class event_id_e {
      NEUTRON = 0,
      TRIGGER = 1
    };

    event_id_e id;
    uint8_t module_id; // 3 bit;
    uint8_t slot_id;  // 5 bit
    uint16_t amplitude; // 10 bit
    uint16_t position; // 10 bit
   Types::Core::DateAndTime timestamp;

  };

  typedef std::vector<event_t>     event_list_t;

  event_list_t parse_File(byte_stream &file);
  std::vector<uint8_t> parse_Header(byte_stream &file);
};

}

}
#endif /* MANTID_DATAHANDLING_LoadDNSEvent_H_ */
