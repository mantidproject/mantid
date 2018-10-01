#ifndef MANTID_ALGORITHMS_MAXENTTRANSFORM_H_
#define MANTID_ALGORITHMS_MAXENTTRANSFORM_H_

#include "MantidAlgorithms/DllConfig.h"
#include <boost/shared_ptr.hpp>
#include <vector>

namespace Mantid {
namespace Algorithms {

/** MaxentTransform : Abstract base class defining MaxEnt transformations from
  image space to data space and vice-versa

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
class MANTID_ALGORITHMS_DLL MaxentTransform {
public:
  // Constructor
  MaxentTransform() = default;
  // Destructor
  virtual ~MaxentTransform() = default;
  // Transfoms form image space to data space
  virtual std::vector<double> imageToData(const std::vector<double> &image) = 0;
  // Transforms from data space to image space
  virtual std::vector<double> dataToImage(const std::vector<double> &data) = 0;
};

using MaxentTransform_sptr = boost::shared_ptr<MaxentTransform>;

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_MAXENTTRANSFORM_H_ */