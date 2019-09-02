// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadISISPolarizationEfficiencies.h"

#include "MantidAPI/FileProperty.h"
#include "MantidAPI/TextAxis.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidHistogramData/Interpolate.h"

#include <limits>

namespace Mantid {
namespace DataHandling {

using namespace Mantid::Kernel;
using namespace Mantid::API;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(LoadISISPolarizationEfficiencies)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string LoadISISPolarizationEfficiencies::name() const {
  return "LoadISISPolarizationEfficiencies";
}

/// Algorithm's version for identification. @see Algorithm::version
int LoadISISPolarizationEfficiencies::version() const { return 1; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string LoadISISPolarizationEfficiencies::summary() const {
  return "Loads ISIS reflectometry polarization efficiency factors from files: "
         "one factor per file.";
}

const std::vector<std::string>
LoadISISPolarizationEfficiencies::seeAlso() const {
  return {"CreatePolarizationEfficiencies", "JoinISISPolarizationEfficiencies",
          "PolarizationEfficiencyCor"};
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void LoadISISPolarizationEfficiencies::init() {
  declareProperty(std::make_unique<API::FileProperty>(
                      Pp, "", API::FileProperty::OptionalLoad),
                  "Path to the file containing the Pp polarization efficiency "
                  "in XYE columns.");

  declareProperty(std::make_unique<API::FileProperty>(
                      Ap, "", API::FileProperty::OptionalLoad),
                  "Path to the file containing the Ap polarization efficiency "
                  "in XYE columns.");

  declareProperty(std::make_unique<API::FileProperty>(
                      Rho, "", API::FileProperty::OptionalLoad),
                  "Path to the file containing the Rho polarization efficiency "
                  "in XYE columns.");

  declareProperty(
      std::make_unique<API::FileProperty>(Alpha, "",
                                          API::FileProperty::OptionalLoad),
      "Path to the file containing the Alpha polarization efficiency "
      "in XYE columns.");

  declareProperty(std::make_unique<API::FileProperty>(
                      P1, "", API::FileProperty::OptionalLoad),
                  "Path to the file containing the P1 polarization efficiency "
                  "in XYE columns.");

  declareProperty(std::make_unique<API::FileProperty>(
                      P2, "", API::FileProperty::OptionalLoad),
                  "Path to the file containing the P2 polarization efficiency "
                  "in XYE columns.");

  declareProperty(std::make_unique<API::FileProperty>(
                      F1, "", API::FileProperty::OptionalLoad),
                  "Path to the file containing the F1 polarization efficiency "
                  "in XYE columns.");

  declareProperty(std::make_unique<API::FileProperty>(
                      F2, "", API::FileProperty::OptionalLoad),
                  "Path to the file containing the F2 polarization efficiency "
                  "in XYE columns.");

  initOutputWorkspace();
}

/// Load efficiencies from files and put them into a single workspace.
/// @param props :: Names of properties containg names of files to load.
MatrixWorkspace_sptr LoadISISPolarizationEfficiencies::createEfficiencies(
    std::vector<std::string> const &props) {

  auto alg = createChildAlgorithm("JoinISISPolarizationEfficiencies");
  alg->initialize();
  for (auto const &propName : props) {
    auto loader = createChildAlgorithm("Load");
    loader->initialize();
    loader->setPropertyValue("Filename", getPropertyValue(propName));
    loader->execute();
    Workspace_sptr output = loader->getProperty("OutputWorkspace");
    auto ws = boost::dynamic_pointer_cast<MatrixWorkspace>(output);
    if (!ws) {
      throw std::invalid_argument("File " + propName +
                                  " cannot be loaded into a MatrixWorkspace.");
    }
    if (ws->getNumberHistograms() != 1) {
      throw std::runtime_error(
          "Loaded workspace must contain a single histogram. Found " +
          std::to_string(ws->getNumberHistograms()));
    }
    alg->setProperty(propName, ws);
  }
  alg->execute();
  MatrixWorkspace_sptr outWS = alg->getProperty("OutputWorkspace");
  return outWS;
}

} // namespace DataHandling
} // namespace Mantid
