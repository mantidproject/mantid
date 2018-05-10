#include "MantidKernel/ConfigObserver.h"
#include "MantidPythonInterface/kernel/Environment/CallMethod.h"
#include "MantidPythonInterface/kernel/Environment/GlobalInterpreterLock.h"
#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/pure_virtual.hpp>

using namespace boost::python;
using Mantid::Kernel::ConfigObserver;
using Mantid::PythonInterface::Environment::callMethod;

class ConfigObserverWrapper : public ConfigObserver {
public:
  explicit ConfigObserverWrapper(PyObject *self) : m_self(self) {}
  using ConfigObserver::notifyValueChanged;

  void onValueChanged(const std::string &name, const std::string &newValue,
                      const std::string &prevValue) override {
    callMethod<void>(m_self, "onValueChanged", name, newValue, prevValue);
  }

private:
  PyObject *m_self;
};

namespace boost {
namespace python {
template <> struct has_back_reference<ConfigObserverWrapper> : mpl::true_ {};
} // namespace python
} // namespace boost

void export_ConfigObserver() {
  class_<ConfigObserverWrapper, boost::noncopyable>("ConfigObserver")
      .def("onValueChanged",
           pure_virtual(&ConfigObserverWrapper::onValueChanged));
}
