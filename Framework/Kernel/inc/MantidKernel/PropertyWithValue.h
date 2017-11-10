#ifndef MANTID_KERNEL_PROPERTYWITHVALUE_H_
#define MANTID_KERNEL_PROPERTYWITHVALUE_H_

#include "MantidKernel/Property.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/NullValidator.h"

#include <vector>

namespace NeXus {
class File;
}

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

    Copyright &copy; 2007-2010 ISIS Rutherford Appleton Laboratory, NScD Oak
   Ridge National Laboratory & European Spallation Source

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://github.com/mantidproject/mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
template <typename TYPE> class DLLExport PropertyWithValue : public Property {
public:
  PropertyWithValue(
      const std::string &name, const TYPE &defaultValue,
      IValidator_sptr validator = IValidator_sptr(new NullValidator),
      const unsigned int direction = Direction::Input);
  PropertyWithValue(const std::string &name, const TYPE &defaultValue,
                    const unsigned int direction);
  PropertyWithValue(const std::string &name, const TYPE &defaultValue,
                    const std::string defaultValueStr,
                    IValidator_sptr validator, const unsigned int direction);
  PropertyWithValue(const PropertyWithValue &right);
  PropertyWithValue<TYPE> *clone() const override;

  void saveProperty(::NeXus::File *file) override;
  std::string value() const override;
  std::string valueAsPrettyStr(const size_t maxLength = 0,
                               const bool collapseLists = true) const override;
  virtual bool operator==(const PropertyWithValue<TYPE> &rhs) const;
  virtual bool operator!=(const PropertyWithValue<TYPE> &rhs) const;
  int size() const override;
  std::string getDefault() const override;
  std::string setValue(const std::string &value) override;
  std::string setDataItem(const boost::shared_ptr<DataItem> data) override;
  PropertyWithValue &operator=(const PropertyWithValue &right);
  PropertyWithValue &operator+=(Property const *right) override;
  virtual TYPE &operator=(const TYPE &value);
  virtual const TYPE &operator()() const;
  virtual operator const TYPE &() const;
  std::string isValid() const override;
  bool isDefault() const override;
  std::vector<std::string> allowedValues() const override;
  bool isMultipleSelectionAllowed() override;
  virtual void replaceValidator(IValidator_sptr newValidator);

protected:
  /// The value of the property
  TYPE m_value;
  /// the property's default value which is also its initial value
  // const TYPE m_initialValue;
  TYPE m_initialValue;

private:
  std::string setValueFromProperty(const Property &right) override;

  template <typename U>
  std::string setTypedValue(const U &value, const boost::true_type &);

  template <typename U>
  std::string setTypedValue(const U &value, const boost::false_type &);

  const TYPE getValueForAlias(const TYPE &alias) const;

  /// Visitor validator class
  IValidator_sptr m_validator;

  /// Static reference to the logger class
  static Logger g_logger;

  /// Private default constructor
  PropertyWithValue() = default;
};

template <>
MANTID_KERNEL_DLL void
PropertyWithValue<float>::saveProperty(::NeXus::File *file);
template <>
MANTID_KERNEL_DLL void
PropertyWithValue<double>::saveProperty(::NeXus::File *file);
template <>
MANTID_KERNEL_DLL void
PropertyWithValue<int32_t>::saveProperty(::NeXus::File *file);
template <>
MANTID_KERNEL_DLL void
PropertyWithValue<uint32_t>::saveProperty(::NeXus::File *file);
template <>
MANTID_KERNEL_DLL void
PropertyWithValue<int64_t>::saveProperty(::NeXus::File *file);
template <>
MANTID_KERNEL_DLL void
PropertyWithValue<uint64_t>::saveProperty(::NeXus::File *file);
template <>
MANTID_KERNEL_DLL void
PropertyWithValue<std::string>::saveProperty(::NeXus::File *file);
template <>
MANTID_KERNEL_DLL void
PropertyWithValue<std::vector<double>>::saveProperty(::NeXus::File *file);
template <>
MANTID_KERNEL_DLL void
PropertyWithValue<std::vector<int32_t>>::saveProperty(::NeXus::File *file);

template <typename TYPE>
Logger PropertyWithValue<TYPE>::g_logger("PropertyWithValue");

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_PROPERTYWITHVALUE_H_*/
