#ifndef MANTID_DATAHANDLING_SaveReflCustomAscii_H_
#define MANTID_DATAHANDLING_SaveReflCustomAscii_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataHandling/AsciiPointBase.h"

namespace Mantid {
namespace DataHandling {
/**
Saves a file in ILL Cosmos format  from a 2D workspace
(Workspace2D class). SaveILLCosmosAscii is an algorithm but inherits frrm the
AsciiPointBase class which provides the main implementation for the init() &
exec() methods.
Output is tab delimited Ascii point data with dq/q and extra header information.

Copyright &copy; 2007-14 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge
National Laboratory

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
class DLLExport SaveReflCustomAscii : public DataHandling::AsciiPointBase {
public:
  /// Default constructor
  SaveReflCustomAscii() {}
  /// Destructor
  ~SaveReflCustomAscii() {}
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "SaveReflCustomAscii"; }
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Saves a 2D workspace to a ascii file.";
  }
  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 1; }

  ///
  void data(std::ofstream &file, const std::vector<double> &XData,
            bool exportDeltaQ);

private:
  /// Return the file extension this algorthm should output.
  virtual std::string ext() { return ".dat"; }
  /// extra properties specifically for this
  virtual void extraProps();
  /// write any extra information required
  virtual void extraHeaders(std::ofstream &file);
};
} // namespace DataHandling
} // namespace Mantid

#endif /*  MANTID_DATAHANDLING_SaveReflCustomAscii_H_  */
