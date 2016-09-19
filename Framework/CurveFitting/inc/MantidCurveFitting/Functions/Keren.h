#ifndef MANTID_CURVEFITTING_KEREN_H_
#define MANTID_CURVEFITTING_KEREN_H_

#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidCurveFitting/DllConfig.h"

namespace Mantid {
namespace CurveFitting {
namespace Functions {

/** Keren : Keren fitting function for muon scientists

  Generalization of the Abragam relaxation function to a longitudinal field
  See Phys Rev B, 50, 10039 (1994)

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
class MANTID_CURVEFITTING_DLL Keren : public API::ParamFunction,
                                      public API::IFunction1D {

public:
  /// Name of function
  std::string name() const override { return "Keren"; }
  /// Category for function
  const std::string category() const override { return "Muon"; }
  /// Set active parameter
  void setActiveParameter(size_t i, double value) override;
  /// Get active parameter
  double activeParameter(size_t i) const override;

protected:
  /// Evaluate the function at the given values
  void function1D(double *out, const double *xValues,
                  const size_t nData) const override;
  /// Initialize parameters
  void init() override;
  /// Time-dependent muon polarization
  double polarization(const double delta, const double larmor,
                      const double fluct, const double time) const;
  /// Relaxation form
  double relaxation(const double delta, const double larmor, const double fluct,
                    const double time) const;
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid

#endif /* MANTID_CURVEFITTING_KEREN_H_ */