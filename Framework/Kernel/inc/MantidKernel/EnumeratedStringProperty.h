// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "MantidKernel/EnumeratedString.h"
#include "MantidKernel/NullValidator.h"
#include "MantidKernel/Property.h"
#include <vector>

namespace NeXus {
class File;
}

namespace Mantid {

namespace Kernel {
/** A concrete property based on user options of a finite list of strings.
 *  Allows for easy comparison by binding the string list to an enum.

    @class Mantid::Kernel::EnumeratedStringProperty

    @author Reece Boston, ORNL
    @date October 1, in this the two-thousand-and-twenty-fourth year of our Lord
*/
template <class E, std::vector<std::string> const *const names> class EnumeratedStringProperty : public Property {

  using ENUMSTRING = EnumeratedString<E, names>;

public:
  // CONSTRUCTORS
  EnumeratedStringProperty(std::string const &name, ENUMSTRING const &defaultValue = static_cast<E>(0),
                           Direction::Type const direction = Direction::Input);
  EnumeratedStringProperty(EnumeratedStringProperty const &right);
  EnumeratedStringProperty *clone() const override;
  EnumeratedStringProperty &operator=(EnumeratedStringProperty const &right);

  // GETTERS
  std::string value() const override;
  std::string valueAsPrettyStr(size_t const maxLength = 0, bool const collapseLists = true) const override;
  Json::Value valueAsJson() const override;

  bool operator==(EnumeratedStringProperty const &rhs) const;
  bool operator!=(EnumeratedStringProperty const &rhs) const;
  int size() const override;
  std::string getDefault() const override;

  /** Allows you to get the value of the property simply by typing its name.
   *  Means you can use an expression like: int i = myProperty;
   * @return the value
   */
  operator ENUMSTRING() const { return this->m_value; };
  operator E() const { return static_cast<E>(m_value); };
  operator std::string() const { return static_cast<std::string>(m_value); };

  ENUMSTRING operator()() const;
  std::string isValid() const override;
  bool isDefault() const override;
  std::vector<std::string> allowedValues() const override;
  bool isMultipleSelectionAllowed() override;

  // SETTERS
  std::string setValue(E const value);
  std::string setValue(std::string const &value) override;
  std::string setValue(ENUMSTRING const &value);
  std::string setValueFromJson(const Json::Value &value) override;
  std::string setDataItem(const std::shared_ptr<DataItem> &data) override;
  EnumeratedStringProperty const &operator=(E const value);
  EnumeratedStringProperty const &operator=(std::string const &value);
  EnumeratedStringProperty const &operator=(ENUMSTRING const &value);

  // MUTATORS AND SUNDRY
  EnumeratedStringProperty &operator+=(Property const *right) override;
  void saveProperty(::NeXus::File *file) override;

protected:
  /// The value of the property
  ENUMSTRING m_value;
  /// the property's default value which is also its initial value
  ENUMSTRING m_initialValue;

private:
  std::string setValueFromProperty(Property const &right) override;

  template <typename U> std::string setTypedValue(U const &value, std::true_type const &);

  template <typename U> std::string setTypedValue(U const &value, std::false_type const &);
};

// template <typename TYPE> Logger PropertyWithValue<TYPE>::g_logger("PropertyWithValue");

} // namespace Kernel
} // namespace Mantid

#include "EnumeratedStringProperty.hxx"
