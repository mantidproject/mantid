#ifndef MANTID_CURVEFITTING_STRETCHEXPMUON_H_
#define MANTID_CURVEFITTING_STRETCHEXPMUON_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/ParamFunction.h"
namespace Mantid {
namespace CurveFitting {
namespace Functions {
/**
Provide stetch exponential function for Muon scientists

@author Karl Palmen, ISIS, RAL
@date 12/03/2012

Copyright &copy; 2007-2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport StretchExpMuon : public API::ParamFunction,
                                 public API::IFunction1D {
public:
  /// overwrite IFunction base class methods
  std::string name() const override { return "StretchExpMuon"; }
  const std::string category() const override { return "Muon"; }

protected:
  void function1D(double *out, const double *xValues,
                  const size_t nData) const override;
  void init() override;
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_STRETCHEXPMUON_H_*/
