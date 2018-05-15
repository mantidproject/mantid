#ifndef FUNCTIONADAPTERTESTCOMMON_H
#define FUNCTIONADAPTERTESTCOMMON_H

#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IPeakFunction.h"
#include <boost/algorithm/string/replace.hpp>
#include <boost/python/detail/wrap_python.hpp>

namespace Mantid {
namespace PythonInterface {

// Generic template is NOT defined. This causes a linker error when
// called by a specialization that doesn't yet exist. If you get this
// then you need to write a new specialization for the given function type
template <typename FunctionType> std::string clsBlueprint();

template <> inline std::string clsBlueprint<Mantid::API::IFunction1D>() {
  return "from mantid.api import IFunction1D, FunctionFactory\n"
         "class {0}(IFunction1D):\n"
         "    def init(self):\n"
         "        pass\n"
         "    def function1D(self, x):\n"
         "{1}\n\n"
         "FunctionFactory.Instance().subscribe({0})\n";
}

template <> inline std::string clsBlueprint<Mantid::API::IPeakFunction>() {
  return "from mantid.api import IPeakFunction, FunctionFactory\n"
         "class {0}(IPeakFunction):\n"
         "    def init(self):\n"
         "        pass\n"
         "    def functionLocal(self, x):\n"
         "{1}\n"
         "    def centre(self):"
         "        return 0.0\n"
         "    def setCentre(self, x):\n"
         "        pass\n"
         "    def height(self):\n"
         "        return 1.0\n"
         "    def setHeight(self, x):\n"
         "        pass\n"
         "    def fwhm(self):\n"
         "        return 0.1\n"
         "    def setFwhm(self, x):\n"
         "        pass\n"
         "FunctionFactory.Instance().subscribe({0})\n";
}

template <typename FunctionType>
void subscribeTestFunction(std::string clsName, std::string functionImpl) {
  using boost::algorithm::replace_all;
  using boost::algorithm::replace_all_copy;
  std::string blueprint =
      replace_all_copy(clsBlueprint<FunctionType>(), "{0}", clsName);
  replace_all(blueprint, "{1}", functionImpl);
  PyRun_SimpleString(blueprint.c_str());
}

template <typename FunctionType>
boost::shared_ptr<FunctionType> createTestFunction(std::string clsName,
                                                   std::string functionImpl) {
  using Mantid::API::FunctionFactory;
  subscribeTestFunction<FunctionType>(clsName, functionImpl);
  return boost::dynamic_pointer_cast<FunctionType>(
      FunctionFactory::Instance().createFunction(clsName));
}
}
}

#endif // FUNCTIONADAPTERTESTCOMMON_H
