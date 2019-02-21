// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAlgorithms/ReflectometryBackgroundSubtraction.h"
#include "MantidAPI/CommonBinsValidator.h"
#include "MantidKernel/EnabledWhenProperty.h"

namespace Mantid {
namespace Algorithms {

/// Algorithm's name for identification. @see Algorithm::name
const std::string ReflectometryBackgroundSubtraction::name() const {
  return "ReflectometryBackgroundSubtraction";
}

/// Algorithm's version for identification. @see Algorithm::version
int ReflectometryBackgroundSubtraction::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string ReflectometryBackgroundSubtraction::category() const {
  return "Reflectometry;Reflectometry\\ISIS";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string ReflectometryBackgroundSubtraction::summary() const {
  return "";
}

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ReflectometryBackgroundSubtraction)

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void ReflectometryBackgroundSubtraction::init() {

  // Input workspace
  declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "InputWorkspace", "", Direction::Input,
                      boost::make_shared<CommonBinsValidator>()),
                  "An input workspace.");

  initOffspecBgdProperties();
  initSNSBgdProperties();
  initILLBgdProperties();

  // Offspec output workspace
  declareProperty(make_unique<WorkspaceProperty<>>("OutputWorkspaceOffspec", "",
                                                   Direction::Output),
                  "A Workspace with the background removed calculated with a "
                  "basic offspec script.");


  // SNS output workspace
  declareProperty(make_unique<WorkspaceProperty<>>("OutputWorkspaceSNS", "",
                                                   Direction::Output),
                  "A Workspace with the background removed calculated using "
                  "the SNS liquids calculation.");

  // ILL output workspace
  declareProperty(make_unique<WorkspaceProperty<>>("OutputWorkspaceILL", "",
                                                   Direction::Output),
                  "A Workspace with the background removed calculated by "
                  "fitting a polynomial to the background.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void ReflectometryBackgroundSubtraction::exec() {
  API::MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  
  // calculate offspec background
  bool offspecproperty = getProperty("CalculateOffspec");
  if (offspecproperty) {
	  auto offspec = createChildAlgorithm("OffspecBackgroundSubtraction");
	  offspec->initialize();
	  offspec->setProperty("InputWorkspace", inputWS);
	  offspec->setProperty("BottomBackgroundRanges", getPropertyValue("BottomBackgroundRanges"));
	  offspec->setProperty("TopBackgroundRanges", getPropertyValue("TopBackgroundRanges"));
	  offspec->execute();
	  API::MatrixWorkspace_sptr outputOS =
		  offspec->getProperty("OutputWorkspace");
      setProperty("OutputWorkspaceOffspec", outputOS);
  } else {
    setProperty("OutputWorkspaceOffspec", inputWS);
  }
  
  
  //calculate ILL background
  bool ILLproperty = getProperty("CalculateILL");
  if (ILLproperty) {
	auto poly = createChildAlgorithm("CalculatePolynomialBackground");
	poly->initialize();
	poly->setProperty("InputWorkspace", inputWS);
	poly->setProperty("Degree", getPropertyValue("Degree"));
	poly->setProperty("XRanges", getPropertyValue("XRanges"));
	poly->setProperty("CostFunction", getPropertyValue("CostFunction"));
	poly->execute();
	API::MatrixWorkspace_sptr outputILL = poly->getProperty("OutputWorkspace");
    setProperty("OutputWorkspaceILL", outputILL);
  } else {
    setProperty("OutputWorkspaceILL", inputWS);
  }
  
  
  //calculate SNS background
  bool SNSproperty = getProperty("CalculateSNS");
  if (SNSproperty) {
    auto snsBgd = createChildAlgorithm("LRSubtractAverageBackground");
    snsBgd->initialize();
    snsBgd->setProperty("InputWorkspace", inputWS);
    snsBgd->setProperty("PeakRange", getPropertyValue("PeakRange"));
    snsBgd->setProperty("BackgroundRange", getPropertyValue("BackgroundRange"));
    snsBgd->setProperty("LowResolutionRange", getPropertyValue("LowResolutionRange"));
    snsBgd->setProperty("SumPeak", getPropertyValue("SumPeak"));
    snsBgd->execute();
    API::MatrixWorkspace_sptr outputSNS =
        snsBgd->getProperty("OutputWorkspace");
    setProperty("OutputWorkspaceSNS", outputSNS);
  } else {
    setProperty("OutputWorkspaceSNS", inputWS);
  }

  }
} // namespace Algorithms
} // namespace Mantid
