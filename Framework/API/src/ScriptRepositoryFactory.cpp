#include "MantidAPI/ScriptRepositoryFactory.h"
#include "MantidAPI/ScriptRepository.h"
#include "MantidKernel/LibraryManager.h"
#include <Poco/StringTokenizer.h>
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

ScriptRepositoryFactoryImpl::~ScriptRepositoryFactoryImpl() {}

} // namespace API
} // namespace Mantid
