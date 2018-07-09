#ifndef MANTID_ALGORITHMS_APPLYDETAILEDBALANCE_H_
#define MANTID_ALGORITHMS_APPLYDETAILEDBALANCE_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Algorithms {

/** ApplyDetailedBalance : The algorithm calculates the imaginary part of the
  dynamic susceptibility chi''=Pi*(1-exp(-E/T))*S

  @author Andrei Savici, ORNL
  @date 2011-09-01

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
class DLLExport ApplyDetailedBalance : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "ApplyDetailedBalance"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Transform scattering intensity to dynamic susceptibility.";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; };
  /// Algorithm's category for identification
  const std::string category() const override {
    return "Inelastic\\Corrections";
  }

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_APPLYDETAILEDBALANCE_H_ */
