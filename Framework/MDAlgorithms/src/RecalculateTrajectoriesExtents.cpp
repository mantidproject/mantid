// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidMDAlgorithms/RecalculateTrajectoriesExtents.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/MDGeometry/QSample.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/TimeSeriesProperty.h"

namespace Mantid {
namespace MDAlgorithms {
using namespace Mantid::Kernel;
using namespace Mantid::API;
using VectorDoubleProperty = Kernel::PropertyWithValue<std::vector<double>>;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(RecalculateTrajectoriesExtents)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string RecalculateTrajectoriesExtents::name() const {
  return "RecalculateTrajectoriesExtents";
}

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
                  "Copy of the input MDEventWorkspace with the corrected "
                  "trajectory extents.");
}

/// Validate the input workspace @see Algorithm::validateInputs
std::map<std::string, std::string>
RecalculateTrajectoriesExtents::validateInputs() {
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
  if (outWS->getNumDims() > 3) {
    if (outWS->getDimension(3)->getMDFrame().name() == "DeltaE") {
      diffraction = false;
      if (outWS->getExperimentInfo(0)->run().hasProperty("Ei")) {
        Ei = outWS->getExperimentInfo(0)->run().getPropertyValueAsType<double>(
            "Ei");
      } else {
        throw std::runtime_error(
            "Workspace contains energy transfer axis, but no Ei."
            "The MD normalization workflow is not implemented for "
            "indirect geometry");
      }
    }
  }

  const double energyToK = 8.0 * M_PI * M_PI * PhysicalConstants::NeutronMass *
                           PhysicalConstants::meV * 1e-20 /
                           (PhysicalConstants::h * PhysicalConstants::h);

  auto convention = Kernel::ConfigService::Instance().getString("Q.convention");
  // get limits for all dimensions
  double qxmin = outWS->getDimension(0)->getMinimum();
  double qxmax = outWS->getDimension(0)->getMaximum();
  double qymin = outWS->getDimension(1)->getMinimum();
  double qymax = outWS->getDimension(1)->getMaximum();
  double qzmin = outWS->getDimension(2)->getMinimum();
  double qzmax = outWS->getDimension(2)->getMaximum();
  double dEmin(0.0), dEmax(0.0);
  size_t nqedims = 3;
  if (!diffraction) {
    nqedims = 4;
    dEmin = outWS->getDimension(3)->getMinimum();
    dEmax = outWS->getDimension(3)->getMaximum();
  }
  std::vector<double> otherDimsMin, otherDimsMax;
  std::vector<std::string> otherDimsNames;
  for (size_t i = nqedims; i < outWS->getNumDims(); i++) {
    otherDimsMin.emplace_back(outWS->getDimension(i)->getMinimum());
    otherDimsMax.emplace_back(outWS->getDimension(i)->getMaximum());
    otherDimsNames.emplace_back(outWS->getDimension(i)->getName());
  }

  // Loop over experiment infos
  size_t nExperimentInfos = outWS->getNumExperimentInfo();
  if (nExperimentInfos > 1) {
    g_log.warning("More than one experiment info. On merged workspaces, the "
                  "limits recalculations might be wrong");
  }

  for (size_t iExpInfo = 0; iExpInfo < nExperimentInfos; iExpInfo++) {
    auto &currentExptInfo =
        *(outWS->getExperimentInfo(static_cast<uint16_t>(iExpInfo)));

    const auto &spectrumInfo = currentExptInfo.spectrumInfo();
    const int64_t nspectra = static_cast<int64_t>(spectrumInfo.size());
    std::vector<double> lowValues, highValues;

    auto *lowValuesLog = dynamic_cast<VectorDoubleProperty *>(
        currentExptInfo.getLog("MDNorm_low"));
    lowValues = (*lowValuesLog)();

    auto *highValuesLog = dynamic_cast<VectorDoubleProperty *>(
        currentExptInfo.getLog("MDNorm_high"));
    highValues = (*highValuesLog)();

    // deal with other dimensions first
    bool zeroWeights(false);
    for (size_t iOtherDims = 0; iOtherDims < otherDimsNames.size();
         iOtherDims++) {
      // check other dimensions. there might be no events, but if the first log
      // value is not within limits, the weight should be zero as well
      auto *otherDimsLog = dynamic_cast<Kernel::TimeSeriesProperty<double> *>(
          currentExptInfo.run().getProperty(otherDimsNames[iOtherDims]));
      if ((otherDimsLog->firstValue() < otherDimsMin[iOtherDims]) ||
          (otherDimsLog->firstValue() > otherDimsMax[iOtherDims])) {
        zeroWeights = true;
        g_log.warning() << "In experimentInfo " << iExpInfo << ", log "
                        << otherDimsNames[iOtherDims] << " is outside limits\n";
        continue;
      }
    }
    if (zeroWeights) {
      highValues = lowValues;
    } else {
      auto source = currentExptInfo.getInstrument()->getSource()->getPos();
      auto sample = currentExptInfo.getInstrument()->getSample()->getPos();
      auto beamDir = sample - source;
      beamDir.normalize();
      auto gon = currentExptInfo.run().getGoniometerMatrix();
      gon.Invert();

      // calculate limits in Q_sample
      for (int64_t i = 0; i < nspectra; i++) {
        if (!spectrumInfo.hasDetectors(i) || spectrumInfo.isMonitor(i) ||
            spectrumInfo.isMasked(i)) {
          highValues[i] = lowValues[i];
          continue;
        }
        const auto &detector = spectrumInfo.detector(i);
        double theta = detector.getTwoTheta(sample, beamDir);
        double phi = detector.getPhi();
        V3D qout(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta)),
            qin(0., 0., 1.), qLabLow, qLabHigh, qSampleLow, qSampleHigh;
        double kfmin, kfmax;
        if (convention == "Crystallography") {
          qout *= -1;
          qin *= -1;
        }
        if (diffraction) {
          // units of limits are momentum
          kfmin = lowValues[i];
          kfmax = highValues[i];
          qLabLow = (qin - qout) * kfmin;
          qLabHigh = (qin - qout) * kfmax;
        } else {
          if (dEmin > lowValues[i]) {
            lowValues[i] = dEmin;
          }
          if (dEmax < highValues[i]) {
            highValues[i] = dEmax;
          }
          if (lowValues[i] > Ei) {
            lowValues[i] = Ei;
          }
          if (highValues[i] > Ei) {
            highValues[i] = Ei;
          }

          double ki = std::sqrt(energyToK * Ei);
          kfmin = std::sqrt(energyToK * (Ei - lowValues[i]));
          kfmax = std::sqrt(energyToK * (Ei - highValues[i]));
          qLabLow = qin * ki - qout * kfmin;
          qLabHigh = qin * ki - qout * kfmax;
        }
        qSampleLow = gon * qLabLow;
        qSampleHigh = gon * qLabHigh;
        // check intersection with the box
        // completely outside the box -> no weight
        if (((qSampleLow.X() < qxmin) && (qSampleHigh.X() < qxmin)) ||
            ((qSampleLow.X() > qxmax) && (qSampleHigh.X() > qxmax)) ||
            ((qSampleLow.Y() < qymin) && (qSampleHigh.Y() < qymin)) ||
            ((qSampleLow.Y() > qymax) && (qSampleHigh.Y() > qymax)) ||
            ((qSampleLow.Z() < qzmin) && (qSampleHigh.Z() < qzmin)) ||
            ((qSampleLow.Z() > qzmax) && (qSampleHigh.Z() > qzmax))) {
          highValues[i] = lowValues[i];
          continue;
        }
        // either intersection or completely indide the box
        if ((qxmin - qSampleLow.X()) * (qxmin - qSampleHigh.X()) < 0) {
          double kfIntersection = (qxmin - qSampleLow.X()) * (kfmax - kfmin) /
                                      (qSampleHigh.X() - qSampleLow.X()) +
                                  kfmin;
          if (!diffraction) {
            kfIntersection = Ei - kfIntersection * kfIntersection / energyToK;
          }
          if ((qSampleLow.X() < qxmin) && (lowValues[i] < kfIntersection)) {
            lowValues[i] = kfIntersection;
          }
          if ((qSampleHigh.X() < qxmin) && (highValues[i] > kfIntersection)) {
            highValues[i] = kfIntersection;
          }
        }

        if ((qxmax - qSampleLow.X()) * (qxmax - qSampleHigh.X()) < 0) {
          double kfIntersection = (qxmax - qSampleLow.X()) * (kfmax - kfmin) /
                                      (qSampleHigh.X() - qSampleLow.X()) +
                                  kfmin;
          if (!diffraction) {
            kfIntersection = Ei - kfIntersection * kfIntersection / energyToK;
          }
          if ((qSampleLow.X() > qxmax) && (lowValues[i] < kfIntersection)) {
            lowValues[i] = kfIntersection;
          }
          if ((qSampleHigh.X() > qxmax) && (highValues[i] > kfIntersection)) {
            highValues[i] = kfIntersection;
          }
        }

        if ((qymin - qSampleLow.Y()) * (qymin - qSampleHigh.Y()) < 0) {
          double kfIntersection = (qymin - qSampleLow.Y()) * (kfmax - kfmin) /
                                      (qSampleHigh.Y() - qSampleLow.Y()) +
                                  kfmin;
          if (!diffraction) {
            kfIntersection = Ei - kfIntersection * kfIntersection / energyToK;
          }
          if ((qSampleLow.Y() < qymin) && (lowValues[i] < kfIntersection)) {
            lowValues[i] = kfIntersection;
          }
          if ((qSampleHigh.Y() < qymin) && (highValues[i] > kfIntersection)) {
            highValues[i] = kfIntersection;
          }
        }

        if ((qymax - qSampleLow.Y()) * (qymax - qSampleHigh.Y()) < 0) {
          double kfIntersection = (qymax - qSampleLow.Y()) * (kfmax - kfmin) /
                                      (qSampleHigh.Y() - qSampleLow.Y()) +
                                  kfmin;
          if (!diffraction) {
            kfIntersection = Ei - kfIntersection * kfIntersection / energyToK;
          }
          if ((qSampleLow.Y() > qymax) && (lowValues[i] < kfIntersection)) {
            lowValues[i] = kfIntersection;
          }
          if ((qSampleHigh.Y() > qymax) && (highValues[i] > kfIntersection)) {
            highValues[i] = kfIntersection;
          }
        }

        if ((qzmin - qSampleLow.Z()) * (qzmin - qSampleHigh.Z()) < 0) {
          double kfIntersection = (qzmin - qSampleLow.Z()) * (kfmax - kfmin) /
                                      (qSampleHigh.Z() - qSampleLow.Z()) +
                                  kfmin;
          if (!diffraction) {
            kfIntersection = Ei - kfIntersection * kfIntersection / energyToK;
          }
          if ((qSampleLow.Z() < qzmin) && (lowValues[i] < kfIntersection)) {
            lowValues[i] = kfIntersection;
          }
          if ((qSampleHigh.Z() < qzmin) && (highValues[i] > kfIntersection)) {
            highValues[i] = kfIntersection;
          }
        }

        if ((qzmax - qSampleLow.Z()) * (qzmax - qSampleHigh.Z()) < 0) {
          double kfIntersection = (qzmax - qSampleLow.Z()) * (kfmax - kfmin) /
                                      (qSampleHigh.Z() - qSampleLow.Z()) +
                                  kfmin;
          if (!diffraction) {
            kfIntersection = Ei - kfIntersection * kfIntersection / energyToK;
          }
          if ((qSampleLow.Z() > qzmax) && (lowValues[i] < kfIntersection)) {
            lowValues[i] = kfIntersection;
          }
          if ((qSampleHigh.Z() > qzmax) && (highValues[i] > kfIntersection)) {
            highValues[i] = kfIntersection;
          }
        }
      } // end loop over spectra
    }

    // set the new values for the MDNorm_low and MDNorm_high
    currentExptInfo.mutableRun().addProperty("MDNorm_low", lowValues, true);
    currentExptInfo.mutableRun().addProperty("MDNorm_high", highValues, true);
  }

  setProperty("OutputWorkspace", outWS);
}

} // namespace MDAlgorithms
} // namespace Mantid
