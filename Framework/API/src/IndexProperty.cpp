#include "MantidAPI/IndexProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidIndexing/GlobalSpectrumIndex.h"
#include "MantidIndexing/IndexInfo.h"
#include "MantidIndexing/SpectrumNumber.h"

namespace Mantid {
namespace API {
IndexProperty::IndexProperty(const std::string &name,
                             const IWorkspaceProperty &workspaceProp,
                             const IndexTypeProperty &indexTypeProp,
                             Kernel::IValidator_sptr validator)
    : ArrayProperty(name, "", validator), m_workspaceProp(workspaceProp),
      m_indexTypeProp(indexTypeProp), m_indices(0), m_indicesExtracted(false) {}

IndexProperty *IndexProperty::clone() const { return new IndexProperty(*this); }

bool IndexProperty::isDefault() const { return m_value.empty(); }

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

  return error;
}

std::string IndexProperty::operator=(const std::string &rhs) {
  return setValue(rhs);
}

Indexing::SpectrumIndexSet IndexProperty::getIndices() const {
  MatrixWorkspace_sptr wksp;
  if ((wksp = boost::dynamic_pointer_cast<MatrixWorkspace>(
           m_workspaceProp.getWorkspace())) == nullptr)
    throw std::runtime_error("Invalid workspace type provided to "
                             "IndexProperty. Must be convertible to "
                             "MatrixWorkspace.");

  const auto &indexInfo = wksp->indexInfo();
  auto type = m_indexTypeProp.selectedType();

  if (m_value.empty()) {
    return indexInfo.makeIndexSet();
  } else {
    auto res = std::minmax_element(m_value.cbegin(), m_value.cend());
    auto min = static_cast<size_t>(*res.first);
    auto max = static_cast<size_t>(*res.second);

    auto isRange = (max - min) == (m_value.size() - 1);

    if (isRange) {
      switch (type) {
      case IndexType::WorkspaceIndex:
        return indexInfo.makeIndexSet(
            static_cast<Indexing::GlobalSpectrumIndex>(min),
            static_cast<Indexing::GlobalSpectrumIndex>(max));
      case IndexType::SpectrumNum:
        return indexInfo.makeIndexSet(
            static_cast<Indexing::SpectrumNumber>(min),
            static_cast<Indexing::SpectrumNumber>(max));
      }
    } else {
      switch (type) {
      case IndexType::WorkspaceIndex:
        return indexInfo.makeIndexSet(
            std::vector<Indexing::GlobalSpectrumIndex>(m_value.begin(),
                                                       m_value.end()));
      case IndexType::SpectrumNum:
        return indexInfo.makeIndexSet(std::vector<Indexing::SpectrumNumber>(
            m_value.begin(), m_value.end()));
      }
    }
  }

  m_indicesExtracted = true;
  return m_indices;
}
} // namespace API
} // namespace Mantid
