// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/SumSpectra.h"
#include "MantidAPI/CommonBinsValidator.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/RebinnedOutput.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidGeometry/IDetector.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/EnabledWhenProperty.h"

#include <functional>

namespace Mantid::Algorithms {

// Register the class into the algorithm factory
DECLARE_ALGORITHM(SumSpectra)

using namespace Kernel;
using namespace API;
using namespace DataObjects;

namespace {
/**
 * @param validationOutput Output map to be populated with any errors
 * @param ws An input workspace to verify
 * @param minIndex Minimum index of range to sum
 * @param maxIndex Mmaximum index of range to sum
 * @param indices A list of indices to sum
 */
bool validateSingleMatrixWorkspace(std::map<std::string, std::string> &validationOutput, const MatrixWorkspace &ws,
                                   const int minIndex, const int maxIndex, const std::vector<int> &indices) {
  bool success(true);
  const auto numSpectra = static_cast<int>(ws.getNumberHistograms());
  // check StartWorkSpaceIndex,  >=0 done by validator
  if (minIndex >= numSpectra) {
    validationOutput["StartWorkspaceIndex"] = "Selected minimum workspace index is greater than available "
                                              "spectra.";
    success = false;
  }
  // check EndWorkspaceIndex in range
  if (maxIndex != EMPTY_INT()) {
    // check EndWorkspaceIndex in range
    if (maxIndex >= numSpectra) {
      validationOutput["EndWorkspaceIndex"] = "Selected maximum workspace index is greater than available "
                                              "spectra.";
      success = false;
      // check StartWorkspaceIndex < EndWorkspaceIndex
    }
  }
  // check ListOfWorkspaceIndices in range
  if (std::any_of(indices.cbegin(), indices.cend(),
                  [numSpectra](const auto index) { return (index >= numSpectra) || (index < 0); })) {
    validationOutput["ListOfWorkspaceIndices"] = "One or more indices out of range of available spectra.";
    success = false;
  }
  return success;
}

/**
 * @param validationOutput Output map to be populated with any errors
 * @param name A string identifier for an input workspace to verify
 * @param minIndex Minimum index of range to sum
 * @param maxIndex Mmaximum index of range to sum
 * @param indices A list of indices to sum
 */
void validateWorkspaceName(std::map<std::string, std::string> &validationOutput, const std::string &name,
                           const int minIndex, const int maxIndex, const std::vector<int> &indices) {
  const auto &ads = AnalysisDataService::Instance();
  if (!ads.doesExist(name))
    return;
  auto wsGroup = ads.retrieveWS<WorkspaceGroup>(name);
  if (!wsGroup)
    return;
  size_t index = 0;
  for (const auto &item : *wsGroup) {
    auto matrixWs = std::dynamic_pointer_cast<MatrixWorkspace>(item);
    if (!matrixWs) {
      validationOutput["InputWorkspace"] = "Input group contains an invalid workspace type at item " +
                                           std::to_string(index) + ". All members must be a  MatrixWorkspace";
      break;
    }
    if (!validateSingleMatrixWorkspace(validationOutput, *matrixWs, minIndex, maxIndex, indices)) {
      break;
    }
    ++index;
  }
}
} // namespace

/** Initialisation method.
 *
 */
void SumSpectra::init() {
  declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input,
                                                        std::make_shared<CommonBinsValidator>()),
                  "The workspace containing the spectra to be summed.");
  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "", Direction::Output),
                  "The name of the workspace to be created as the output of the algorithm. "
                  " A workspace of this name will be created and stored in the Analysis "
                  "Data Service.");

  auto mustBePositive = std::make_shared<BoundedValidator<int>>();
  mustBePositive->setLower(0);
  declareProperty("StartWorkspaceIndex", 0, mustBePositive, "The first Workspace index to be included in the summing");
  declareProperty("EndWorkspaceIndex", EMPTY_INT(), mustBePositive,
                  "The last Workspace index to be included in the summing");

  declareProperty(std::make_unique<Kernel::ArrayProperty<int>>("ListOfWorkspaceIndices"),
                  "A list of workspace indices as a string with ranges, for "
                  "example: 5-10,15,20-23. \n"
                  "Optional: if not specified, then the "
                  "Start/EndWorkspaceIndex fields are used alone. "
                  "If specified, the range and the list are combined (without "
                  "duplicating indices). For example, a range of 10 to 20 and "
                  "a list '12,15,26,28' gives '10-20,26,28'.");

  declareProperty("IncludeMonitors", true, "Whether to include monitor spectra in the summation.");

  declareProperty("WeightedSum", false,
                  "Instead of the usual spectra sum, calculate the weighted "
                  "sum. This has the form: \n"
                  ":math:`nSpectra "
                  "\\times\\Sigma(Signal_i/Error_i^2)/\\Sigma(1/Error_i^2)`\n "
                  "This property is ignored for event workspace.\n"
                  "The sums are defined for :math:`Error_i != 0` only, so the "
                  "values with zero error are dropped from the summation. To "
                  "estimate the number of dropped values see the "
                  "description. ");

  declareProperty("RemoveSpecialValues", false,
                  "If enabled floating point special values such as NaN or Inf"
                  " are removed before the spectra are summed.");

  declareProperty("MultiplyBySpectra", true,
                  "For unnormalized data one should multiply the weighted sum "
                  "by the number of spectra contributing to the bin.");
  setPropertySettings("MultiplyBySpectra", std::make_unique<EnabledWhenProperty>("WeightedSum", IS_EQUAL_TO, "1"));

  declareProperty("UseFractionalArea", true,
                  "Normalize the output workspace to the fractional area for "
                  "RebinnedOutput workspaces.");
}

/*
 * Validate the input parameters
 * @returns map with keys corresponding to properties with errors and values
 * containing the error messages.
 */
std::map<std::string, std::string> SumSpectra::validateInputs() {
  // create the map
  std::map<std::string, std::string> validationOutput;

  // Non-workspace checks
  const int minIndex = getProperty("StartWorkspaceIndex");
  const int maxIndex = getProperty("EndWorkspaceIndex");
  if (minIndex > maxIndex) {
    validationOutput["StartWorkspaceIndex"] = "Selected minimum workspace "
                                              "index is greater than selected "
                                              "maximum workspace index.";
    validationOutput["EndWorkspaceIndex"] = "Selected maximum workspace index "
                                            "is lower than selected minimum "
                                            "workspace index.";
  } else {
    const std::vector<int> indices = getProperty("ListOfWorkspaceIndices");
    if (MatrixWorkspace_const_sptr singleWs = getProperty("InputWorkspace")) {
      validateSingleMatrixWorkspace(validationOutput, *singleWs, minIndex, maxIndex, indices);
    } else {
      validateWorkspaceName(validationOutput, getPropertyValue("InputWorkspace"), minIndex, maxIndex, indices);
    }
  }
  return validationOutput;
}

/** Executes the algorithm
 *
 */
void SumSpectra::exec() {
  // Try and retrieve the optional properties
  m_keepMonitors = getProperty("IncludeMonitors");
  m_replaceSpecialValues = getProperty("RemoveSpecialValues");

  // Get the input workspace
  MatrixWorkspace_const_sptr localworkspace = getProperty("InputWorkspace");
  m_numberOfSpectra = static_cast<int>(localworkspace->getNumberHistograms());
  determineIndices(m_numberOfSpectra);
  m_yLength = localworkspace->y(*(m_indices.begin())).size();

  // determine the output spectrum number
  m_outSpecNum = getOutputSpecNo(localworkspace);
  g_log.information() << "Spectra remapping gives single spectra with spectra number: " << m_outSpecNum << "\n";

  m_calculateWeightedSum = getProperty("WeightedSum");
  m_multiplyByNumSpec = getProperty("MultiplyBySpectra");

  // setup all of the outputs
  MatrixWorkspace_sptr outputWorkspace = nullptr;
  size_t numSpectra(0); // total number of processed spectra
  size_t numMasked(0);  // total number of the masked and skipped spectra
  size_t numZeros(0);   // number of spectra which have 0 value in the first
  // column (used in special cases of evaluating how good
  // Poissonian statistics is)

  Progress progress(this, 0.0, 1.0, m_indices.size());
  EventWorkspace_const_sptr eventW = std::dynamic_pointer_cast<const EventWorkspace>(localworkspace);
  if (eventW) {
    if (m_calculateWeightedSum) {
      g_log.warning("Ignoring request for WeightedSum");
      m_calculateWeightedSum = false;
    }
    outputWorkspace = create<EventWorkspace>(*eventW, 1, eventW->binEdges(0));

    execEvent(outputWorkspace, progress, numSpectra, numMasked, numZeros);
  } else {
    //-------Workspace 2D mode -----

    // Create the 2D workspace for the output
    outputWorkspace = API::WorkspaceFactory::Instance().create(
        localworkspace, 1, localworkspace->x(*(m_indices.begin())).size(), m_yLength);

    // This is the (only) output spectrum
    auto &outSpec = outputWorkspace->getSpectrum(0);

    // Copy over the bin boundaries
    outSpec.setSharedX(localworkspace->sharedX(0));

    // Build a new spectra map
    outSpec.setSpectrumNo(m_outSpecNum);
    outSpec.clearDetectorIDs();

    if (localworkspace->id() == "RebinnedOutput") {
      // this version is for a special workspace that has fractional overlap
      // information
      doFractionalSum(outputWorkspace, progress, numSpectra, numMasked, numZeros);
    } else {
      // for things where all the bins are lined up
      doSimpleSum(outputWorkspace, progress, numSpectra, numMasked, numZeros);
    }

    // take the square root of all the accumulated squared errors - Assumes
    // Gaussian errors
    auto &YError = outSpec.mutableE();
    std::transform(YError.begin(), YError.end(), YError.begin(), (double (*)(double))std::sqrt);
  }

  // set up the summing statistics
  outputWorkspace->mutableRun().addProperty("NumAllSpectra", int(numSpectra), "", true);
  outputWorkspace->mutableRun().addProperty("NumMaskSpectra", int(numMasked), "", true);
  outputWorkspace->mutableRun().addProperty("NumZeroSpectra", int(numZeros), "", true);

  // Assign it to the output workspace property
  setProperty("OutputWorkspace", outputWorkspace);
}

void SumSpectra::determineIndices(const size_t numberOfSpectra) {
  // assume that m_numberOfSpectra has been set
  m_indices.clear();

  // try the list form first
  const std::vector<int> indices_list = getProperty("ListOfWorkspaceIndices");
  m_indices.insert(indices_list.begin(), indices_list.end());

  // add the range specified by the user
  // this has been checked to be 0<= m_minWsInd <= maxIndex <=
  // m_numberOfSpectra where maxIndex can be an EMPTY_INT
  int minIndex = getProperty("StartWorkspaceIndex");
  int maxIndex = getProperty("EndWorkspaceIndex");
  if (isEmpty(maxIndex) && m_indices.empty()) {
    maxIndex = static_cast<int>(numberOfSpectra - 1);
  }

  // create the indices in the range
  if (!isEmpty(maxIndex)) {
    for (int i = minIndex; i <= maxIndex; i++)
      m_indices.insert(static_cast<size_t>(i));
  }
}

/**
 * Determine the minimum spectrum No for summing. This requires that
 * SumSpectra::indices has aly been set.
 * @param localworkspace The workspace to use.
 * @return The minimum spectrum No for all the spectra being summed.
 */
specnum_t SumSpectra::getOutputSpecNo(const MatrixWorkspace_const_sptr &localworkspace) {
  // initial value - any included spectrum will do
  specnum_t specId = localworkspace->getSpectrum(*(m_indices.begin())).getSpectrumNo();

  // the total number of spectra
  size_t totalSpec = localworkspace->getNumberHistograms();

  specnum_t temp;
  for (const auto index : m_indices) {
    if (index < totalSpec) {
      temp = localworkspace->getSpectrum(index).getSpectrumNo();
      if (temp < specId)
        specId = temp;
    }
  }

  return specId;
}

/**
 * Calls an algorithm to replace special values within the workspace
 * such as NaN or Inf to 0.
 * @return The workspace with special floating point values set to 0
 */
API::MatrixWorkspace_sptr SumSpectra::replaceSpecialValues() {
  // Get a copy of the input workspace
  MatrixWorkspace_sptr wksp = getProperty("InputWorkspace");

  if (!m_replaceSpecialValues) {
    // Skip any additional processing
    return wksp;
  }

  auto alg = createChildAlgorithm("ReplaceSpecialValues");
  alg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", wksp);
  std::string outName = "_" + wksp->getName() + "_clean";
  alg->setProperty("OutputWorkspace", outName);
  alg->setProperty("NaNValue", 0.0);
  alg->setProperty("NaNError", 0.0);
  alg->setProperty("InfinityValue", 0.0);
  alg->setProperty("InfinityError", 0.0);
  alg->executeAsChildAlg();
  return alg->getProperty("OutputWorkspace");
}

namespace { // anonymous namespace
// small function that normalizes the accumulated weight in a consistent fashion
// the weights are modified in the process
size_t applyWeight(const size_t numSpectra, HistogramData::HistogramY &y, std::vector<double> &weights,
                   const std::vector<size_t> &nZeros, const bool multiplyByNumSpec) {
  // convert weight into proper normalization factor
  if (multiplyByNumSpec) {
    std::transform(weights.begin(), weights.end(), nZeros.begin(), weights.begin(),
                   [numSpectra](const double weight, const size_t nzero) {
                     if (numSpectra > nzero) {
                       return static_cast<double>(numSpectra - nzero) / weight;
                     } else {
                       return 1.;
                     }
                   });
  } else {
    std::transform(weights.begin(), weights.end(), nZeros.begin(), weights.begin(),
                   [numSpectra](const double weight, const size_t nzero) {
                     if (numSpectra > nzero) {
                       return 1. / weight;
                     } else {
                       return 1.;
                     }
                   });
  }

  // apply the normalization
  y *= weights;

  // the total number of bins with any zeros between all of the spectra used
  return std::accumulate(nZeros.begin(), nZeros.end(), size_t(0));
}

// various checks on the workspace index to see if it should be included in the
// sum if it is masked, the value of numMasked is incremented
bool useSpectrum(const SpectrumInfo &spectrumInfo, const size_t wsIndex, const bool keepMonitors, size_t &numMasked) {
  if (spectrumInfo.hasDetectors(wsIndex)) {
    // Skip monitors, if the property is set to do so
    if (!keepMonitors && spectrumInfo.isMonitor(wsIndex))
      return false;
    // Skip masked detectors
    if (spectrumInfo.isMasked(wsIndex)) {
      numMasked++;
      return false;
    }
  }
  return true;
}
} // anonymous namespace

/**
 * This function deals with the logic necessary for summing a Workspace2D.
 * @param outputWorkspace the workspace to hold the summed input
 * @param progress the progress indicator
 * @param numSpectra The number of spectra contributed to the sum.
 * @param numMasked The spectra dropped from the summations because they are
 * masked.
 * @param numZeros The number of zero bins in histogram workspace or empty
 * spectra for event workspace.
 */
void SumSpectra::doSimpleSum(const MatrixWorkspace_sptr &outputWorkspace, Progress &progress, size_t &numSpectra,
                             size_t &numMasked, size_t &numZeros) {
  // Clean workspace of any NANs or Inf values
  auto localworkspace = replaceSpecialValues();

  // Get references to the output workspaces's data vectors
  auto &outSpec = outputWorkspace->getSpectrum(0);
  auto &YSum = outSpec.mutableY();
  auto &YErrorSum = outSpec.mutableE();

  std::vector<double> Weight;
  std::vector<size_t> nZeros;
  if (m_calculateWeightedSum) {
    Weight.assign(YSum.size(), 0.);
    nZeros.assign(YSum.size(), 0);
  }

  const auto &spectrumInfo = localworkspace->spectrumInfo();
  // Loop over spectra
  for (const auto wsIndex : m_indices) {
    if (!useSpectrum(spectrumInfo, wsIndex, m_keepMonitors, numMasked))
      continue;
    numSpectra++;

    const auto &YValues = localworkspace->y(wsIndex);
    const auto &YErrors = localworkspace->e(wsIndex);

    if (m_calculateWeightedSum) {
      // Retrieve the spectrum into a vector
      for (size_t yIndex = 0; yIndex < m_yLength; ++yIndex) {
        const double yErrorsVal = YErrors[yIndex];
        if (std::isnormal(yErrorsVal)) { // is non-zero, nan, or infinity
          const double errsq = yErrorsVal * yErrorsVal;
          YErrorSum[yIndex] += errsq;
          Weight[yIndex] += 1. / errsq;
          YSum[yIndex] += YValues[yIndex] / errsq;
        } else {
          nZeros[yIndex]++;
        }
      }
    } else {
      YSum += YValues;
      std::transform(YErrorSum.begin(), YErrorSum.end(), YErrors.begin(), YErrorSum.begin(),
                     [](const double accum, const double yerrorSpec) { return accum + yerrorSpec * yerrorSpec; });
    }

    // Map all the detectors onto the spectrum of the output
    outSpec.addDetectorIDs(localworkspace->getSpectrum(wsIndex).getDetectorIDs());

    progress.report();
  }

  if (m_calculateWeightedSum) {
    numZeros = applyWeight(numSpectra, YSum, Weight, nZeros, m_multiplyByNumSpec);
  } else {
    numZeros = 0;
  }
}

/**
 * This function handles the logic for summing RebinnedOutput workspaces.
 * @param outputWorkspace the workspace to hold the summed input
 * @param progress the progress indicator
 * @param numSpectra The number of spectra contributed to the sum.
 * @param numMasked The spectra dropped from the summations because they are
 * masked.
 * @param numZeros The number of zero bins in histogram workspace or empty
 * spectra for event workspace.
 */
void SumSpectra::doFractionalSum(const MatrixWorkspace_sptr &outputWorkspace, Progress &progress, size_t &numSpectra,
                                 size_t &numMasked, size_t &numZeros) {
  // First, we need to clean the input workspace for nan's and inf's in order
  // to treat the data correctly later. This will create a new private
  // workspace that will be retrieved as mutable.
  auto localworkspace = replaceSpecialValues();

  // Transform to real workspace types
  RebinnedOutput_sptr inWS = std::dynamic_pointer_cast<RebinnedOutput>(localworkspace);
  RebinnedOutput_sptr outWS = std::dynamic_pointer_cast<RebinnedOutput>(outputWorkspace);

  // Check finalize state prior to the sum process, at the completion
  // the output is unfinalized
  auto isFinalized = inWS->isFinalized();

  // Get references to the output workspaces's data vectors
  auto &outSpec = outputWorkspace->getSpectrum(0);
  auto &YSum = outSpec.mutableY();
  auto &YErrorSum = outSpec.mutableE();
  auto &FracSum = outWS->dataF(0);

  std::vector<double> Weight;
  std::vector<size_t> nZeros;
  if (m_calculateWeightedSum) {
    Weight.assign(YSum.size(), 0);
    nZeros.assign(YSum.size(), 0);
  }

  const auto &spectrumInfo = localworkspace->spectrumInfo();
  // Loop over spectra
  for (const auto wsIndex : m_indices) {
    if (!useSpectrum(spectrumInfo, wsIndex, m_keepMonitors, numMasked))
      continue;
    numSpectra++;

    // Retrieve the spectrum into a vector
    const auto &YValues = localworkspace->y(wsIndex);
    const auto &YErrors = localworkspace->e(wsIndex);
    const auto &FracArea = inWS->readF(wsIndex);

    if (m_calculateWeightedSum) {
      for (size_t yIndex = 0; yIndex < m_yLength; ++yIndex) {
        const double yErrorsVal = YErrors[yIndex];
        const double fracVal = (isFinalized ? FracArea[yIndex] : 1.0);
        if (std::isnormal(yErrorsVal)) { // is non-zero, nan, or infinity
          const double errsq = yErrorsVal * yErrorsVal * fracVal * fracVal;
          YErrorSum[yIndex] += errsq;
          Weight[yIndex] += 1. / errsq;
          YSum[yIndex] += YValues[yIndex] * fracVal / errsq;
        } else {
          nZeros[yIndex]++;
        }
      }
    } else {
      for (size_t yIndex = 0; yIndex < m_yLength; ++yIndex) {
        const double fracVal = (isFinalized ? FracArea[yIndex] : 1.0);
        YSum[yIndex] += YValues[yIndex] * fracVal;
        YErrorSum[yIndex] += YErrors[yIndex] * YErrors[yIndex] * fracVal * fracVal;
      }
    }
    // accumulation of fractional weight is the same
    std::transform(FracSum.begin(), FracSum.end(), FracArea.begin(), FracSum.begin(), std::plus<double>());

    // Map all the detectors onto the spectrum of the output
    outSpec.addDetectorIDs(localworkspace->getSpectrum(wsIndex).getDetectorIDs());

    progress.report();
  }

  if (m_calculateWeightedSum) {
    numZeros = applyWeight(numSpectra, YSum, Weight, nZeros, m_multiplyByNumSpec);
  } else {
    numZeros = 0;
  }

  // Create the correct representation if using fractional area
  auto useFractionalArea = getProperty("UseFractionalArea");
  if (useFractionalArea) {
    outWS->finalize();
  }
}

/** Executes the algorithm
 * @param outputWorkspace the workspace to hold the summed input
 * @param progress the progress indicator
 * @param numSpectra The number of spectra contributed to the sum.
 * @param numMasked The spectra dropped from the summations because they are
 * masked.
 * @param numZeros The number of zero bins in histogram workspace or empty
 * spectra for event workspace.
 */
void SumSpectra::execEvent(const MatrixWorkspace_sptr &outputWorkspace, Progress &progress, size_t &numSpectra,
                           size_t &numMasked, size_t &numZeros) {
  MatrixWorkspace_const_sptr localworkspace = getProperty("InputWorkspace");
  EventWorkspace_const_sptr inputWorkspace = std::dynamic_pointer_cast<const EventWorkspace>(localworkspace);

  // Get the pointer to the output event list
  EventWorkspace_sptr outputEventWorkspace = std::dynamic_pointer_cast<EventWorkspace>(outputWorkspace);
  EventList &outputEL = outputEventWorkspace->getSpectrum(0);
  outputEL.setSpectrumNo(m_outSpecNum);
  outputEL.clearDetectorIDs();

  const auto &spectrumInfo = inputWorkspace->spectrumInfo();

  // count number of events for the output
  std::size_t numOutputEvents{0};
  for (const auto i : m_indices) {
    if (spectrumInfo.hasDetectors(i)) {
      // Skip monitors, if the property is set to do so
      if (!m_keepMonitors && spectrumInfo.isMonitor(i))
        continue;
      // Skip masked detectors
      if (spectrumInfo.isMasked(i)) {
        continue;
      }
    }
    numOutputEvents += inputWorkspace->getSpectrum(i).getNumberEvents();
  }

  outputEL.reserve(numOutputEvents);

  // Loop over spectra
  for (const auto i : m_indices) {
    if (spectrumInfo.hasDetectors(i)) {
      // Skip monitors, if the property is set to do so
      if (!m_keepMonitors && spectrumInfo.isMonitor(i))
        continue;
      // Skip masked detectors
      if (spectrumInfo.isMasked(i)) {
        numMasked++;
        continue;
      }
    }
    numSpectra++;

    // Add the event lists with the operator
    const EventList &inputEL = inputWorkspace->getSpectrum(i);
    if (inputEL.empty()) {
      ++numZeros;
    }
    outputEL += inputEL;

    progress.report();
  }
}

} // namespace Mantid::Algorithms
