#include "MantidKernel/ConfigObserver.h"
#include <boost/python/class.hpp>
#include <boost/python/def.hpp>

using namespace boost::python;
using Mantid::Kernel::ConfigObserver;

class ConfigObserverWrapper : public ConfigObserver,
                              public wrapper<ConfigObserver> {
public:
  using ConfigObserver::ConfigObserver;
  using ConfigObserver::notifyValueChanged;
  void onValueChanged(const std::string &name, const std::string &newValue,
                      const std::string &prevValue) override {
    if (override onValueChangedOverride =
            this->get_override("onValueChanged")) {
      onValueChangedOverride(name, newValue, prevValue);
    } else {
      ConfigObserver::onValueChanged(name, newValue, prevValue);
    }
  }

  void default_onValueChanged(const std::string &name,
                              const std::string &newValue,
                              const std::string &prevValue) {
    return this->ConfigObserver::onValueChanged(name, newValue, prevValue);
  }
};

void export_ConfigObserver() {
  class_<ConfigObserverWrapper, boost::noncopyable>("ConfigObserver")
      .def("onValueChanged", &ConfigObserverWrapper::onValueChanged,
           &ConfigObserverWrapper::default_onValueChanged);
}
