#ifndef MANTID_API_ASSOCIATIVECOMPOSITEFUNCTION_H_
#define MANTID_API_ASSOCIATIVECOMPOSITEFUNCTION_H_

#include "MantidAPI/CompositeFunction.h"

namespace Mantid {
    namespace API {
/** Subclass of CompositeFunction that enforces the associative property. If ASF
   denotes the composition operation, then:
   ASF(ASF(f,g), h) == ASF(f, ASF(g, h)) == ASF(f, g, h)

    @author Jose Borreguero, NScD Oak Ridge
    @date 2017/08/15

    Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

    File change history is stored at: <https://github.com/mantidproject/mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport AssociativeCompositeFunction : public CompositeFunction {
public:
    std::string name() const override { return "AssociativeCompositeFunction"; }

    /// Mechanism to ascertain if the component functions of function f
    /// are to be separately incorporated. Subclasses must override.
    bool virtual isAssociative(IFunction_sptr f) const = 0;

    /// Add a function at the back of the internal function list
    virtual size_t addFunction(IFunction_sptr f) override;

    /// Insert a function at a given index in the vector of component functions
    void insertFunction(size_t i, IFunction_sptr f);

    /// Replace a function
    void replaceFunction(size_t i, IFunction_sptr f);
};

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_ASSOCIATIVECOMPOSITEFUNCTION_H_*/