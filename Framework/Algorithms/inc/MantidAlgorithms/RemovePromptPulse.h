#ifndef MANTID_ALGORITHMS_REMOVEPROMPTPULSE_H_
#define MANTID_ALGORITHMS_REMOVEPROMPTPULSE_H_

#include "MantidAPI/DistributedAlgorithm.h"
#include "MantidAPI/Run.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Algorithms {

/** RemovePromptPulse : TODO: DESCRIPTION

  @author
  @date 2011-07-18

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
class DLLExport RemovePromptPulse : public API::DistributedAlgorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override;

  /// Algorithm's version for identification
  int version() const override;

  /// Algorithm's category for identification
  const std::string category() const override;

  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Remove the prompt pulse for a time of flight measurement.";
  }

private:
  /// Sets documentation strings for this algorithm

  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;
  /// Try to get the frequency from a given name.
  double getFrequency(const API::Run &run);
  std::vector<double> calculatePulseTimes(const double tmin, const double tmax,
                                          const double period);
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_REMOVEPROMPTPULSE_H_ */
