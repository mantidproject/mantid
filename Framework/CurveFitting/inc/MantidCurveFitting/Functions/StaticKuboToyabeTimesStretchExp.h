#ifndef MANTID_CURVEFITTING_STATICKUBOTOYABETIMESSTRETCHEXP_H_
#define MANTID_CURVEFITTING_STATICKUBOTOYABETIMESSTRETCHEXP_H_

#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/ParamFunction.h"

namespace Mantid {
namespace CurveFitting {
namespace Functions {

/**
StaticKuboToyabeTimesStretchExp fitting function

Represents multiplication of two other fitting functions: StaticKuboToyabe and
Stretched Exponential Decay.

@author Lamar Moore
@date 13/11/2015

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
class DLLExport StaticKuboToyabeTimesStretchExp : public API::ParamFunction,
                                                  public API::IFunction1D {
public:
  std::string name() const override {
    return "StaticKuboToyabeTimesStretchExp";
  }

  const std::string category() const override { return "Muon"; }

protected:
  void function1D(double *out, const double *xValues,
                  const size_t nData) const override;

  void init() override;
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid

#endif /* MANTID_CURVEFITTING_STATICKUBOTOYABETIMESSTRETCHEXP_H_ */