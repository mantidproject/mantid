#ifndef IMPLICIT_FUNCTION_FACTORY
#define IMPLICIT_FUNCTION_FACTORY

/** @class ImplicitFunctionFactory ImplicitFunctionFactory.h
Kernel/ImplicitFunctionFactory.h

This dynamic factory implementation generates concrete instances of
ImplicitFunctions.

    @author Owen Arnold, Tessella Support Services plc
    @date 27/10/2010

Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

#include "ImplicitFunctionParserFactory.h"
#include "MantidAPI/DllConfig.h"
#include "MantidGeometry/MDGeometry/MDImplicitFunction.h"
#include "MantidKernel/DynamicFactory.h"
#include "MantidKernel/SingletonHolder.h"

namespace Mantid {
namespace API {
class MANTID_API_DLL ImplicitFunctionFactoryImpl
    : public Kernel::DynamicFactory<Mantid::Geometry::MDImplicitFunction> {
public:
  ImplicitFunctionFactoryImpl(const ImplicitFunctionFactoryImpl &) = delete;
  ImplicitFunctionFactoryImpl &
  operator=(const ImplicitFunctionFactoryImpl &) = delete;
  Mantid::Geometry::MDImplicitFunction_sptr
  create(const std::string &className) const override;

  virtual Mantid::Geometry::MDImplicitFunction *
  createUnwrapped(Poco::XML::Element *processXML) const;

  Mantid::Geometry::MDImplicitFunction *
  createUnwrapped(const std::string &processXML) const override;

  friend struct Mantid::Kernel::CreateUsingNew<ImplicitFunctionFactoryImpl>;

private:
  /// Private Constructor for singleton class
  ImplicitFunctionFactoryImpl() = default;
  /// Private Destructor
  ~ImplicitFunctionFactoryImpl() override = default;
};

using ImplicitFunctionFactory =
    Mantid::Kernel::SingletonHolder<ImplicitFunctionFactoryImpl>;
} // namespace API
} // namespace Mantid

namespace Mantid {
namespace Kernel {
EXTERN_MANTID_API template class MANTID_API_DLL
    Mantid::Kernel::SingletonHolder<Mantid::API::ImplicitFunctionFactoryImpl>;
}
} // namespace Mantid

#endif
