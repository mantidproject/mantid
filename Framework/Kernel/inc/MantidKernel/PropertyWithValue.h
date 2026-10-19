// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/DllConfig.h"
#include "MantidKernel/IValidator_fwd.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Property.h"
#include "MantidKernel/PropertyManager_fwd.h"

#include <string>
#include <vector>

namespace Mantid {

namespace Kernel {
/** The concrete, templated class for properties.
    The supported types at present are int, double, bool & std::string.

    With reference to the Gaudi structure, this class can be seen as the
   equivalent of both the
    Gaudi class of the same name and its sub-classses.

    @class Mantid::Kernel::PropertyWithValue

    @author Russell Taylor, Tessella Support Services plc
    @author Based on the Gaudi class of the same name (see
   http://proj-gaudi.web.cern.ch/proj-gaudi/)
    @date 14/11/2007
*/
template <typename TYPE> class PropertyWithValue : public Property {
public:
  PropertyWithValue(std::string name, TYPE defaultValue, IValidator_sptr validator = nullptr,
                    const unsigned int direction = Direction::Input);
  PropertyWithValue(std::string name, TYPE defaultValue, const unsigned int direction);
  PropertyWithValue(const std::string &name, const TYPE &defaultValue, const std::string &defaultValueStr,
                    IValidator_sptr validator, const unsigned int direction);
  PropertyWithValue(const PropertyWithValue<TYPE> &right);
  PropertyWithValue() = delete;
  PropertyWithValue<TYPE> *clone() const override;

  void saveProperty(Nexus::File *file) override;
  std::string value() const override;
  std::string valueAsPrettyStr(const size_t maxLength = 0, const bool collapseLists = true) const override;
  Json::Value valueAsJson() const override;
  bool operator==(const PropertyWithValue<TYPE> &rhs) const;
  bool operator!=(const PropertyWithValue<TYPE> &rhs) const;
  int size() const override;
  std::string getDefault() const override;
  std::string setValue(const std::string &value) override;
  std::string setValueFromJson(const Json::Value &value) override;
  std::string setDataItem(const std::shared_ptr<DataItem> &data) override;
  PropertyWithValue &operator=(const PropertyWithValue &right);
  PropertyWithValue &operator+=(Property const *right) override;
  virtual PropertyWithValue &operator=(const TYPE &value);
  virtual const TYPE &operator()() const;
  virtual operator const TYPE &() const;
  std::string isValid() const override;
  bool isDefault() const override;
  std::vector<std::string> allowedValues() const override;
  bool isMultipleSelectionAllowed() override;
  virtual void replaceValidator(IValidator_sptr newValidator);
  IValidator_sptr getValidator() const;

protected:
  /// The value of the property
  TYPE m_value;
  /// the property's default value which is also its initial value
  // const TYPE m_initialValue;
  TYPE m_initialValue;

private:
  std::string setValueFromProperty(const Property &right) override;

  template <typename U> std::string setTypedValue(const U &value, const std::true_type &);

  template <typename U> std::string setTypedValue(const U &value, const std::false_type &);

  const TYPE getValueForAlias(const TYPE &alias) const;

  /// Visitor validator class
  IValidator_sptr m_validator;

  /// Static reference to the logger class
  static Logger g_logger;
};

// Instantiated and exported by PropertyWithValue.cpp. OptionalBool and Matrix
// are declared in OptionalBool.h and MatrixProperty.h.
MANTID_KERNEL_EXTERN_TEMPLATE(PropertyWithValue<float>);
MANTID_KERNEL_EXTERN_TEMPLATE(PropertyWithValue<double>);
MANTID_KERNEL_EXTERN_TEMPLATE(PropertyWithValue<int32_t>);
MANTID_KERNEL_EXTERN_TEMPLATE(PropertyWithValue<uint32_t>);
MANTID_KERNEL_EXTERN_TEMPLATE(PropertyWithValue<int64_t>);
MANTID_KERNEL_EXTERN_TEMPLATE(PropertyWithValue<uint64_t>);
MANTID_KERNEL_EXTERN_TEMPLATE(PropertyWithValue<uint16_t>);
MANTID_KERNEL_EXTERN_TEMPLATE(PropertyWithValue<bool>);
MANTID_KERNEL_EXTERN_TEMPLATE(PropertyWithValue<std::string>);
MANTID_KERNEL_EXTERN_TEMPLATE(PropertyWithValue<std::vector<float>>);
MANTID_KERNEL_EXTERN_TEMPLATE(PropertyWithValue<std::vector<double>>);
MANTID_KERNEL_EXTERN_TEMPLATE(PropertyWithValue<std::vector<int32_t>>);
MANTID_KERNEL_EXTERN_TEMPLATE(PropertyWithValue<std::vector<uint16_t>>);
MANTID_KERNEL_EXTERN_TEMPLATE(PropertyWithValue<std::vector<uint32_t>>);
MANTID_KERNEL_EXTERN_TEMPLATE(PropertyWithValue<std::vector<int64_t>>);
MANTID_KERNEL_EXTERN_TEMPLATE(PropertyWithValue<std::vector<uint64_t>>);
MANTID_KERNEL_EXTERN_TEMPLATE(PropertyWithValue<std::vector<bool>>);
MANTID_KERNEL_EXTERN_TEMPLATE(PropertyWithValue<std::vector<std::string>>);
MANTID_KERNEL_EXTERN_TEMPLATE(PropertyWithValue<std::vector<std::vector<int32_t>>>);
MANTID_KERNEL_EXTERN_TEMPLATE(PropertyWithValue<std::vector<std::vector<std::string>>>);
MANTID_KERNEL_EXTERN_TEMPLATE(PropertyWithValue<std::shared_ptr<PropertyManager>>);
// The integer type of the same width that is not int64_t
#if defined(_WIN32) || defined(__clang__) && defined(__APPLE__)
MANTID_KERNEL_EXTERN_TEMPLATE(PropertyWithValue<long>);
MANTID_KERNEL_EXTERN_TEMPLATE(PropertyWithValue<unsigned long>);
MANTID_KERNEL_EXTERN_TEMPLATE(PropertyWithValue<std::vector<long>>);
MANTID_KERNEL_EXTERN_TEMPLATE(PropertyWithValue<std::vector<unsigned long>>);
MANTID_KERNEL_EXTERN_TEMPLATE(PropertyWithValue<std::vector<std::vector<long>>>);
#endif
#ifdef __linux__
MANTID_KERNEL_EXTERN_TEMPLATE(PropertyWithValue<long long>);
MANTID_KERNEL_EXTERN_TEMPLATE(PropertyWithValue<unsigned long long>);
MANTID_KERNEL_EXTERN_TEMPLATE(PropertyWithValue<std::vector<long long>>);
MANTID_KERNEL_EXTERN_TEMPLATE(PropertyWithValue<std::vector<unsigned long long>>);
MANTID_KERNEL_EXTERN_TEMPLATE(PropertyWithValue<std::vector<std::vector<long long>>>);
#endif

template <typename TYPE> Logger PropertyWithValue<TYPE>::g_logger("PropertyWithValue");

} // namespace Kernel
} // namespace Mantid
