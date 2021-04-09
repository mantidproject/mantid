// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/ConfigObserver.h"
#include "MantidPythonInterface/core/CallMethod.h"
#include "MantidPythonInterface/core/GlobalInterpreterLock.h"
#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/pure_virtual.hpp>

using namespace boost::python;
using Mantid::Kernel::ConfigObserver;
using Mantid::PythonInterface::callMethod;

class ConfigObserverWrapper : public ConfigObserver {
public:
  explicit ConfigObserverWrapper(PyObject *self) : m_self(self) {}
  using ConfigObserver::notifyValueChanged;

  void onValueChanged(const std::string &name, const std::string &newValue, const std::string &prevValue) override {
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
      .def("onValueChanged", pure_virtual(&ConfigObserverWrapper::onValueChanged));
}
