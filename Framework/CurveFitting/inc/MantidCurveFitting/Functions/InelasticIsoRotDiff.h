#ifndef MANTID_INELASTICISOROTDIFF_H_
#define MANTID_INELASTICISOROTDIFF_H_

// Mantid Coding standars <http://www.mantidproject.org/Coding_Standards>
// Mantid Headers from the same project
// Mantid headers from other projects
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/ParamFunction.h"
// 3rd party library headers (N/A)
// standard library headers (N/A)

namespace Mantid {
namespace CurveFitting {
namespace Functions {
/**
@author Jose Borreguero, NScD
@date 25/09/2016

Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

/**
 * @brief Inelastic part of the IsoRotDiff function.
 */
class DLLExport InelasticIsoRotDiff : public API::ParamFunction,
                                      public API::IFunction1D {
public:
  InelasticIsoRotDiff();

  /// overwrite IFunction base class methods
  void init() override;

  /// overwrite IFunction base class methods
  std::string name() const override { return "InelasticIsoRotDiff"; }

  /// overwrite IFunction base class methods
  const std::string category() const override { return "QuasiElastic"; }

protected:
  void function1D(double *out, const double *xValues,
                  const size_t nData) const override;
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid

#endif // MANTID_INELASTICISOROTDIFF_H_
