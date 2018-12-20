#ifndef MANTID_DATAHANDLING_SAVEASCII_H_
#define MANTID_DATAHANDLING_SAVEASCII_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace DataHandling {
/** @class SaveAscii SaveAscii.h DataHandling/SaveAscii.h

Saves a workspace or selected spectra in a coma-separated ascii file. Spectra
are saved in columns.

@author Roman Tolchenov, Tessella plc
@date 3/07/09

Copyright &copy; 2007-9 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport SaveAscii : public API::Algorithm {
public:
  /// Default constructor
  SaveAscii();
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "SaveAscii"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Saves a 2D workspace to a ascii file.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "DataHandling\\Text"; }

private:
  /// Overwrites Algorithm method.
  void init() override;
  /// Overwrites Algorithm method
  void exec() override;

  /// Map the separator options to their string equivalents
  std::map<std::string, std::string> m_separatorIndex;
};

} // namespace DataHandling
} // namespace Mantid

#endif /*  MANTID_DATAHANDLING_SAVEASCII_H_  */
