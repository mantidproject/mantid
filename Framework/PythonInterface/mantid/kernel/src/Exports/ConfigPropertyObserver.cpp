// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/ConfigPropertyObserver.h"
#include "MantidPythonInterface/core/CallMethod.h"
#include "MantidPythonInterface/core/GlobalInterpreterLock.h"
#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/pure_virtual.hpp>

using namespace boost::python;
using Mantid::Kernel::ConfigPropertyObserver;
using Mantid::PythonInterface::callMethod;

class ConfigPropertyObserverWrapper : public ConfigPropertyObserver {
public:
  ConfigPropertyObserverWrapper(PyObject *self, const std::string &propertyName)
      : ConfigPropertyObserver(propertyName), m_self(self) {}

  void onPropertyValueChanged(const std::string &newValue,
                              const std::string &prevValue) override {
    callMethod<void>(m_self, "onPropertyValueChanged", newValue, prevValue);
  }

private:
  PyObject *m_self;
};

namespace boost {
namespace python {
template <>
struct has_back_reference<ConfigPropertyObserverWrapper> : mpl::true_ {};
} // namespace python
} // namespace boost

void export_ConfigPropertyObserver() {
  class_<ConfigPropertyObserverWrapper, boost::noncopyable>(
      "ConfigPropertyObserver", init<std::string>())
      .def(init<std::string>())
      .def(
          "onPropertyValueChanged",
          pure_virtual(&ConfigPropertyObserverWrapper::onPropertyValueChanged));
}
