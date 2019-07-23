// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/IntegrateEPP.h"

#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/MandatoryValidator.h"

namespace Mantid {
namespace Algorithms {

using Mantid::API::WorkspaceProperty;
using Mantid::Kernel::Direction;

namespace {
/// A private namespace defining property name constants.
namespace PropertyNames {
const static std::string EPP_WORKSPACE{"EPPWorkspace"};
const static std::string INPUT_WORKSPACE{"InputWorkspace"};
const static std::string OUTPUT_WORKSPACE{"OutputWorkspace"};
const static std::string WIDTH{"HalfWidthInSigmas"};
} // namespace PropertyNames
} // namespace

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(IntegrateEPP)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string IntegrateEPP::name() const { return "IntegrateEPP"; }

/// Algorithm's version for identification. @see Algorithm::version
int IntegrateEPP::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string IntegrateEPP::category() const {
  return "Arithmetic;Transforms\\Rebin";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string IntegrateEPP::summary() const {
  return "Integrate a workspace around elastic peak positions.";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void IntegrateEPP::init() {
  declareProperty(std::make_unique<WorkspaceProperty<API::MatrixWorkspace>>(
                      PropertyNames::INPUT_WORKSPACE, "", Direction::Input),
                  "A workspace to be integrated.");
  declareProperty(std::make_unique<WorkspaceProperty<API::MatrixWorkspace>>(
                      PropertyNames::OUTPUT_WORKSPACE, "", Direction::Output),
                  "An workspace containing the integrated histograms.");
  declareProperty(std::make_unique<WorkspaceProperty<API::ITableWorkspace>>(
                      PropertyNames::EPP_WORKSPACE, "", Direction::Input),
                  "Table containing information on the elastic peaks.");
  const auto mandatoryDouble =
      boost::make_shared<Kernel::MandatoryValidator<double>>();
  const auto positiveDouble =
      boost::make_shared<Kernel::BoundedValidator<double>>();
  positiveDouble->setLower(0.0);
  positiveDouble->setLowerExclusive(true);
  const auto mandatoryPositiveDouble =
      boost::make_shared<Kernel::CompositeValidator>();
  mandatoryPositiveDouble->add(mandatoryDouble);
  mandatoryPositiveDouble->add(positiveDouble);
  declareProperty(PropertyNames::WIDTH, 5.0, mandatoryPositiveDouble,
                  "Half of the integration width in multiplies of 'Sigma'.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void IntegrateEPP::exec() {
  API::MatrixWorkspace_sptr inWS = getProperty(PropertyNames::INPUT_WORKSPACE);
  API::ITableWorkspace_const_sptr eppWS =
      getProperty(PropertyNames::EPP_WORKSPACE);
  const double sigmaMultiplier = getProperty(PropertyNames::WIDTH);
  const auto indexCol = eppWS->getColumn("WorkspaceIndex");
  const auto sigmaCol = eppWS->getColumn("Sigma");
  const auto centreCol = eppWS->getColumn("PeakCentre");
  const auto statusCol = eppWS->getColumn("FitStatus");
  std::vector<double> begins(inWS->getNumberHistograms(), 0.0);
  std::vector<double> ends(begins.size(), 0.0);
  for (size_t i = 0; i < eppWS->rowCount(); ++i) {
    const auto &fitStatus = statusCol->cell<std::string>(i);
    if (fitStatus != "success") {
      continue;
    }
    const double centre = centreCol->toDouble(i);
    const double halfWidth = sigmaMultiplier * sigmaCol->toDouble(i);
    const int index = indexCol->cell<int>(i);
    if (index < 0 || static_cast<size_t>(index) >= begins.size()) {
      throw std::runtime_error(
          "The 'WorkspaceIndex' column contains an invalid value.");
    }
    begins[index] = centre - halfWidth;
    ends[index] = centre + halfWidth;
    interruption_point();
  }
  auto integrate = createChildAlgorithm("Integration", 0.0, 1.0);
  integrate->setProperty("InputWorkspace", inWS);
  const std::string outWSName = getProperty(PropertyNames::OUTPUT_WORKSPACE);
  integrate->setPropertyValue("OutputWorkspace", outWSName);
  integrate->setProperty("RangeLowerList", begins);
  integrate->setProperty("RangeUpperList", ends);
  integrate->setProperty("IncludePartialBins", true);
  integrate->executeAsChildAlg();
  API::MatrixWorkspace_sptr outWS = integrate->getProperty("OutputWorkspace");
  setProperty(PropertyNames::OUTPUT_WORKSPACE, outWS);
}

/** Validate input properties.
 *
 * @return A map mapping input property names to discovered issues.
 */
std::map<std::string, std::string> IntegrateEPP::validateInputs() {
  std::map<std::string, std::string> issues;
  API::MatrixWorkspace_const_sptr inWS =
      getProperty(PropertyNames::INPUT_WORKSPACE);
  API::ITableWorkspace_const_sptr eppWS =
      getProperty(PropertyNames::EPP_WORKSPACE);
  if (eppWS->rowCount() > inWS->getNumberHistograms()) {
    issues[PropertyNames::EPP_WORKSPACE] =
        "The EPP workspace contains too many rows.";
  }
  return issues;
}

} // namespace Algorithms
} // namespace Mantid
