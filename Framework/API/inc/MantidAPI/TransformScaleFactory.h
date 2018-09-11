#ifndef MANTID_API_TRANSFORMSCALEFACTORY_H_
#define MANTID_API_TRANSFORMSCALEFACTORY_H_

#include "MantidAPI/DllConfig.h"
#include "MantidKernel/DynamicFactory.h"
#include "MantidKernel/SingletonHolder.h"

namespace Mantid {
//
// Forward declarations
//
namespace Kernel {
class IPropertyManager;
}
namespace API {
//
// Forward declarations
//
class ITransformScale;

/**

Constructs a scaling transform object from a string

Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class MANTID_API_DLL TransformScaleFactoryImpl
    : public Kernel::DynamicFactory<ITransformScale> {
public:
  /// Returns scaling transform
  boost::shared_ptr<ITransformScale>
  create(const std::string &type) const override;
  TransformScaleFactoryImpl(const TransformScaleFactoryImpl &) = delete;
  TransformScaleFactoryImpl &
  operator=(const TransformScaleFactoryImpl &) = delete;

private:
  friend struct Mantid::Kernel::CreateUsingNew<TransformScaleFactoryImpl>;

  /// Private Constructor for singleton class
  TransformScaleFactoryImpl() = default;
  ~TransformScaleFactoryImpl() override = default;
  /// Override the DynamicFactory::createUnwrapped() method. We don't want it
  /// used here.
  ITransformScale *createUnwrapped(const std::string &className) const override;

  // Do not use default methods
};

using TransformScaleFactory =
    Mantid::Kernel::SingletonHolder<TransformScaleFactoryImpl>;

} // namespace API
} // namespace Mantid

namespace Mantid {
namespace Kernel {
EXTERN_MANTID_API template class MANTID_API_DLL
    Mantid::Kernel::SingletonHolder<Mantid::API::TransformScaleFactoryImpl>;
}
} // namespace Mantid

#endif /* MANTID_API_TRANSFORMSCALEFACTORY_H_ */
