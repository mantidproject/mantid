#include "MantidKernel/ConfigObserver.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidPythonInterface/kernel/Environment/GlobalInterpreterLock.h"
#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/pure_virtual.hpp>
#include <iostream>
#include <csignal>
#include <execinfo.h>

using namespace boost::python;
using Mantid::Kernel::ConfigObserver;
using Mantid::PythonInterface::Environment::GlobalInterpreterLock;

class ConfigObserverWrapper : public ConfigObserver,
                              public wrapper<ConfigObserver> {
public:
  using ConfigObserver::ConfigObserver;
  using ConfigObserver::notifyValueChanged;

  void onValueChanged(const std::string &name, const std::string &newValue,
                      const std::string &prevValue) override {
    try {
      GlobalInterpreterLock lock;
      auto onValueChangedOverride = this->get_override("onValueChanged");
      onValueChangedOverride(name, newValue, prevValue);
    } catch (const std::exception &exc) {
      std::cerr << "  what(): " << exc.what() << "\n\n";
    } catch (boost::python::error_already_set &) {
      PyErr_Print();
    } catch (...) {
      std::cerr << "  what(): Unknown exception type. No more information "
                   "available\n\n";
      std::cerr << "Backtrace:\n";
      void *trace_elems[32];
      int trace_elem_count(backtrace(trace_elems, 32));
      char **stack_syms(backtrace_symbols(trace_elems, trace_elem_count));
      for (int i = 0; i < trace_elem_count; ++i) {
        std::cerr << ' ' << stack_syms[i] << '\n';
      }
      free(stack_syms);
    }
  }
};

void export_ConfigObserver() {
  class_<ConfigObserverWrapper, boost::noncopyable>("ConfigObserver")
      .def("onValueChanged",
           pure_virtual(&ConfigObserverWrapper::onValueChanged));
}
