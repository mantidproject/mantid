#ifndef MANTID_DATAHANDLING_SAVEANSTOASCII_H_
#define MANTID_DATAHANDLING_SAVEANSTOASCII_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidDataHandling/AsciiPointBase.h"

namespace Mantid {
namespace DataHandling {
/**
Saves a file in Ansto format and from a 2D workspace
(Workspace2D class). SaveANSTOAscii is an algorithm but inherits from the
AsciiPointBase class which provides the main implementation for the init() &
exec() methods.
Output is tab delimited Ascii point data with dq/q.

Copyright &copy; 2007-14 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport SaveANSTOAscii : public DataHandling::AsciiPointBase {
public:
  /// Default constructor
  SaveANSTOAscii() {}
  /// Destructor
  ~SaveANSTOAscii() {}
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "SaveANSTOAscii"; }
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Saves a 2D workspace to a ascii file.";
  }

  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 1; }

private:
  /// Return the file extension this algorthm should output.
  virtual std::string ext() { return ".txt"; }
  /// return if the line should start with a separator
  virtual bool leadingSep() { return false; }
  /// no extra properties required so override blank
  virtual void extraProps() {}
  /// no extra information required so override blank
  virtual void extraHeaders(std::ofstream &file);
};

} // namespace DataHandling
} // namespace Mantid

#endif /*  MANTID_DATAHANDLING_SAVEANSTO_H_  */
