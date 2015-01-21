#ifndef MANTID_ALGORITHMS_ADDPEAK_H_
#define MANTID_ALGORITHMS_ADDPEAK_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Algorithms {

/**
 * Add a peak to a PeaksWorkspace.

  @date 2012-10-16

  Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport AddPeak : public API::Algorithm {
public:
  AddPeak();
  ~AddPeak();

  /// Algorithm's name for identification
  virtual const std::string name() const { return "AddPeak"; };
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Adds a peak to a PeaksWorkspace.";
  }
  /// Algorithm's version for identification
  virtual int version() const { return 1; };
  /// Algorithm's category for identification
  virtual const std::string category() const {
    return "Crystal;Utility\\Workspaces";
  }

private:
  /// Initialise the properties
  void init();
  /// Run the algorithm
  void exec();
};

} // namespace Mantid
} // namespace Algorithms

#endif /* MANTID_ALGORITHMS_ADDPEAK_H_ */
