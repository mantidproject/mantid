// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/CostFunctionFactory.h"
#include "MantidAPI/ICostFunction.h"
#include "MantidKernel/LibraryManager.h"

namespace Mantid {
namespace API {

CostFunctionFactoryImpl::CostFunctionFactoryImpl() : Kernel::DynamicFactory<ICostFunction>() {
  // we need to make sure the library manager has been loaded before we
  // are constructed so that it is destroyed after us and thus does
  // not close any loaded DLLs with loaded algorithms in them
  Mantid::Kernel::LibraryManager::Instance();
}

} // namespace API
} // namespace Mantid
