#include "MantidAPI/IndexTypeProperty.h"

namespace Mantid {
namespace API {
IndexTypeProperty::IndexTypeProperty(const std::string &name,
                                     const int indexType)
    : PropertyWithValue<std::string>(name, "", Kernel::Direction::Input) {
  if (indexType & IndexType::WorkspaceIndex)
    m_allowedValues.push_back("WorkspaceIndex");
  if (indexType & IndexType::SpectrumNum)
    m_allowedValues.push_back("SpectrumNumber");

  if (m_allowedValues.empty())
    throw std::invalid_argument("Argument indexType incorrectly specified");

  m_value = m_allowedValues[0];
}

IndexType IndexTypeProperty::selectedType() const {
  auto val = this->value();
  if (val == "SpectrumNumber")
    return IndexType::SpectrumNum;
  else if (val == "WorkspaceIndex")
    return IndexType::WorkspaceIndex;
  else if (val.empty())
    throw std::runtime_error("This value cannot be used until initialised");
  else
    throw std::runtime_error(val + " is an invalid IndexType.");
}

int IndexTypeProperty::allowedTypes() const {
  int types(0);
  const auto beg = m_allowedValues.cbegin();
  const auto end = m_allowedValues.cend();

  if (std::find(beg, end, "SpectrumNumber") != end) {
    types |= IndexType::SpectrumNum;
  }

  if (std::find(beg, end, "WorkspaceIndex") != end) {
    types |= IndexType::WorkspaceIndex;
  }

  return types;
}

std::vector<std::string> IndexTypeProperty::allowedValues() const {
  return m_allowedValues;
}

bool IndexTypeProperty::isMultipleSelectionAllowed() { return false; }

IndexTypeProperty &IndexTypeProperty::operator=(API::IndexType type) {
  std::string val;

  switch (type) {
  case IndexType::SpectrumNum:
    val = "SpectrumNumber";
    break;
  case IndexType::WorkspaceIndex:
    val = "WorkspaceIndex";
    break;
  }

  *this = val;
  return *this;
}

std::string IndexTypeProperty::generatePropertyName(const std::string &name) {
  return name + "IndexType";
}

} // namespace API
} // namespace Mantid
