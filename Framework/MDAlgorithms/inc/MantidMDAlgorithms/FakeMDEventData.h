#ifndef MANTID_MDALGORITHMS_FAKEMDEVENTDATA_H_
#define MANTID_MDALGORITHMS_FAKEMDEVENTDATA_H_

#include "MantidAPI/Algorithm.h"
#include "MantidMDAlgorithms/DllConfig.h"

namespace Mantid {
namespace MDAlgorithms {

/** FakeMDEventData : Algorithm to create fake multi-dimensional event
  data that gets added to MDEventWorkspace, for use in testing.

  Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
 *
 */
class MANTID_MDALGORITHMS_DLL FakeMDEventData : public API::Algorithm {
public:
  /// Algorithm's name for identification
  virtual const std::string name() const { return "FakeMDEventData"; }
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Adds fake multi-dimensional event data to an existing "
           "MDEventWorkspace, for use in testing.\nYou can create a blank "
           "MDEventWorkspace with CreateMDWorkspace.";
  }
  /// Algorithm's verion for identification
  virtual int version() const { return 1; }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "MDAlgorithms"; }

private:
  /// Initialise the properties
  void init();
  /// Run the algorithm
  void exec();
};

} // namespace Mantid
} // namespace MDAlgorithms

#endif /* MANTID_MDALGORITHMS_FAKEMDEVENTDATA_H_ */
