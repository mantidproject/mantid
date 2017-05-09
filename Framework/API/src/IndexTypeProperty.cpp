#include "MantidAPI/IndexTypeProperty.h"

namespace Mantid {
namespace API {
IndexTypeProperty::IndexTypeProperty(const int indexType = 0)
    : PropertyWithValue<std::string>("IndexType", "",
                                     Kernel::Direction::Input) {
  if (indexType & IndexType::SpectrumNumber)
    m_allowedValues.push_back("SpectrumNumber");
  if (indexType & IndexType::WorkspaceIndex)
    m_allowedValues.push_back("WorkspaceIndex");

  if (m_allowedValues.size() == 0)
    throw std::invalid_argument("Argument indexType incorrectly specified");

  // If only one allowed index type we can just set the value.
  if (m_allowedValues.size() == 1)
    PropertyWithValue<std::string>::operator=(m_allowedValues[0]);
}

IndexType IndexTypeProperty::selectedType() const {
  auto val = this->value();
  if (val.compare("SpectrumNumber") == 0)
    return IndexType::SpectrumNumber;
  else if (val.compare("WorkspaceIndex") == 0)
    return IndexType::WorkspaceIndex;
  else
    throw std::runtime_error("This value cannot be used until initialised");
}

int IndexTypeProperty::allowedTypes() const {
  int types(0);
  auto beg = m_allowedValues.cbegin();
  auto end = m_allowedValues.cend();

  if (std::find(beg, end, "SpectrumNumber") != end) {
    types |= IndexType::SpectrumNumber;
  }

  if (std::find(beg, end, "WorkspaceIndex") != end) {
    types |= IndexType::WorkspaceIndex;
  }

  return types;
}

std::vector<std::string> IndexTypeProperty::allowedValues() const {
  return m_allowedValues;
}

bool IndexTypeProperty::isMultipleSelectionAllowed() { return true; }

std::string &IndexTypeProperty::operator=(const std::string &value) {
  // If there is only one allowed value, this only need be set once.
  if (m_allowedValues.size() == 1)
    return m_value;
  else
    return PropertyWithValue<std::string>::operator=(value);
}
} // namespace API
} // namespace Mantid
