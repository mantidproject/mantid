// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMuon/MuonGroupDetectors.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidHistogramData/HistogramMath.h"

namespace Mantid {
namespace Algorithms {

using namespace Kernel;
using namespace API;
using namespace DataObjects;
using namespace HistogramData;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(MuonGroupDetectors)

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string MuonGroupDetectors::name() const {
  return "MuonGroupDetectors";
}

/// Algorithm's version for identification. @see Algorithm::version
int MuonGroupDetectors::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string MuonGroupDetectors::category() const { return "Muon"; }

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void MuonGroupDetectors::init() {
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "Workspace to apply grouping to.");

  declareProperty(std::make_unique<WorkspaceProperty<TableWorkspace>>(
                      "DetectorGroupingTable", "", Direction::Input),
                  "Table with detector grouping information. Check wiki page "
                  "for table format expected.");

  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "Workspace with detectors grouped.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void MuonGroupDetectors::exec() {
  TableWorkspace_sptr table = getProperty("DetectorGroupingTable");

  // Check that table does have expected format
  if (table->columnCount() != 1)
    throw std::invalid_argument("Grouping table should have one column only");

  if (table->getColumn(0)->type() != "vector_int")
    throw std::invalid_argument("Column should be of integer vector type");

  std::vector<size_t> nonEmptyRows;        // Rows with non-empty groups
  nonEmptyRows.reserve(table->rowCount()); // Most of rows will be non-empty

  // First pass to determine how many non-empty groups we have
  for (size_t row = 0; row < table->rowCount(); ++row) {
    if (!table->cell<std::vector<int>>(row, 0).empty())
      nonEmptyRows.push_back(row);
  }

  if (nonEmptyRows.empty())
    throw std::invalid_argument(
        "Detector Grouping Table doesn't contain any non-empty groups");

  MatrixWorkspace_sptr inWS = getProperty("InputWorkspace");

  // Create output workspace with all the same parameters as an input one except
  // number of histograms
  MatrixWorkspace_sptr outWS =
      WorkspaceFactory::Instance().create(inWS, nonEmptyRows.size());

  // Compile the groups
  for (auto rowIt = nonEmptyRows.begin(); rowIt != nonEmptyRows.end();
       ++rowIt) {
    // Group index in the output workspace
    size_t groupIndex =
        static_cast<size_t>(std::distance(nonEmptyRows.begin(), rowIt));

    std::vector<int> &detectorIDs = table->cell<std::vector<int>>(*rowIt, 0);

    // Recieve detector IDs, but need workspace indices to group, so convert
    std::vector<size_t> wsIndices =
        inWS->getIndicesFromDetectorIDs(detectorIDs);

    if (wsIndices.size() != detectorIDs.size())
      throw std::invalid_argument("Some of the detector IDs were not found");

    // We will be setting them anew
    outWS->getSpectrum(groupIndex).clearDetectorIDs();

    // Using the first detector X values
    outWS->setSharedX(groupIndex, inWS->sharedX(wsIndices.front()));

    auto hist = outWS->histogram(groupIndex);
    for (auto &wsIndex : wsIndices) {
      hist += inWS->histogram(wsIndex);
      // Detectors list of the group should contain all the detectors of
      // it's
      // elements
      outWS->getSpectrum(groupIndex)
          .addDetectorIDs(inWS->getSpectrum(wsIndex).getDetectorIDs());
    }

    outWS->setHistogram(groupIndex, hist);

    outWS->getSpectrum(groupIndex)
        .setSpectrumNo(static_cast<specnum_t>(groupIndex + 1));
  }

  setProperty("OutputWorkspace", outWS);
}

} // namespace Algorithms
} // namespace Mantid
