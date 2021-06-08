// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/FuncMinimizerFactory.h"
#include "MantidAPI/Expression.h"
#include "MantidAPI/IFuncMinimizer.h"
#include "MantidKernel/LibraryManager.h"

#include <stdexcept>

namespace Mantid {
namespace API {

FuncMinimizerFactoryImpl::FuncMinimizerFactoryImpl() : Kernel::DynamicFactory<IFuncMinimizer>() {
  // we need to make sure the library manager has been loaded before we
  // are constructed so that it is destroyed after us and thus does
  // not close any loaded DLLs with loaded algorithms in them
  Mantid::Kernel::LibraryManager::Instance();
}

/**
 * Creates an instance of a minimizer
 * @param str :: The minimizer initialization string which includes its type
 *   and optionally properties: "type,prop1=value1,prop2=value2"
 * @return A pointer to the created minimizer
 */
std::shared_ptr<IFuncMinimizer> FuncMinimizerFactoryImpl::createMinimizer(const std::string &str) const {
  // check if there are any properties defined - look for a comma
  if (str.find(',') == std::string::npos) { // no properties - create minimizer and return
    return create(str);
  }

  // parse the string
  Expression parser;
  parser.parse(str);
  parser.toList(); // make sure it is a list
  const size_t n = parser.size();
  if (n == 0) {
    std::string mess = "Found empty initialization string";
    throw std::invalid_argument(mess);
  }

  // create the minimizer from the type which is
  const std::string type = parser[0].str();
  auto minimizer = create(type);

  // set the properties
  for (size_t i = 1; i < n; ++i) {
    auto &param = parser[i];
    if (param.size() == 2 && param.name() == "=") {
      const std::string parName = param[0].str();
      if (minimizer->existsProperty(parName)) {
        minimizer->setPropertyValue(parName, param[1].str());
      }
    }
  }

  return minimizer;
}

} // namespace API
} // namespace Mantid
