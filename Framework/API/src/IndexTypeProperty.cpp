#include "MantidAPI/IndexTypeProperty.h"

namespace Mantid {
namespace API {
IndexTypeProperty::IndexTypeProperty(const int indexType = 0)
    : PropertyWithValue<std::string>("IndexType", "",
                                     Kernel::Direction::Input) {
  if (indexType & IndexType::WorkspaceIndex)
    m_allowedValues.push_back("WorkspaceIndex");
  if (indexType & IndexType::SpectrumNumber)
    m_allowedValues.push_back("SpectrumNumber");

  if (m_allowedValues.size() == 0)
    throw std::invalid_argument("Argument indexType incorrectly specified");

  m_value = m_allowedValues[0];
}

IndexType IndexTypeProperty::selectedType() const {
  auto val = this->value();
  if (val.compare("SpectrumNumber") == 0)
	  return IndexType::SpectrumNumber;
  else if (val.compare("WorkspaceIndex") == 0)
	  return IndexType::WorkspaceIndex;
  else if (val.empty())
	  throw std::runtime_error("This value cannot be used until initialised");
  else
	  throw std::runtime_error(val + " is an invalid IndexType.");
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

bool IndexTypeProperty::isMultipleSelectionAllowed() { return false; }

std::string &IndexTypeProperty::operator=(API::IndexType type) {
  std::string val;

  switch (type) {
  case IndexType::SpectrumNumber:
    val = "SpectrumNumber";
    break;
  case IndexType::WorkspaceIndex:
    val = "WorkspaceIndex";
    break;
  }

  return *this = val;
}

} // namespace API
} // namespace Mantid
