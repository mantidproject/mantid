// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
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
  // No workspace acts as an empty set and so is considered valid
  if (!m_workspaceProp.getWorkspace())
    return error;

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

IndexProperty &IndexProperty::operator=(const std::string &rhs) {
  setValue(rhs);
  return *this;
}

IndexProperty::operator Indexing::SpectrumIndexSet() const {
  return getIndices();
}

Indexing::SpectrumIndexSet IndexProperty::getIndices() const {
  const auto &indexInfo = getIndexInfoFromWorkspace();
  auto type = m_indexTypeProp.selectedType();

  if (m_value.empty()) {
    return indexInfo.makeIndexSet();
  } else {
    auto min = m_value.front();
    auto max = m_value.back();
    auto isRange = (max - min) == static_cast<int>(m_value.size() - 1);
    if (isRange) {
      switch (type) {
      case IndexType::WorkspaceIndex:
        return indexInfo.makeIndexSet(
            static_cast<Indexing::GlobalSpectrumIndex>(min),
            static_cast<Indexing::GlobalSpectrumIndex>(max));
      case IndexType::SpectrumNum:
        return indexInfo.makeIndexSet(
            static_cast<Indexing::SpectrumNumber>(static_cast<int32_t>(min)),
            static_cast<Indexing::SpectrumNumber>(static_cast<int32_t>(max)));
      }
    } else {
      switch (type) {
      case IndexType::WorkspaceIndex:
        return indexInfo.makeIndexSet(
            std::vector<Indexing::GlobalSpectrumIndex>(m_value.begin(),
                                                       m_value.end()));
      case IndexType::SpectrumNum: {
        std::vector<Indexing::SpectrumNumber> spectrumNumbers(m_value.cbegin(),
                                                              m_value.cend());
        return indexInfo.makeIndexSet(spectrumNumbers);
      }
      }
    }
  }

  m_indicesExtracted = true;
  return m_indices;
}

/** Return IndexInfo created from workspace but containing selected spectra.
 *
 * The selected spectra are the same as in the SpectrumIndexSet returned by this
 * property and the order is guaranteed to be consistent. That is, if the Nth
 * entry in the SpectrumIndexSet is M, the spectrum with index M in the input
 * workspace is equal to the spectrum with index N in the returned IndexInfo. */
Indexing::IndexInfo IndexProperty::getFilteredIndexInfo() const {
  const auto &indexInfo = getIndexInfoFromWorkspace();
  if (m_value.empty())
    return indexInfo;
  switch (m_indexTypeProp.selectedType()) {
  case IndexType::WorkspaceIndex:
    return {std::vector<Indexing::GlobalSpectrumIndex>(m_value.begin(),
                                                       m_value.end()),
            indexInfo};
  case IndexType::SpectrumNum: {
    std::vector<Indexing::SpectrumNumber> spectrumNumbers(m_value.cbegin(),
                                                          m_value.cend());
    return {spectrumNumbers, indexInfo};
  }
  default:
    throw std::runtime_error(
        "IndexProperty::getFilteredIndexInfo -- unsupported index type");
  }
}

std::string IndexProperty::generatePropertyName(const std::string &name) {
  return name + "IndexSet";
}

const Indexing::IndexInfo &IndexProperty::getIndexInfoFromWorkspace() const {
  auto wksp = boost::dynamic_pointer_cast<MatrixWorkspace>(
      m_workspaceProp.getWorkspace());
  if (!wksp)
    throw std::runtime_error("Invalid workspace type provided to "
                             "IndexProperty. Must be convertible to "
                             "MatrixWorkspace.");
  return wksp->indexInfo();
}

} // namespace API
} // namespace Mantid
