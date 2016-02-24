#include "MantidAlgorithms/SpectrumAlgorithm.h"

#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/EventWorkspace.h"

using namespace Mantid;
using namespace Algorithms;

void SpectrumAlgorithm::declareSpectrumIndexSetProperties() {
  auto mustBePositive = boost::make_shared<Kernel::BoundedValidator<int>>();
  mustBePositive->setLower(0);
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

Kernel::SpectrumIndexSet SpectrumAlgorithm::getSpectrumIndexSet(
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

template <>
void SpectrumAlgorithm::ifEventWorkspaceClearMRU(
    const DataObjects::EventWorkspace &workspace) {
  workspace.clearMRU();
}
