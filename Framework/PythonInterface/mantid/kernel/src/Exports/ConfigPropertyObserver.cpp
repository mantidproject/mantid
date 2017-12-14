#include "MantidKernel/ConfigPropertyObserver.h"
#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/pure_virtual.hpp>

using namespace boost::python;
using Mantid::Kernel::ConfigPropertyObserver;

class ConfigPropertyObserverWrapper : public ConfigPropertyObserver,
                                      public wrapper<ConfigPropertyObserver> {
public:
  using ConfigPropertyObserver::ConfigPropertyObserver;
  void onPropertyValueChanged(const std::string &newValue,
                              const std::string &prevValue) override {
    auto onPropertyValueChangedOverride =
        this->get_override("onPropertyValueChanged");
    onPropertyValueChangedOverride(newValue, prevValue);
  }
};

void export_ConfigPropertyObserver() {
  class_<ConfigPropertyObserverWrapper, boost::noncopyable>(
      "ConfigPropertyObserver", init<std::string>())
      .def(init<std::string>())
      .def(
          "onPropertyValueChanged",
          pure_virtual(&ConfigPropertyObserverWrapper::onPropertyValueChanged));
}
