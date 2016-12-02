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
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "SaveANSTOAscii"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Saves a 2D workspace to a ascii file.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }

private:
  /// Return the file extension this algorthm should output.
  std::string ext() override { return ".txt"; }
  /// return if the line should start with a separator
  bool leadingSep() override { return false; }
  /// only separator property required, nothing else
  void extraProps() override { appendSeparatorProperty(); }
  /// no extra information required so override blank
  void extraHeaders(std::ofstream &file) override;
};

} // namespace DataHandling
} // namespace Mantid

#endif /*  MANTID_DATAHANDLING_SAVEANSTO_H_  */
