#ifndef MANTID_DATAHANDLING_FAKEISISHISTODAE_H_
#define MANTID_DATAHANDLING_FAKEISISHISTODAE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Poco
{
  namespace Net
  {
    class TCPServer;
  }
}

namespace Mantid
{
namespace DataHandling
{
/**
    Simulates ISIS histogram DAE. It runs continuously until canceled and listens to port 6789 for
    ISIS DAE commands.

    Copyright &copy; 2008-9 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
class DLLExport FakeISISHistoDAE : public API::Algorithm
{
public:
  FakeISISHistoDAE();
  virtual ~FakeISISHistoDAE();

  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "FakeISISHistoDAE";};
  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 1;};
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "DataHandling\\DataAcquisition";}

private:
  /// Sets documentation strings for this algorithm
  virtual void initDocs();
  // Implement abstract Algorithm methods
  void init();
  void exec();
  /// Poco TCP server
  Poco::Net::TCPServer* m_server;
  /// Mutex
  Kernel::Mutex m_mutex;
};

} // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_FAKEISISHISTODAE_H_*/
