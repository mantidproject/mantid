#ifndef MANTID_ALGORITHMS_MAXENTENTROPYPOSITIVEVALUES_H_
#define MANTID_ALGORITHMS_MAXENTENTROPYPOSITIVEVALUES_H_

#include "MantidAlgorithms/DllConfig.h"
#include "MantidAlgorithms/MaxentEntropy.h"

namespace Mantid {
namespace Algorithms {

/** MaxentEntropyPositiveValues : TODO: DESCRIPTION

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
class MANTID_ALGORITHMS_DLL MaxentEntropyPositiveValues : public MaxentEntropy {
public:
  MaxentEntropyPositiveValues() = default;
  virtual ~MaxentEntropyPositiveValues() = default;

  double getDerivative(double value) override;
  double getSecondDerivative(double x) override;
  double correctValue(double x, double y) override;
};
// Helper typedef for scoped pointer of this type.
typedef boost::shared_ptr<MaxentEntropyPositiveValues>
    MaxentEntropyPositiveValues_sptr;

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_MAXENTENTROPYPOSITIVEVALUES_H_ */