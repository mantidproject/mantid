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
template <typename FunctionType>
std::string clsBlueprint(bool includeDerivative);

template <>
inline std::string
clsBlueprint<Mantid::API::IFunction1D>(bool includeDerivative) {
  std::string blueprint =
      "from mantid.api import IFunction1D, FunctionFactory\n"
      "class {0}(IFunction1D):\n"
      "    def init(self):\n"
      "        self.declareParameter('A', 1.0)\n"
      "    def function1D(self, x):\n"
      "{1}\n";
  if (includeDerivative) {
    blueprint.append("    def functionDeriv1D(self, x, jacobian):\n"
                     "{2}\n");
  }
  blueprint.append("FunctionFactory.Instance().subscribe({0})\n");
  return blueprint;
}

template <>
inline std::string
clsBlueprint<Mantid::API::IPeakFunction>(bool includeDerivative) {
  std::string blueprint =
      "from mantid.api import IPeakFunction, FunctionFactory\n"
      "class {0}(IPeakFunction):\n"
      "    def init(self):\n"
      "        self.declareParameter('A', 1.0)\n"
      "    def functionLocal(self, x):\n"
      "{1}\n";
  if (includeDerivative) {
    blueprint.append("    def functionDerivLocal(self, x, jacobian):\n"
                     "{2}\n");
  }
  blueprint.append("    def centre(self):"
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
                   "FunctionFactory.Instance().subscribe({0})\n");
  return blueprint;
}

template <typename FunctionType>
void subscribeTestFunction(const std::string &clsName, std::string functionImpl,
                           std::string derivImpl) {
  using boost::algorithm::replace_all;
  using boost::algorithm::replace_all_copy;
  const bool includeDerivative(!derivImpl.empty());
  std::string blueprint = replace_all_copy(
      clsBlueprint<FunctionType>(includeDerivative), "{0}", clsName);
  replace_all(blueprint, "{1}", functionImpl);
  if (includeDerivative) {
    replace_all(blueprint, "{2}", derivImpl);
  }
  PyRun_SimpleString(blueprint.c_str());
}

template <typename FunctionType>
boost::shared_ptr<FunctionType> createTestFunction(std::string clsName,
                                                   std::string functionImpl,
                                                   std::string derivImpl = "") {
  using Mantid::API::FunctionFactory;
  subscribeTestFunction<FunctionType>(clsName, std::move(functionImpl),
                                      std::move(derivImpl));
  return boost::dynamic_pointer_cast<FunctionType>(
      FunctionFactory::Instance().createFunction(clsName));
}

class FunctionAdapterTestJacobian : public Mantid::API::Jacobian {
public:
  FunctionAdapterTestJacobian(size_t ny, size_t np)
      : m_np(np), m_data(ny * np) {}
  void set(size_t iY, size_t iP, double value) override {
    m_data[iY * m_np + iP] = value;
  }
  double get(size_t iY, size_t iP) override { return m_data[iY * m_np + iP]; }
  void zero() override { m_data.assign(m_data.size(), 0.0); }

private:
  size_t m_np;
  std::vector<double> m_data;
};
}
}

#endif // FUNCTIONADAPTERTESTCOMMON_H
