// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MDALGORITHMS_FOREGROUNDMODELFACTORY_H_
#define MANTID_MDALGORITHMS_FOREGROUNDMODELFACTORY_H_

#include "MantidKernel/DynamicFactory.h"
#include "MantidKernel/SingletonHolder.h"
#include "MantidMDAlgorithms/DllConfig.h"

namespace Mantid {
namespace API {
class IFunction;
}
namespace MDAlgorithms {
//
// Forward declaration
//
class ForegroundModel;

/**
 * A factory class for mapping string names of models to their
 * class types. Allows a model to be instantiated from a string.
 */
class MANTID_MDALGORITHMS_DLL ForegroundModelFactoryImpl
    : public Kernel::DynamicFactory<ForegroundModel> {
private:
  /// Base-class type
  using BaseClass = Kernel::DynamicFactory<ForegroundModel>;

public:
  /// A create method to ensure the model is initialized properly
  ForegroundModel *createModel(const std::string &name,
                               const API::IFunction &fitFunction);
  /// Disable copy operator
  ForegroundModelFactoryImpl(const ForegroundModelFactoryImpl &) = delete;

  /// Disable assignment operator
  ForegroundModelFactoryImpl &
  operator=(const ForegroundModelFactoryImpl &) = delete;

private:
  /// Policy needs to access constructor
  friend struct Kernel::CreateUsingNew<ForegroundModelFactoryImpl>;
  /// Default constructor
  ForegroundModelFactoryImpl();

  // Do not allow the default create & createUnwrapped to be called
  using BaseClass::create;
  using BaseClass::createUnwrapped;
};

/// Typedef singleton instance to ForegroundFactory
using ForegroundModelFactory =
    Kernel::SingletonHolder<ForegroundModelFactoryImpl>;
} // namespace MDAlgorithms
} // namespace Mantid

namespace Mantid {
namespace Kernel {
EXTERN_MANTID_MDALGORITHMS template class MANTID_MDALGORITHMS_DLL
    Kernel::SingletonHolder<Mantid::MDAlgorithms::ForegroundModelFactoryImpl>;
}
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_FOREGROUNDMODELFACTORY_H_ */
