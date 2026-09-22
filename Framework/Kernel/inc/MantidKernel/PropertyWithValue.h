// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/Logger.h"
#include "MantidKernel/NullValidator.h"
#include "MantidKernel/Property.h"
#include "MantidKernel/PropertyManager_fwd.h"

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
template <typename TYPE> class DLLExport PropertyWithValue : public Property {
public:
  PropertyWithValue(std::string name, TYPE defaultValue, IValidator_sptr validator = IValidator_sptr(new NullValidator),
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

template <> MANTID_KERNEL_DLL void PropertyWithValue<float>::saveProperty(Nexus::File *file);
template <> MANTID_KERNEL_DLL void PropertyWithValue<double>::saveProperty(Nexus::File *file);
template <> MANTID_KERNEL_DLL void PropertyWithValue<int32_t>::saveProperty(Nexus::File *file);
template <> MANTID_KERNEL_DLL void PropertyWithValue<uint32_t>::saveProperty(Nexus::File *file);
template <> MANTID_KERNEL_DLL void PropertyWithValue<int64_t>::saveProperty(Nexus::File *file);
template <> MANTID_KERNEL_DLL void PropertyWithValue<uint64_t>::saveProperty(Nexus::File *file);
template <> MANTID_KERNEL_DLL void PropertyWithValue<std::string>::saveProperty(Nexus::File *file);
template <> MANTID_KERNEL_DLL void PropertyWithValue<std::vector<double>>::saveProperty(Nexus::File *file);
template <> MANTID_KERNEL_DLL void PropertyWithValue<std::vector<int32_t>>::saveProperty(Nexus::File *file);

template <typename TYPE> Logger PropertyWithValue<TYPE>::g_logger("PropertyWithValue");

// 'extern template' declarations matching (most of) the explicit instantiations in
// Kernel/src/PropertyWithValue.cpp. Without these, every translation unit that declares a
// property of one of these types (e.g. via IPropertyManager::declareProperty) implicitly
// re-instantiates PropertyWithValue<TYPE> locally -- and this header is included, directly or
// via Algorithm.h, by most of the codebase. Matrix<T>, OptionalBool and their vectors are left
// out to avoid pulling Matrix.h/OptionalBool.h into this very high fan-out header.
#ifndef PROPERTYWITHVALUE_PROVIDES_EXPLICIT_INSTANTIATIONS
extern template class MANTID_KERNEL_DLL PropertyWithValue<uint16_t>;
extern template class MANTID_KERNEL_DLL PropertyWithValue<bool>;
extern template class MANTID_KERNEL_DLL PropertyWithValue<std::vector<float>>;
extern template class MANTID_KERNEL_DLL PropertyWithValue<std::vector<uint16_t>>;
extern template class MANTID_KERNEL_DLL PropertyWithValue<std::vector<uint32_t>>;
extern template class MANTID_KERNEL_DLL PropertyWithValue<std::vector<int64_t>>;
extern template class MANTID_KERNEL_DLL PropertyWithValue<std::vector<uint64_t>>;
extern template class MANTID_KERNEL_DLL PropertyWithValue<std::vector<bool>>;
extern template class MANTID_KERNEL_DLL PropertyWithValue<std::vector<std::string>>;
extern template class MANTID_KERNEL_DLL PropertyWithValue<std::vector<std::vector<int32_t>>>;
extern template class MANTID_KERNEL_DLL PropertyWithValue<std::vector<std::vector<std::string>>>;
extern template class MANTID_KERNEL_DLL PropertyWithValue<std::shared_ptr<PropertyManager>>;
#if defined(_WIN32) || defined(__clang__) && defined(__APPLE__)
extern template class MANTID_KERNEL_DLL PropertyWithValue<long>;
extern template class MANTID_KERNEL_DLL PropertyWithValue<unsigned long>;
extern template class MANTID_KERNEL_DLL PropertyWithValue<std::vector<long>>;
extern template class MANTID_KERNEL_DLL PropertyWithValue<std::vector<unsigned long>>;
extern template class MANTID_KERNEL_DLL PropertyWithValue<std::vector<std::vector<long>>>;
#endif
#ifdef __linux__
extern template class MANTID_KERNEL_DLL PropertyWithValue<long long>;
extern template class MANTID_KERNEL_DLL PropertyWithValue<unsigned long long>;
extern template class MANTID_KERNEL_DLL PropertyWithValue<std::vector<long long>>;
extern template class MANTID_KERNEL_DLL PropertyWithValue<std::vector<unsigned long long>>;
extern template class MANTID_KERNEL_DLL PropertyWithValue<std::vector<std::vector<long long>>>;
#endif
// These 9 types already get an explicitly-specialized (and DLL-attributed) saveProperty()
// declared just above, in this same header. GCC treats that as already fixing the whole
// specialization's visibility, so repeating the DLL macro here triggers the same
// "attributes ignored after type is already defined" warning that Kernel/PropertyWithValue.cpp
// works around for its own (non-extern) explicit instantiations of these types.
extern template class PropertyWithValue<float>;
extern template class PropertyWithValue<double>;
extern template class PropertyWithValue<int32_t>;
extern template class PropertyWithValue<uint32_t>;
extern template class PropertyWithValue<int64_t>;
extern template class PropertyWithValue<uint64_t>;
extern template class PropertyWithValue<std::vector<double>>;
extern template class PropertyWithValue<std::vector<int32_t>>;
extern template class PropertyWithValue<std::string>;
#endif // PROPERTYWITHVALUE_PROVIDES_EXPLICIT_INSTANTIATIONS

} // namespace Kernel
} // namespace Mantid
