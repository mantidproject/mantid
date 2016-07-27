#ifndef MANTID_ALGORITHMS_CALCULATE_COUNTINGRATE_H_
#define MANTID_ALGORITHMS_CALCULATE_COUNTINGRATE_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Algorithms {

/**  In normal circumstances an instrument in event mode counts neutrons with constant steady rate which depends on beam, instrument settings and sample.
    Sometimes hardware issues cause it to count much faster or slower. This appears as spurious signals on the final neutronic images and users want to filter these signals.

    The calculate neutrons counting rate as the function of the experiment time and add appropriate logs to the event workspace 
    for further event filtering on the basis of these logs.


  Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport CalculateCountingRate : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;

private:
  void init() override;
  void exec() override;

};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_CALCULATE_COUNTINGRATE_H_ */
