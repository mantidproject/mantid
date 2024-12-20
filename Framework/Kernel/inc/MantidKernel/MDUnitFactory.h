// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/ChainableFactory.h"
#include "MantidKernel/DllConfig.h"
#include "MantidKernel/MDUnit.h"
#include <memory>

namespace Mantid {
namespace Kernel {

/** MDUnitFactory : Abstract type. Factory method with chain of reponsibility
  succession for creating MDUnits.
*/
class MANTID_KERNEL_DLL MDUnitFactory : public ChainableFactory<MDUnitFactory, MDUnit, std::string> {

private:
  /// Create the product
  MDUnit *createRaw(const std::string &unitString) const override = 0;

  /// Indicate an ability to intepret the string
  bool canInterpret(const std::string &unitString) const override = 0;
};

//-----------------------------------------------------------------------
// Derived MDUnitFactory declarations
//-----------------------------------------------------------------------

class MANTID_KERNEL_DLL LabelUnitFactory : public MDUnitFactory {
  LabelUnit *createRaw(const std::string &unitString) const override;
  bool canInterpret(const std::string &unitString) const override;
};

class MANTID_KERNEL_DLL InverseAngstromsUnitFactory : public MDUnitFactory {
  InverseAngstromsUnit *createRaw(const std::string &unitString) const override;
  bool canInterpret(const std::string &unitString) const override;
};

class MANTID_KERNEL_DLL ReciprocalLatticeUnitFactory : public MDUnitFactory {
  ReciprocalLatticeUnit *createRaw(const std::string &unitString) const override;
  bool canInterpret(const std::string &unitString) const override;
};

using MDUnitFactory_uptr = std::unique_ptr<MDUnitFactory>;

using MDUnitFactory_const_uptr = std::unique_ptr<const MDUnitFactory>;

/// Convience method. Pre-constructed builder chain.
MDUnitFactory_uptr MANTID_KERNEL_DLL makeMDUnitFactoryChain();

} // namespace Kernel
} // namespace Mantid
