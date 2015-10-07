#ifndef MANTID_LIVEDATA_FAKEISISHISTODAE_H_
#define MANTID_LIVEDATA_FAKEISISHISTODAE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Poco {
namespace Net {
class TCPServer;
}
}

namespace Mantid {
namespace LiveData {
/**
    Simulates ISIS histogram DAE. It runs continuously until canceled and
   listens to port 6789 for
    ISIS DAE commands.

    Data is generated starting at 10000 microseconds Time of flight, and each
   bin requested covers 100 microseconds.
    The algorithm silently defines three additional spectra with numbers
   NSpectra+1, NSpectra+2 and NSpectra+3 in a
    different time regime (they have different binning to the rest of the
   spectra).

    Copyright &copy; 2008-9 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

    File change history is stored at: <https://github.com/mantidproject/mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport FakeISISHistoDAE : public API::Algorithm {
public:
  FakeISISHistoDAE();
  virtual ~FakeISISHistoDAE();

  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "FakeISISHistoDAE"; };
  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 1; };
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const {
    return "DataHandling\\DataAcquisition";
  }
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Simulates ISIS histogram DAE.";
  }

private:
  // Implement abstract Algorithm methods
  void init();
  void exec();
  /// Poco TCP server
  Poco::Net::TCPServer *m_server;
  /// Mutex
  Kernel::Mutex m_mutex;
};

} // namespace LiveData
} // namespace Mantid

#endif /*MANTID_LIVEDATA_FAKEISISHISTODAE_H_*/
