#ifndef MANTID_ALGORITHMS_MAXENTENTROPY_H_
#define MANTID_ALGORITHMS_MAXENTENTROPY_H_

#include "MantidAlgorithms/DllConfig.h"
#include <boost/shared_ptr.hpp>
#include <vector>

namespace Mantid {
namespace Algorithms {

/** MaxentEntropy : Abstract base class defining the necessary methods to
  calculate any type of entropy to be used by MaxEnt

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
class MANTID_ALGORITHMS_DLL MaxentEntropy {
public:
  // Constructor
  MaxentEntropy() = default;
  // Destructor
  virtual ~MaxentEntropy() = default;
  // First derivative of the entropy
  virtual std::vector<double> derivative(const std::vector<double> &values,
                                         double background) = 0;
  // Second derivative of the entropy
  virtual std::vector<double>
  secondDerivative(const std::vector<double> &values, double background) = 0;
  // Corrects an invalid value
  virtual std::vector<double> correctValues(const std::vector<double> &value,
                                            double newValue) = 0;
};

using MaxentEntropy_sptr = boost::shared_ptr<MaxentEntropy>;

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_MAXENTENTROPY_H_ */
