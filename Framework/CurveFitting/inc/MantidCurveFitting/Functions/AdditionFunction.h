#ifndef MANTID_CURVEFITTING_ADDITIONFUNCTION_H_
#define MANTID_CURVEFITTING_ADDITIONFUNCTION_H_

#endif /*MANTID_CURVEFITTING_ADDITIONFUNCTION_H_*/

#include "MantidAPI/IFunction.h"
#include "MantidAPI/AssociativeCompositeFunction.h"

namespace Mantid {
namespace CurveFitting {
namespace Functions {
/**
Same as CompositeFunction but enforcing the associative property
of the + operator

@author Jose Borreguero, NScD Oak Ridge
@date 2017/08/16

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

class DLLExport AdditionFunction : public API::AssociativeCompositeFunction {
public:
    /// overwrite IFunction base class methods
    std::string name() const override { return "AdditionFunction"; }
    /// if f is of the same class then its component functions
    /// are treated separately
    bool isAssociative(API::IFunction_sptr f) const override;

protected:
    /// overwrite IFunction base class method, which declare function parameters
    void init() override {};
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid