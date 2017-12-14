#include "MantidKernel/ConfigPropertyObserver.h"
#include <boost/python/class.hpp>
#include <boost/python/def.hpp>

using namespace boost::python;
using Mantid::Kernel::ConfigPropertyObserver;

class ConfigPropertyObserverWrapper : public ConfigPropertyObserver,
                                      public wrapper<ConfigPropertyObserver> {
public:
  using ConfigPropertyObserver::ConfigPropertyObserver;
  void onPropertyValueChanged(const std::string &newValue,
                              const std::string &prevValue) override {
    if (override onPropertyValueChangedOverride =
            this->get_override("onPropertyValueChanged")) {
      onPropertyValueChangedOverride(newValue, prevValue);
    } else {
      ConfigPropertyObserver::onPropertyValueChanged(newValue, prevValue);
    }
  }

  void default_onPropertyValueChanged(const std::string &newValue,
                                      const std::string &prevValue) {
    return this->ConfigPropertyObserver::onPropertyValueChanged(newValue,
                                                                prevValue);
  }
};

void export_ConfigPropertyObserver() {
  class_<ConfigPropertyObserverWrapper, boost::noncopyable>(
      "ConfigPropertyObserver", init<std::string>())
      .def(init<std::string>())
      .def("onPropertyValueChanged",
           &ConfigPropertyObserverWrapper::onPropertyValueChanged,
           &ConfigPropertyObserverWrapper::default_onPropertyValueChanged);
}
