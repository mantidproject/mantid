#ifndef MANTID_DATAHANDLING_SAVEDAVEGRP_H_
#define MANTID_DATAHANDLING_SAVEDAVEGRP_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace DataHandling {

/** SaveDaveGrp : @class SaveAscii SaveAscii.h DataHandling/SaveAscii.h

  Saves a workspace to a DAVE grp file. See
  http://www.ncnr.nist.gov/dave/documentation/ascii_help.pdf
  Properties:
  <ul>
        <li>Filename - the name of the file to write to.  </li>
        <li>Workspace - the workspace name to be saved.</li>
        <li>ToMicroeV - transform any energy axis into micro eV (optional) </li>
  </ul>

  @author Andrei Savici, ORNL
  @date 2011-07-22

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
class DLLExport SaveDaveGrp : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "SaveDaveGrp"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Saves a 2D workspace to DAVE grouped data format file.See "
           "http://www.ncnr.nist.gov/dave/documentation/ascii_help.pdf";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override {
    return {"LoadDaveGrp"};
  }
  /// Algorithm's category for identification
  const std::string category() const override {
    return "DataHandling\\Text;Inelastic\\DataHandling";
  }
  /// Algorithm's aliases
  const std::string alias() const override { return "SaveDASC"; }

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_SAVEDAVEGRP_H_ */
