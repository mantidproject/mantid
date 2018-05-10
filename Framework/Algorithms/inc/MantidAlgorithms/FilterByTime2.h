#ifndef MANTID_ALGORITHMS_FILTERBYTIME2_H_
#define MANTID_ALGORITHMS_FILTERBYTIME2_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Algorithms {

/** FilterByTime2 : TODO: DESCRIPTION

  @date 2012-04-25

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
class DLLExport FilterByTime2 : public API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "FilterByTime"; };
  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 2; };
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override {
    return "Events\\EventFiltering";
  }
  /// Algorithm's summary for identification overriding a virtual method
  const std::string summary() const override {
    return "Filter event data by time.";
  }

private:
  /// Sets documentation strings for this algorithm

  // Implement abstract Algorithm methods
  void init() override;
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_FILTERBYTIME2_H_ */
