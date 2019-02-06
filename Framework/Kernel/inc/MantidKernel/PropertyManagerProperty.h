// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_KERNEL_PROPERTYMANAGERPROPERTY_H_
#define MANTID_KERNEL_PROPERTYMANAGERPROPERTY_H_

#include "MantidKernel/PropertyManager_fwd.h"
#include "MantidKernel/PropertyWithValue.h"

namespace Mantid {
namespace Kernel {

class MANTID_KERNEL_DLL PropertyManagerProperty final
    : public PropertyWithValue<PropertyManager_sptr> {
public:
  // Convenience typedefs
  using BaseClass = PropertyWithValue<PropertyManager_sptr>;
  using ValueType = PropertyManager_sptr;

  PropertyManagerProperty(const std::string &name,
                          unsigned int direction = Direction::Input);
  PropertyManagerProperty(const std::string &name,
                          const ValueType &defaultValue,
                          unsigned int direction = Direction::Input);
  using BaseClass::operator=;
  PropertyManagerProperty *clone() const override {
    return new PropertyManagerProperty(*this);
  }

  std::string value() const override;
  Json::Value valueAsJson() const override;
  std::string getDefault() const override;
  std::string setValue(const std::string &strValue) override;
  std::string setValueFromJson(const Json::Value &value) override;

private:
  std::string m_dataServiceKey;
  std::string m_defaultAsStr;
};

} // namespace Kernel
} // namespace Mantid

#endif /* MANTID_KERNEL_PROPERTYMANAGERPROPERTY_H_ */
