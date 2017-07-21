#include "MantidAPI/IndexProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidIndexing/GlobalSpectrumIndex.h"
#include "MantidIndexing/IndexInfo.h"
#include "MantidIndexing/SpectrumNumber.h"

#include "MantidKernel/ArrayProperty.tcc"

namespace Mantid {
namespace API {
IndexProperty::IndexProperty(const std::string &name,
                             const IWorkspaceProperty &workspaceProp,
                             const IndexTypeProperty &indexTypeProp,
                             Kernel::IValidator_sptr validator)
    : ArrayProperty<int>(name, "", validator), m_workspaceProp(workspaceProp),
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

  auto list = value();
  if ((std::count(list.cbegin(), list.cend(), ':') == 1 ||
       std::count(list.cbegin(), list.cend(), '-') == 1) &&
      std::count(list.cbegin(), list.cend(), ',') == 0) {
    auto pos = list.find(":");
    pos = (pos == std::string::npos) ? list.find("-") : pos;
    auto min = stoi(list.substr(0, pos));
    auto max = stoi(list.substr(pos + 1, list.size() - 1));

    switch (type) {
    case IndexType::WorkspaceIndex:
      return indexInfo.makeIndexSet(
          static_cast<Indexing::GlobalSpectrumIndex>(min),
          static_cast<Indexing::GlobalSpectrumIndex>(max));
    case IndexType::SpectrumNum:
      return indexInfo.makeIndexSet(static_cast<Indexing::SpectrumNumber>(min),
                                    static_cast<Indexing::SpectrumNumber>(max));
    }
  } else if (!m_value.empty()) {
    switch (type) {
    case IndexType::WorkspaceIndex:
      return indexInfo.makeIndexSet(std::vector<Indexing::GlobalSpectrumIndex>(
          m_value.begin(), m_value.end()));
    case IndexType::SpectrumNum:
      return indexInfo.makeIndexSet(std::vector<Indexing::SpectrumNumber>(
          m_value.begin(), m_value.end()));
    }
  } else {
    return indexInfo.makeIndexSet();
  }

  m_indicesExtracted = true;
  return m_indices;
}
} // namespace API
} // namespace Mantid
