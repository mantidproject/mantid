#ifndef MANTID_CURVEFITTING_CALCULATECHISQUARED_H_
#define MANTID_CURVEFITTING_CALCULATECHISQUARED_H_

#include "MantidKernel/System.h"
#include "MantidCurveFitting/IFittingAlgorithm.h"

namespace Mantid {
namespace CurveFitting {

/**

  Calculate chi squared for a function and a data set in a workspace.

  Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport CalculateChiSquared : public IFittingAlgorithm {
public:
  CalculateChiSquared();
  virtual ~CalculateChiSquared();

  virtual const std::string name() const;
  virtual int version() const;
  virtual const std::string summary() const;

private:
  void initConcrete();
  void execConcrete();
};

} // namespace CurveFitting
} // namespace Mantid

#endif /* MANTID_CURVEFITTING_CALCULATECHISQUARED_H_ */
