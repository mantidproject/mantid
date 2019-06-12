// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/ScriptRepositoryFactory.h"
#include "MantidAPI/ScriptRepository.h"
#include "MantidKernel/LibraryManager.h"
#include "MantidKernel/StringTokenizer.h"
#include <sstream>

namespace Mantid {
namespace API {

ScriptRepositoryFactoryImpl::ScriptRepositoryFactoryImpl()
    : Kernel::DynamicFactory<ScriptRepository>() {
  // we need to make sure the library manager has been loaded before we
  // are constructed so that it is destroyed after us and thus does
  // not close any loaded DLLs with loaded algorithms in them
  Mantid::Kernel::LibraryManager::Instance();
}

} // namespace API
} // namespace Mantid
