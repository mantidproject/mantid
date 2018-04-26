#ifndef MANTID_ALGORITHMS_SHIFTLOGTIME_H_
#define MANTID_ALGORITHMS_SHIFTLOGTIME_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Algorithms {

/** ShiftLogTime : TODO: DESCRIPTION

  @date 2011-08-26

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
class DLLExport ShiftLogTime : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"CreateLogTimeCorrection", "ChangePulsetime", "ChangeLogTime"};
  }
  const std::string category() const override;

  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Shifts the indexes of the specified log. This will make the log "
           "shorter by the specified shift.";
  }

private:
  /// Sets documentation strings for this algorithm

  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_SHIFTLOGTIME_H_ */
