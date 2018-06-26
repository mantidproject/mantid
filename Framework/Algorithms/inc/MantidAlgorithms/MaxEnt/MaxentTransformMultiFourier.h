#ifndef MANTID_ALGORITHMS_MAXENTTRANSFORMMULTIFOURIER_H_
#define MANTID_ALGORITHMS_MAXENTTRANSFORMMULTIFOURIER_H_

#include "MantidAlgorithms/MaxEnt/MaxentTransformFourier.h"
#include "MantidAlgorithms/MaxEnt/MaxentSpace.h"
#include "MantidAlgorithms/MaxEnt/MaxentSpaceComplex.h"
//#include <memory>

namespace Mantid {
namespace Algorithms {

using MaxentSpace_sptr = boost::shared_ptr<MaxentSpace>;
using MaxentSpaceComplex_sptr = boost::shared_ptr<MaxentSpaceComplex>;

/** MaxentTransformMultiFourier : Defines a transformation from 
  data space to image space (and vice-versa) 
  where spaces are related by a **1D** Fourier Transform,
  in which which the data has multiple spectra concatenatenated.

  In transforming from data to image, the spectra are added together
  before tranforming to a single image.
  In transforming the image to data, copies to the transformed data
  (one for each spectrum) are concatenated and then have the supplied
  adjustments applied.

  The concatenated format of the data is chosen to enable existing code
  to calculate its chi squared.


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
class MANTID_ALGORITHMS_DLL MaxentTransformMultiFourier : public MaxentTransformFourier {
public:
  // Deleted default constructor
  MaxentTransformMultiFourier() = delete;
  // Constructor
  MaxentTransformMultiFourier(MaxentSpaceComplex_sptr dataSpace,
                         MaxentSpace_sptr imageSpace,
                         size_t numSpec);
  // Transfoms form image space to data space
  std::vector<double> imageToData(const std::vector<double> &image) override;
  // Transforms from data space to image space
  std::vector<double> dataToImage(const std::vector<double> &data) override;
  // Set the adjustments to be applie to data when converted from image
  void setAdjustments(const std::vector<double> &linAdj, const std::vector<double> &constAdj);

private:
  MaxentSpace_sptr m_dataSpace;
  MaxentSpace_sptr m_imageSpace;
  size_t m_numSpec;
  std::vector<double> m_linearAdjustments;
  std::vector<double> m_constAdjustments;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_MAXENTTRANSFORMMULTIFOURIER_H_ */
