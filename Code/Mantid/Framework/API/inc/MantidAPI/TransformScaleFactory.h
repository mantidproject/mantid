#ifndef MANTID_API_TRANSFORMSCALEFACTORY_H_
#define MANTID_API_TRANSFORMSCALEFACTORY_H_

#include "MantidAPI/DllConfig.h"
#include "MantidKernel/ClassMacros.h"
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
  boost::shared_ptr<ITransformScale> create(const std::string &type) const;

private:
  friend struct Mantid::Kernel::CreateUsingNew<TransformScaleFactoryImpl>;

  /// Private Constructor for singleton class
  TransformScaleFactoryImpl();
  /// Private destructor for singleton
  virtual ~TransformScaleFactoryImpl();
  /// Override the DynamicFactory::createUnwrapped() method. We don't want it
  /// used here.
  ITransformScale *createUnwrapped(const std::string &className) const;
  /// Private copy constructor - NO COPY ALLOWED
  TransformScaleFactoryImpl(const TransformScaleFactoryImpl &);
  /// Private assignment operator - NO ASSIGNMENT ALLOWED
  TransformScaleFactoryImpl &operator=(const TransformScaleFactoryImpl &);
  // Do not use default methods
};

/// Forward declaration of a specialisation of SingletonHolder for
/// TransformScaleFactoryImpl (needed for dllexport/dllimport) and a typedef for
/// it.
#ifdef _WIN32
// this breaks new namespace declaraion rules; need to find a better fix
template class MANTID_API_DLL
    Mantid::Kernel::SingletonHolder<TransformScaleFactoryImpl>;
#endif /* _WIN32 */
typedef MANTID_API_DLL Mantid::Kernel::SingletonHolder<
    TransformScaleFactoryImpl> TransformScaleFactory;

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_TRANSFORMSCALEFACTORY_H_ */
