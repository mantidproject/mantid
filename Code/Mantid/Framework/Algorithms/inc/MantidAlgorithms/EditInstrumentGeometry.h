#ifndef MANTID_ALGORITHMS_EDITINSTRUMENTGEOMETRY_H_
#define MANTID_ALGORITHMS_EDITINSTRUMENTGEOMETRY_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"

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
  EditInstrumentGeometry();
  ~EditInstrumentGeometry();
  virtual const std::string name() const;
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "The edit or added information will be attached to a Workspace.  "
           "Currently it is in an overwrite mode only.";
  }

  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const;
  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const;
  /// Validate the inputs that must be parallel
  virtual std::map<std::string, std::string> validateInputs();

private:
  /// Initialise the properties
  void init();
  /// Run the algorithm
  void exec();
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_EDITINSTRUMENTGEOMETRY_H_ */
