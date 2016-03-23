#ifndef MANTID_API_IFUNCTIONGENERAL_H_
#define MANTID_API_IFUNCTIONGENERAL_H_

#include "MantidAPI/DllConfig.h"

#include "MantidAPI/IFunction.h"
#include "MantidAPI/FunctionDomainGeneral.h"
#include "MantidKernel/Logger.h"

namespace Mantid {
namespace API {

/** IFunctionGeneral :

    

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
class DLLExport IFunctionGeneral : public virtual IFunction {
public:
  void function(const FunctionDomain &domain,
                FunctionValues &values) const override;
  void functionDeriv(const FunctionDomain &domain, Jacobian &jacobian) override;

  /// Provide a concrete function in an implementation that operates on a
  /// FunctionDomainGeneral.
  virtual void functionGeneral(const FunctionDomainGeneral &domain,
                                  FunctionValues &values) const = 0;

protected:
  static Kernel::Logger g_log;
};

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_IFUNCTIONGENERAL_H_ */
