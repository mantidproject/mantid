#ifndef MANTID_ALGORITHMS_MAXENTSPACE_H_
#define MANTID_ALGORITHMS_MAXENTSPACE_H_

#include "MantidAlgorithms/DllConfig.h"
#include <boost/shared_ptr.hpp>
#include <vector>

namespace Mantid {
namespace Algorithms {

/** MaxentSpace : Abstract base class defining a MaxentSpace. A Maxent space can
  be the space of real numbers or the space of complex numbers at the moment.
  This class is intended to be used in conjunction with MaxentTransformFourier,
  to define the input and output to/from the Fourier transform.

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
class MANTID_ALGORITHMS_DLL MaxentSpace {
public:
  // Constructor
  MaxentSpace() = default;
  // Destructor
  virtual ~MaxentSpace() = default;
  // Converts a given vector to a complex vector
  virtual std::vector<double> toComplex(const std::vector<double> &values) = 0;
  // Converts to a complex vector
  virtual std::vector<double>
  fromComplex(const std::vector<double> &values) = 0;
};

using MaxentSpace_sptr = boost::shared_ptr<MaxentSpace>;

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_MAXENTSPACE_H_ */