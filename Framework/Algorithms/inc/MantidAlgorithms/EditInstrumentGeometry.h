#ifndef MANTID_ALGORITHMS_EDITINSTRUMENTGEOMETRY_H_
#define MANTID_ALGORITHMS_EDITINSTRUMENTGEOMETRY_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Algorithms {

/** EditInstrumentGeometry : TODO: DESCRIPTION

  @author
  @date 2011-08-22

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
class DLLExport EditInstrumentGeometry : public API::Algorithm {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "The edit or added information will be attached to a Workspace.  "
           "Currently it is in an overwrite mode only.";
  }

  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override;
  /// Algorithm's version for identification overriding a virtual method
  int version() const override;
  /// Validate the inputs that must be parallel
  std::map<std::string, std::string> validateInputs() override;

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_EDITINSTRUMENTGEOMETRY_H_ */
