#ifndef MANTID_ALGORITHMS_MAXENTENTROPYNEGATIVEVALUES_H_
#define MANTID_ALGORITHMS_MAXENTENTROPYNEGATIVEVALUES_H_

#include "MantidAlgorithms/DllConfig.h"
#include "MantidAlgorithms/MaxentEntropy.h"

namespace Mantid {
namespace Algorithms {

/** MaxentEntropyNegativeValues : TODO: DESCRIPTION

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
class MANTID_ALGORITHMS_DLL MaxentEntropyNegativeValues : public MaxentEntropy {
public:
  MaxentEntropyNegativeValues() = default;
  virtual ~MaxentEntropyNegativeValues() = default;

	double getDerivative(double x) override;
	double getSecondDerivative(double x) override;
	double correctValue(double x) override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_MAXENTENTROPYNEGATIVEVALUES_H_ */