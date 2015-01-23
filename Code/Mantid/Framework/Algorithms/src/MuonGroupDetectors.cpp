#include "MantidAlgorithms/MuonGroupDetectors.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/TableWorkspace.h"

namespace Mantid {
namespace Algorithms {

using namespace Kernel;
using namespace API;
using namespace DataObjects;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(MuonGroupDetectors)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
MuonGroupDetectors::MuonGroupDetectors() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
MuonGroupDetectors::~MuonGroupDetectors() {}

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string MuonGroupDetectors::name() const {
  return "MuonGroupDetectors";
};

/// Algorithm's version for identification. @see Algorithm::version
int MuonGroupDetectors::version() const { return 1; };

/// Algorithm's category for identification. @see Algorithm::category
const std::string MuonGroupDetectors::category() const { return "Muon"; }

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void MuonGroupDetectors::init() {
  declareProperty(new WorkspaceProperty<MatrixWorkspace>("InputWorkspace", "",
                                                         Direction::Input),
                  "Workspace to apply grouping to.");

  declareProperty(new WorkspaceProperty<TableWorkspace>("DetectorGroupingTable",
                                                        "", Direction::Input),
                  "Table with detector grouping information. Check wiki page "
                  "for table format expected.");

  declareProperty(new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace", "",
                                                         Direction::Output),
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
    if (table->cell<std::vector<int>>(row, 0).size() != 0)
      nonEmptyRows.push_back(row);
  }

  if (nonEmptyRows.size() == 0)
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
    std::vector<size_t> wsIndices;
    inWS->getIndicesFromDetectorIDs(detectorIDs, wsIndices);

    if (wsIndices.size() != detectorIDs.size())
      throw std::invalid_argument("Some of the detector IDs were not found");

    // We will be setting them anew
    outWS->getSpectrum(groupIndex)->clearDetectorIDs();

    for (auto detIt = wsIndices.begin(); detIt != wsIndices.end(); detIt++) {
      for (size_t i = 0; i < inWS->blocksize(); ++i) {
        // Sum the y values
        outWS->dataY(groupIndex)[i] += inWS->dataY(*detIt)[i];

        // Sum the errors in quadrature
        outWS->dataE(groupIndex)[i] = sqrt(pow(outWS->dataE(groupIndex)[i], 2) +
                                           pow(inWS->dataE(*detIt)[i], 2));
      }

      // Detectors list of the group should contain all the detectors of it's
      // elements
      outWS->getSpectrum(groupIndex)
          ->addDetectorIDs(inWS->getSpectrum(*detIt)->getDetectorIDs());
    }

    // Using the first detector X values
    outWS->dataX(groupIndex) = inWS->dataX(wsIndices.front());

    outWS->getSpectrum(groupIndex)
        ->setSpectrumNo(static_cast<specid_t>(groupIndex + 1));
  }

  setProperty("OutputWorkspace", outWS);
}

} // namespace Algorithms
} // namespace Mantid
