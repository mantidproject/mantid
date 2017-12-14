#include "MantidKernel/ConfigServiceObserver.h"
#include <boost/python/class.hpp>
#include <boost/python/def.hpp>

using namespace boost::python;
using Mantid::Kernel::ConfigServiceObserver;

class ConfigServiceObserverWrapper : public ConfigServiceObserver,
                                     public wrapper<ConfigServiceObserver> {
public:
  using ConfigServiceObserver::ConfigServiceObserver;
  using ConfigServiceObserver::notifyValueChanged;
  void onValueChanged(const std::string &name, const std::string &newValue,
                      const std::string &prevValue) override {
    if (override onValueChangedOverride =
            this->get_override("onValueChanged")) {
      onValueChangedOverride(name, newValue, prevValue);
    } else {
      ConfigServiceObserver::onValueChanged(name, newValue, prevValue);
    }
  }

  void default_onValueChanged(const std::string &name,
                              const std::string &newValue,
                              const std::string &prevValue) {
    return this->ConfigServiceObserver::onValueChanged(name, newValue,
                                                       prevValue);
  }
};

void export_ConfigServiceObserver() {
  class_<ConfigServiceObserverWrapper, boost::noncopyable>(
      "ConfigServiceObserver")
      .def("onValueChanged", &ConfigServiceObserverWrapper::onValueChanged,
           &ConfigServiceObserverWrapper::default_onValueChanged);
}
