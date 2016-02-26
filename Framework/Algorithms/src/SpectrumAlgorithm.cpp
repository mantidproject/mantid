#include "MantidAlgorithms/SpectrumAlgorithm.h"

#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/EventWorkspace.h"

using namespace Mantid;
using namespace Algorithms;

/** Declare standard properties for defining ranges/lists of spectra. */
void SpectrumAlgorithm::declareSpectrumIndexSetProperties() {
  auto mustBePositive = boost::make_shared<Kernel::BoundedValidator<int>>();
  mustBePositive->setLower(0);
  // TODO: The names and descriptions of these properties are temporary and for
  // now follow the old naming from the algorithm ChangeBinOffset, to not break
  // its interface. This will be changed once a decision on a generic and
  // uniform interface has been made. We keep it for now, to avoid breaking that
  // algorithms interface twice.
  declareProperty("IndexMin", 0, mustBePositive,
                  "The first Workspace index to be included in the summing");
  declareProperty("IndexMax", EMPTY_INT(), mustBePositive,
                  "The last Workspace index to be included in the summing");

  declareProperty(new Kernel::ArrayProperty<size_t>("WorkspaceIndexList"),
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
 * If declareSpectrumIndexSetProperties() has been called in init(), the user
 * defined range and/or index list will be used to construct the set, otherwise
 * the set will refer to the full range of indices. The number of histograms in
 * the workspace is used for validation. Throws an error if indices are out of
 * range. */
Kernel::IndexSet SpectrumAlgorithm::getSpectrumIndexSet(
    const API::MatrixWorkspace &workspace) const {
  auto numberOfSpectra = workspace.getNumberHistograms();

  if (!existsProperty("IndexMin"))
    return {numberOfSpectra};

  int min = getProperty("IndexMin");
  int max = getProperty("IndexMax");
  std::vector<size_t> indices_list = getProperty("WorkspaceIndexList");

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
