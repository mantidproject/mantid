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
using VectorDoubleProperty = Kernel::PropertyWithValue<std::vector<double>>;

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

  // check if using diffraction or direct inelastic
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

  // get limits for all dimensions
  double qxmin = outWS->getDimension(0)->getMinimum();
  double qxmax = outWS->getDimension(0)->getMaximum();
  double qymin = outWS->getDimension(1)->getMinimum();
  double qymax = outWS->getDimension(1)->getMaximum();
  double qzmin = outWS->getDimension(2)->getMinimum();
  double qzmax = outWS->getDimension(2)->getMaximum();
  double dEmin(0.0), dEmax(0.0);
  size_t nqedims=3;
  if(!diffraction){
    nqedims=4;
    dEmin = outWS->getDimension(3)->getMinimum();
    dEmax = outWS->getDimension(3)->getMaximum();
  }
  std::vector<double> otherDimsMin, otherDimsMax;
  std::vector<std::string> otherDimsNames;
  for (size_t i=nqedims;i<outWS->getNumDims();i++) {
    otherDimsMin.emplace_back(outWS->getDimension(i)->getMinimum());
    otherDimsMax.emplace_back(outWS->getDimension(i)->getMaximum());
    otherDimsNames.emplace_back(outWS->getDimension(i)->getName());
  }


  // Loop over experiment infos
  size_t nExperimentInfos = outWS->getNumExperimentInfo();
  if (nExperimentInfos >1) {
    g_log.warning("More than one experiment info. On merged workspaces, the "
                  "limits recalculations might be wrong");
  }

  for (size_t iExpInfo=0; iExpInfo<nExperimentInfos; iExpInfo++){
    auto &currentExptInfo = *(outWS->getExperimentInfo(static_cast<uint16_t>(iExpInfo)));
    // get instrument and goniometer
    const auto &spectrumInfo = currentExptInfo.spectrumInfo();
    const int64_t nspectra = static_cast<int64_t>(spectrumInfo.size());
    std::vector<double> lowValues, highValues;

    if (currentExptInfo.run().hasProperty("MDNorm_low")){
      auto *lowValuesLog = dynamic_cast<VectorDoubleProperty *>(
          currentExptInfo.getLog("MDNorm_low"));
          lowValues = (*lowValuesLog)();
    } else {
      double minValue=currentExptInfo.run().getBinBoundaries().front();
      lowValues = std::vector<double>(nspectra, minValue);
    }
    if (currentExptInfo.run().hasProperty("MDNorm_high")){
      auto *highValuesLog = dynamic_cast<VectorDoubleProperty *>(
          currentExptInfo.getLog("MDNorm_high"));
          highValues = (*highValuesLog)();
    } else {
      double maxValue=currentExptInfo.run().getBinBoundaries().back();
      highValues = std::vector<double>(nspectra, maxValue);
    }

    // deal with other dimensions first
    bool zeroWeights(false);
    for(size_t iOtherDims=0; iOtherDims<otherDimsNames.size(); iOtherDims++) {
      // check other dimensions. there might be no events, but if the first log value is
      // not within limits, the weight should be zero as well
      auto *otherDimsLog=dynamic_cast<VectorDoubleProperty *>(
            currentExptInfo.getLog(otherDimsNames[iOtherDims]));
      if (((*otherDimsLog)()[0] < otherDimsMin[iOtherDims]) ||((*otherDimsLog)()[0] > otherDimsMax[iOtherDims])){
        zeroWeights=true;
        g_log.warning()<<"In experimentInfo "<<iExpInfo<<", log "<<otherDimsNames[iOtherDims]<<" is outside limits\n";
        continue;
      }
    }
    if (zeroWeights) {
      highValues = lowValues;
    } else {
        // loop over detectors

    }

    // set the new values for the MDNorm_low and MDNorm_high
    currentExptInfo.mutableRun().addProperty("MDNorm_low",lowValues,true);
    currentExptInfo.mutableRun().addProperty("MDNorm_high",highValues,true);
  }


  setProperty("OutputWorkspace", outWS);
}

} // namespace MDAlgorithms
} // namespace Mantid
