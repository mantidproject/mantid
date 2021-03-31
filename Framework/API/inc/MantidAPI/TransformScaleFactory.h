// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

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
 */
class MANTID_API_DLL TransformScaleFactoryImpl : public Kernel::DynamicFactory<ITransformScale> {
public:
  /// Returns scaling transform
  std::shared_ptr<ITransformScale> create(const std::string &type) const override;
  TransformScaleFactoryImpl(const TransformScaleFactoryImpl &) = delete;
  TransformScaleFactoryImpl &operator=(const TransformScaleFactoryImpl &) = delete;

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

using TransformScaleFactory = Mantid::Kernel::SingletonHolder<TransformScaleFactoryImpl>;

} // namespace API
} // namespace Mantid

namespace Mantid {
namespace Kernel {
EXTERN_MANTID_API template class MANTID_API_DLL Mantid::Kernel::SingletonHolder<Mantid::API::TransformScaleFactoryImpl>;
}
} // namespace Mantid
