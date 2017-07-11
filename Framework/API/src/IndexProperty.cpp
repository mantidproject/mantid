#include "MantidAPI/IndexProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidIndexing/GlobalSpectrumIndex.h"
#include "MantidIndexing/IndexInfo.h"
#include "MantidIndexing/SpectrumNumber.h"


namespace Mantid {
using namespace Kernel;

namespace API {
IndexProperty::IndexProperty(const std::string &name,
                             const IWorkspaceProperty &workspaceProp,
                             const IndexTypeProperty &indexTypeProp,
                             Kernel::IValidator_sptr validator)
    : ArrayProperty<int>(name, "", validator), m_workspaceProp(workspaceProp),
      m_indexTypeProp(indexTypeProp), m_min(0), m_max(0), m_indices(0),
      m_indicesExtracted(false) {}

IndexProperty *IndexProperty::clone() const { return new IndexProperty(*this); }

bool IndexProperty::isDefault() const {
  // The default value is an empty vector/string.
  return value().empty();
}

std::string IndexProperty::isValid() const {
  std::string error;

  try {
    getIndices();
  } catch (std::runtime_error &e) {
    error = e.what();
  } catch (std::out_of_range &) {
    error = "Indices provided to IndexProperty are out of range.";
  } catch (std::logic_error &) {
    error = "Duplicate indices supplied to IndexProperty.";
  }

  if (!m_validString.empty())
    error += " " + m_validString;

  return error;
}

std::string IndexProperty::operator=(const std::string &rhs) {
  return setValue(rhs);
}

std::vector<int> &IndexProperty::operator=(const std::vector<int> &rhs) {
  m_indicesExtracted = false;
  m_min = m_max = 0;
  m_validString = "";

  // Instead of just copying vector, determine if we have a pure range.
  if (rhs.size() > 0) {
    auto res = std::minmax_element(rhs.cbegin(), rhs.cend());
    auto minIndex = res.first - rhs.begin();
    auto maxIndex = res.second - rhs.begin();
    bool isPureRange =
        static_cast<size_t>((rhs[maxIndex] - rhs[minIndex] + 1)) == rhs.size();

    if (isPureRange) {
      m_min = rhs[0];
      m_max = rhs[rhs.size() - 1];
      m_value = std::vector<int>();
    } else
      m_value = rhs;
  }

  return m_value;
}

std::string IndexProperty::setValue(const std::string &value) {
  m_indicesExtracted = false;
  m_min = m_max = 0;

  // Check whether or not string value is simply a range
  if ((std::count(value.cbegin(), value.cend(), ':') == 1 ||
       std::count(value.cbegin(), value.cend(), '-') == 1) &&
      std::count(value.cbegin(), value.cend(), ',') == 0) {
    auto pos = value.find(":");
    pos = pos == std::string::npos ? value.find("-") : pos;
    auto min = value.substr(0, pos);
    auto max = value.substr(pos + 1, value.size() - 1);

    std::string err = "";

    try {
      m_min = stoi(min);
      m_max = stoi(max);
    } catch (std::invalid_argument &) {
      err += value + " could not be converted to integer list.";
    }

    if (!err.empty())
      return m_validString = err;

    m_value = std::vector<int>();

    if (m_min >= m_max)
      err += " min should be less than max";
    if (m_min < 0 || m_max < 0)
      err += " min and max values should be greater than zero";

    return m_validString = err;
  } else {
    return m_validString = ArrayProperty<int>::setValue(value);
  }
}

std::string IndexProperty::value() const {
  if (m_min != m_max)
    return std::to_string(m_min) + ":" + std::to_string(m_max);
  else
    return ArrayProperty<int>::value();
}

Indexing::SpectrumIndexSet IndexProperty::getIndices() const {
  if (m_indicesExtracted)
    return m_indices;

  auto wksp = boost::dynamic_pointer_cast<MatrixWorkspace>(
      m_workspaceProp.getWorkspace());

  if (wksp == nullptr)
    throw std::runtime_error("Invalid workspace type provided to "
                             "IndexProperty. Must be convertible to "
                             "MatrixWorkspace.");

  const auto &indexInfo = wksp->indexInfo();
  auto type = m_indexTypeProp.selectedType();

  if (m_min != m_max) {
    switch (type) {
    case IndexType::WorkspaceIndex:
      return m_indices = indexInfo.makeIndexSet(
                 static_cast<Indexing::GlobalSpectrumIndex>(m_min),
                 static_cast<Indexing::GlobalSpectrumIndex>(m_max));
    case IndexType::SpectrumNum:
      return m_indices = indexInfo.makeIndexSet(
                 static_cast<Indexing::SpectrumNumber>(m_min),
                 static_cast<Indexing::SpectrumNumber>(m_max));
    }
  } else if (m_value.empty()) {
    return m_indices = indexInfo.makeIndexSet();
  } else {
    switch (type) {
    case IndexType::WorkspaceIndex:
      return m_indices = indexInfo.makeIndexSet(
                 std::vector<Indexing::GlobalSpectrumIndex>(m_value.begin(),
                                                            m_value.end()));
    case IndexType::SpectrumNum:
      return m_indices =
                 indexInfo.makeIndexSet(std::vector<Indexing::SpectrumNumber>(
                     m_value.begin(), m_value.end()));
    }
  }

  m_indicesExtracted = true;
  return m_indices;
}
} // namespace API
} // namespace Mantid
