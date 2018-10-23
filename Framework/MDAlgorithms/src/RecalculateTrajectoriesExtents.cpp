// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidMDAlgorithms/RecalculateTrajectoriesExtents.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidGeometry/MDGeometry/QSample.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectrumInfo.h"

namespace Mantid {
namespace MDAlgorithms {
using namespace Mantid::Kernel;
using namespace Mantid::API;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(RecalculateTrajectoriesExtents)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string RecalculateTrajectoriesExtents::name() const { return "RecalculateTrajectoriesExtents"; }

/// Algorithm's version for identification. @see Algorithm::version
int RecalculateTrajectoriesExtents::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string RecalculateTrajectoriesExtents::category() const {
  return "MDAlgorithms\\Normalisation";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string RecalculateTrajectoriesExtents::summary() const {
  return "Recalculates trajectory limits set by CropWorkspaceForMDNorm";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void RecalculateTrajectoriesExtents::init() {
  declareProperty(make_unique<WorkspaceProperty<IMDEventWorkspace>>(
                        "InputWorkspace", "", Direction::Input),
                    "An input MDEventWorkspace. Must be in Q_sample frame.");
  declareProperty(make_unique<WorkspaceProperty<IMDEventWorkspace>>(
                        "OutputWorkspace", "", Direction::Output),
                    "Copy of the input MDEventWorkspace with the corrected trajectory extents.");
}

/// Validate the input workspace @see Algorithm::validateInputs
std::map<std::string, std::string> RecalculateTrajectoriesExtents::validateInputs() {
  std::map<std::string, std::string> errorMessage;

  // Check for input workspace frame
  Mantid::API::IMDEventWorkspace_sptr inputWS = this->getProperty("InputWorkspace");
  if (inputWS->getNumDims() < 3) {
    errorMessage.emplace("InputWorkspace",
                         "The input workspace must be at least 3D");
  } else {
    for (size_t i=0; i<3;i++){
      if (inputWS->getDimension(i)->getMDFrame().name() != Mantid::Geometry::QSample::QSampleName){
        errorMessage.emplace("InputWorkspace",
                             "The input workspace must be in Q_sample");
      }
    }
  }
  return errorMessage;
}


//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void RecalculateTrajectoriesExtents::exec() {
  IMDEventWorkspace_sptr inWS = getProperty("InputWorkspace");
  IMDEventWorkspace_sptr outWS = getProperty("OutputWorkspace");

  // If input and output workspaces are not the same, create a new workspace for
  // the output
  if (outWS != inWS) {
    outWS = inWS->clone();
  }

  //check if using diffraction or direct inelastic
  bool diffraction(true);
  double Ei(0.0);
  if (outWS->getNumDims() > 3){
      if (outWS->getDimension(3)->getMDFrame().name() == "DeltaE") {
        diffraction = false;
        if (outWS->getExperimentInfo(0)->run().hasProperty("Ei")) {
          Property *eiprop = outWS->getExperimentInfo(0)->run().getProperty("Ei");
          Ei = boost::lexical_cast<double>(eiprop->value());
        } else {
          throw std::runtime_error("Workspace contains energy transfer axis, but no Ei."
                                   "The MD normalization workflow is not implemented for "
                                   "indirect geometry");
        }
      }
  }

  size_t nExperimentInfos = outWS->getNumExperimentInfo();
  if (nExperimentInfos >1) {
    g_log.warning("More than one experiment info. On merged workspaces, the "
                  "limits recalculations might be wrong");
  }
  for (size_t i=0; i<nExperimentInfos; i++){
    const auto &currentExptInfo = *(outWS->getExperimentInfo(static_cast<uint16_t>(i)));
    // get instrument and goniometer
    const auto &spectrumInfo = currentExptInfo.spectrumInfo();
    const int64_t nspectra = static_cast<int64_t>(spectrumInfo.size());
    std::vector<double> lowValues, highValues;

    if (currentExptInfo.run().hasProperty("MDNorm_low")){
      using VectorDoubleProperty = Kernel::PropertyWithValue<std::vector<double>>;
      auto *lowValuesLog = dynamic_cast<VectorDoubleProperty *>(
          currentExptInfo.getLog("MDNorm_low"));
          lowValues = (*lowValuesLog)();
    } else {
      double minValue=currentExptInfo.run().getBinBoundaries().front();
      lowValues = std::vector<double>(nspectra, minValue);
    }
    if (currentExptInfo.run().hasProperty("MDNorm_high")){
      using VectorDoubleProperty = Kernel::PropertyWithValue<std::vector<double>>;
      auto *highValuesLog = dynamic_cast<VectorDoubleProperty *>(
          currentExptInfo.getLog("MDNorm_high"));
          highValues = (*highValuesLog)();
    } else {
      double maxValue=currentExptInfo.run().getBinBoundaries().back();
      highValues = std::vector<double>(nspectra, maxValue);
    }
    g_log.warning()<<lowValues[0]<<"\n";
    g_log.warning()<<highValues[0]<<"\n";

    // loop over detectors
  }

  setProperty("OutputWorkspace", outWS);
}

} // namespace MDAlgorithms
} // namespace Mantid
