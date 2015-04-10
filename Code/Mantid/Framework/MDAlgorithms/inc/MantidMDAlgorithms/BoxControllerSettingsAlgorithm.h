#ifndef MANTID_MDALGORITHMS_BOXCONTROLLERSETTINGSALGORITHM_H_
#define MANTID_MDALGORITHMS_BOXCONTROLLERSETTINGSALGORITHM_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/BoxController.h"
#include "MantidGeometry/Instrument.h"

namespace Mantid {
namespace MDAlgorithms {

/** An abstract algorithm sub-class for algorithms that
 * define properties for BoxController settings.
 *
 * This will be inherited by other algorithms as required.

  @author Janik Zikovsky
  @date 2011-11-02

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
class DLLExport BoxControllerSettingsAlgorithm : public API::Algorithm {
public:
  BoxControllerSettingsAlgorithm();
  ~BoxControllerSettingsAlgorithm();

protected:
  /// Initialise the properties
  void initBoxControllerProps(const std::string &SplitInto = "5",
                              int SplitThreshold = 1000,
                              int MaxRecursionDepth = 5);

  /// Set the settings in the given box controller
  void setBoxController(Mantid::API::BoxController_sptr bc,
                        Mantid::Geometry::Instrument_const_sptr instrument);

  /// Set the settings in the given box controller
  void setBoxController(Mantid::API::BoxController_sptr bc);

  std::string getBoxSettingsGroupName() { return "Box Splitting Settings"; }
  /// Take the defaults for the box splitting from the instrument parameters.
  void
  takeDefaultsFromInstrument(Mantid::Geometry::Instrument_const_sptr instrument,
                             const size_t ndims);

private:
};

} // namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_BOXCONTROLLERSETTINGSALGORITHM_H_ */
