// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidMDAlgorithms/MDNormalization.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidGeometry/MDGeometry/QSample.h"
#include "MantidKernel/ArrayLengthValidator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/VisibleWhenProperty.h"
#include "MantidKernel/Strings.h"

namespace Mantid {
namespace MDAlgorithms {

using namespace Mantid::Kernel;
using namespace Mantid::API;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(MDNormalization)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string MDNormalization::name() const { return "MDNormalization"; }

/// Algorithm's version for identification. @see Algorithm::version
int MDNormalization::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string MDNormalization::category() const {
  return "MDAlgorithms\\Normalisation";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string MDNormalization::summary() const {
  return "Bins multidimensional data and calculate the normalization on the same grid";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void MDNormalization::init() {
  declareProperty(make_unique<WorkspaceProperty<API::IMDEventWorkspace>>(
                       "InputWorkspace", "", Kernel::Direction::Input),
                  "An input MDEventWorkspace. Must be in Q_sample frame.");

  //RLU and settings
  declareProperty("RLU", true, "Use reciprocal lattice units. If false, use Q_sample");
  setPropertyGroup("RLU","Q projections RLU");

  auto mustBe3D = boost::make_shared<Kernel::ArrayLengthValidator<double> >(3);
  std::vector<double> Q1(3, 0.), Q2(3, 0), Q3(3, 0);
  Q1[0] = 1.;
  Q2[1] = 1.;
  Q3[2] = 1.;

  declareProperty(make_unique<ArrayProperty<double>>("QDimension1", Q1, mustBe3D),
                  "The first Q projection axis - Default is (1,0,0)");
  setPropertySettings("QDimension1", make_unique<Kernel::VisibleWhenProperty>("RLU", IS_EQUAL_TO, "1"));
  setPropertyGroup("QDimension1","Q projections RLU");

  declareProperty(make_unique<ArrayProperty<double>>("QDimension2", Q2, mustBe3D),
                  "The second Q projection axis - Default is (0,1,0)");
  setPropertySettings("QDimension2", make_unique<Kernel::VisibleWhenProperty>("RLU", IS_EQUAL_TO, "1"));
  setPropertyGroup("QDimension2","Q projections RLU");

  declareProperty(make_unique<ArrayProperty<double>>("QDimension3", Q3, mustBe3D),
                  "The thirdtCalculateCover Q projection axis - Default is (0,0,1)");
  setPropertySettings("QDimension3", make_unique<Kernel::VisibleWhenProperty>("RLU", IS_EQUAL_TO, "1"));
  setPropertyGroup("QDimension3","Q projections RLU");

  //Define slicing
  for(std::size_t i=0;i<6;i++) {
    std::string propName = "Dimension"+Strings::toString(i)+"Name";
    std::string propBinning = "Dimension"+Strings::toString(i)+"Binning";
    declareProperty(Kernel::make_unique<PropertyWithValue<std::string>>(propName, "", Direction::Input),
                    "Name for the " + Strings::toString(i) + "th dimension. Leave blank for NONE.");
    declareProperty(Kernel::make_unique<ArrayProperty<double>>(propBinning),
                    "Binning for the " + Strings::toString(i) + "th dimension.\n"+
                    "- Leave blank for complete integration\n"+
                    "- One value is interpreted as step\n"
                    "- Two values are interpreted integration interval\n"+
                    "- Three values are interpreted as min, step, max");
    setPropertyGroup(propName, "Binning");
    setPropertyGroup(propBinning, "Binning");
  }

  declareProperty(make_unique<WorkspaceProperty<API::Workspace>>(
                      "OutputWorkspace", "", Kernel::Direction::Output),
                  "A name for the output data MDHistoWorkspace.");
}

//----------------------------------------------------------------------------------------------
/// Validate the input workspace @see Algorithm::validateInputs
std::map<std::string, std::string>
MDNormalization::validateInputs() {
  std::map<std::string, std::string> errorMessage;

  // Check for input workspace frame
  Mantid::API::IMDEventWorkspace_sptr inputWS =
      this->getProperty("InputWorkspace");
  if (inputWS->getNumDims() < 3) {
    errorMessage.emplace("InputWorkspace",
                         "The input workspace must be at least 3D");
  } else {
    for (size_t i = 0; i < 3; i++) {
      if (inputWS->getDimension(i)->getMDFrame().name() !=
          Mantid::Geometry::QSample::QSampleName) {
        errorMessage.emplace("InputWorkspace",
                             "The input workspace must be in Q_sample");
      }
    }
  }
  // Check for property MDNorm_low and MDNorm_high
  size_t nExperimentInfos = inputWS->getNumExperimentInfo();
  if (nExperimentInfos == 0) {
    errorMessage.emplace("InputWorkspace",
                         "There must be at least one experiment info");
  } else {
    for (size_t iExpInfo = 0; iExpInfo < nExperimentInfos; iExpInfo++) {
      auto &currentExptInfo =
          *(inputWS->getExperimentInfo(static_cast<uint16_t>(iExpInfo)));
      if (!currentExptInfo.run().hasProperty("MDNorm_low")) {
        errorMessage.emplace("InputWorkspace", "Missing MDNorm_low log. Please "
                                               "use CropWorkspaceForMDNorm "
                                               "before converting to MD");
      }
      if (!currentExptInfo.run().hasProperty("MDNorm_high")) {
        errorMessage.emplace("InputWorkspace",
                             "Missing MDNorm_high log. Please use "
                             "CropWorkspaceForMDNorm before converting to MD");
      }
    }
  }
  return errorMessage;
}
//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void MDNormalization::exec() {
  // TODO Auto-generated execute stub
}

} // namespace MDAlgorithms
} // namespace Mantid
