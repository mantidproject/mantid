#ifndef MANTID_ALGORITHMS_MAXENTTRANSFORMFOURIER_H_
#define MANTID_ALGORITHMS_MAXENTTRANSFORMFOURIER_H_

#include "MantidAlgorithms/MaxEnt/MaxentSpace.h"
#include "MantidAlgorithms/MaxEnt/MaxentTransform.h"

namespace Mantid {
namespace Algorithms {

/** MaxentTransformFourier : Defines a transformation from data space to image
  space (and vice-versa) where spaces are related by a **1D** Fourier Transform.

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
class MANTID_ALGORITHMS_DLL MaxentTransformFourier : public MaxentTransform {
public:
  // Deleted default constructor
  MaxentTransformFourier() = delete;
  // Constructor
  MaxentTransformFourier(MaxentSpace_sptr dataSpace,
                         MaxentSpace_sptr imageSpace);
  // Transfoms form image space to data space
  std::vector<double> imageToData(const std::vector<double> &image) override;
  // Transforms from data space to image space
  std::vector<double> dataToImage(const std::vector<double> &data) override;

private:
  MaxentSpace_sptr m_dataSpace;
  MaxentSpace_sptr m_imageSpace;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_MAXENTTRANSFORMFOURIER_H_ */
