// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "MantidKernel/DllConfig.h"
#include "MantidKernel/EnumeratedString.h"
#include "MantidKernel/Property.h"
#include "MantidKernel/PropertyWithValue.h"
#include <string>
#include <vector>

namespace Mantid {
namespace Kernel {

// Define the EnumStringProperty with simpler syntax
template <typename T, const std::vector<std::string> *Names>
using SimpleEnumStringPropertyBase = PropertyWithValue<EnumeratedString<T, Names>>;

template <typename T, const std::vector<std::string> *Names>
class DLLExport EnumStringProperty : public SimpleEnumStringPropertyBase<T, Names> {

public:
  // constructors
  EnumStringProperty();
  /*EnumStringProperty(const std::string &name, std::vector<T> vec,
                const unsigned int direction = Direction::Input);
  EnumStringProperty(const std::string &name,
                const unsigned int direction = Direction::Input);
  EnumStringProperty(const std::string &name, const unsigned int direction = Direction::Input);
  EnumStringProperty(const std::string &name, const std::string &values,
                const IValidator_sptr &validator = IValidator_sptr(new NullValidator),
                const unsigned int direction = Direction::Input);

  ArrayProperty(const ArrayProperty &);

  ArrayProperty<T> *clone() const override;*/
  std::string value() override;

  std::string setValue(const std::string &value) override;
};

/** EnumStringProperty : TODO: DESCRIPTION
 */
} // namespace Kernel
} // namespace Mantid
