// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/SpectrumAlgorithm.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"

using namespace Mantid;
using namespace Algorithms;

/** Declare standard properties for defining ranges/lists of spectra. */
void SpectrumAlgorithm::declareWorkspaceIndexSetProperties(
    const std::string &indexMinPropertyName,
    const std::string &indexMaxPropertyName,
    const std::string &indexRangePropertyName) {
  m_indexMinPropertyName = indexMinPropertyName;
  m_indexMaxPropertyName = indexMaxPropertyName;
  m_indexRangePropertyName = indexRangePropertyName;
  auto mustBePositive = boost::make_shared<Kernel::BoundedValidator<int>>();
  mustBePositive->setLower(0);
  // TODO: The names and descriptions of these properties are temporary and for
  // now follow the old naming from the algorithm ChangeBinOffset, to not break
  // its interface. This will be changed once a decision on a generic and
  // uniform interface has been made. We keep it for now, to avoid breaking that
  // algorithms interface twice.
  declareProperty(m_indexMinPropertyName, 0, mustBePositive,
                  "The first Workspace index to be included in the summing");
  declareProperty(m_indexMaxPropertyName, EMPTY_INT(), mustBePositive,
                  "The last Workspace index to be included in the summing");

  declareProperty(
      std::make_unique<Kernel::ArrayProperty<size_t>>(m_indexRangePropertyName),
      "A list of workspace indices as a string with ranges, for "
      "example: 5-10,15,20-23. \n"
      "Optional: if not specified, then the "
      "Start/EndWorkspaceIndex fields are used alone. "
      "If specified, the range and the list are combined (without "
      "duplicating indices). For example, a range of 10 to 20 and "
      "a list '12,15,26,28' gives '10-20,26,28'.");
}

/** Returns a validated IndexSet refering to spectra in workspace.
 *
 * If declareSpectrumWorkspaceProperties() has been called in init(), the user
 * defined range and/or index list will be used to construct the set, otherwise
 * the set will refer to the full range of indices. The number of histograms in
 * the workspace is used for validation. Throws an error if indices are out of
 * range. */
Kernel::IndexSet SpectrumAlgorithm::getWorkspaceIndexSet(
    const API::MatrixWorkspace &workspace) const {
  auto numberOfSpectra = workspace.getNumberHistograms();

  if (!existsProperty(m_indexMinPropertyName))
    return {numberOfSpectra};

  int min = getProperty(m_indexMinPropertyName);
  int max = getProperty(m_indexMaxPropertyName);
  std::vector<size_t> indices_list = getProperty(m_indexRangePropertyName);

  if (indices_list.empty()) {
    if (isEmpty(max))
      return {numberOfSpectra};
    else
      return {min, max, numberOfSpectra};
  }

  // Add range to index list if given.
  if (!isEmpty(max)) {
    for (int i = min; i <= max; i++)
      indices_list.push_back(i);
  }
  return {indices_list, numberOfSpectra};
}

/** Internal part of the for_each() implementation.
 *
 * This specialization is used to call clearMRU for EventWorkspace, overriding
 *the default implementation that does nothing. */
template <>
void SpectrumAlgorithm::ifEventWorkspaceClearMRU(
    const DataObjects::EventWorkspace &workspace) {
  workspace.clearMRU();
}
