// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidMuon/EstimateMuonAsymmetryFromCounts.h"
#include "MantidMuon/MuonAsymmetryHelper.h"

#include "MantidAPI/IFunction.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Workspace_fwd.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidMuon/MuonAlgorithmHelper.h"

#include <cmath>
#include <numeric>
#include <vector>

namespace Mantid::Algorithms {

using namespace Mantid::DataObjects;
using namespace Kernel;
using API::Progress;
using std::size_t;

// Register the class into the algorithm factory
DECLARE_ALGORITHM(EstimateMuonAsymmetryFromCounts)

/** Initialisation method. Declares properties to be used in algorithm.
 *
 */
void EstimateMuonAsymmetryFromCounts::init() {
  declareProperty(
      std::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>("InputWorkspace", "", Direction::Input),
      "The name of the input 2D workspace.");
  declareProperty("WorkspaceName", "",
                  "The name used in the normalization "
                  "table. If this is blank the "
                  "InputWorkspace's name will be used.");

  declareProperty(
      std::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>("OutputWorkspace", "", Direction::Output),
      "The name of the output 2D workspace.");
  declareProperty("OutputUnNormData", false, "If to output the data with just the exponential decay removed.");

  declareProperty(std::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>(
                      "OutputUnNormWorkspace", "unNormalisedData", Direction::Output, API::PropertyMode::Optional),
                  "The name of the output unnormalized workspace.");

  std::vector<int> empty;
  declareProperty(std::make_unique<Kernel::ArrayProperty<int>>("Spectra", std::move(empty)),
                  "The workspace indices to remove the exponential decay from.");
  declareProperty("StartX", 0.1, "The lower limit for calculating the asymmetry (an X value).");
  declareProperty("EndX", 15.0, "The upper limit for calculating the asymmetry  (an X value).");
  declareProperty("NormalizationIn", 0.0,
                  "If this value is non-zero then this "
                  "is used for the normalization, "
                  "instead of being estimated.");

  declareProperty(std::make_unique<API::WorkspaceProperty<API::ITableWorkspace>>(
                      "NormalizationTable", "", Direction::InOut, API::PropertyMode::Optional),
                  "Name of the table containing the normalizations for the asymmetries.");
}

/*
 * Validate the input parameters
 * @returns map with keys corresponding to properties with errors and values
 * containing the error messages.
 */
std::map<std::string, std::string> EstimateMuonAsymmetryFromCounts::validateInputs() {
  // create the map
  std::map<std::string, std::string> validationOutput;
  // check start and end times
  double startX = getProperty("StartX");
  double endX = getProperty("EndX");
  if (startX > endX) {
    validationOutput["StartX"] = "Start time is after the end time.";
  } else if (startX == endX) {
    validationOutput["StartX"] = "Start and end times are equal, there is no "
                                 "data to apply the algorithm to.";
  }
  double norm = getProperty("NormalizationIn");
  if (norm < 0.0) {
    validationOutput["NormalizationIn"] = "Normalization to use must be positive.";
  }
  return validationOutput;
}

/** Executes the algorithm
 *
 */
void EstimateMuonAsymmetryFromCounts::exec() {
  std::vector<int> spectra = getProperty("Spectra");
  // Get original workspace
  API::MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  std::string wsName = getProperty("WorkspaceName");
  if (wsName == "") {
    wsName = inputWS->getName();
  }
  auto numSpectra = inputWS->getNumberHistograms();
  // Create output workspace with same dimensions as input
  API::MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");
  if (inputWS != outputWS) {
    outputWS = create<API::MatrixWorkspace>(*inputWS);
  }
  bool extraData = getProperty("OutputUnNormData");
  API::MatrixWorkspace_sptr unnormWS = create<API::MatrixWorkspace>(*outputWS);
  double startX = getProperty("StartX");
  double endX = getProperty("EndX");
  const Mantid::API::Run &run = inputWS->run();
  double numGoodFrames = std::stod(run.getProperty("goodfrm")->value());
  if (numGoodFrames == 0) {
    g_log.warning("The data has no good frames, assuming a value of 1");
    numGoodFrames = 1;
  }
  // Share the X values
  for (size_t i = 0; i < static_cast<size_t>(numSpectra); ++i) {
    outputWS->setSharedX(i, inputWS->sharedX(i));
  }

  // No spectra specified = process all spectra
  if (spectra.empty()) {
    spectra = std::vector<int>(numSpectra);
    std::iota(spectra.begin(), spectra.end(), 0);
  }

  Progress prog(this, 0.0, 1.0, numSpectra + spectra.size());
  if (inputWS != outputWS) {

    // Copy all the Y and E data
    PARALLEL_FOR_IF(Kernel::threadSafe(*inputWS, *outputWS))
    for (int64_t i = 0; i < int64_t(numSpectra); ++i) {
      PARALLEL_START_INTERRUPT_REGION
      const auto index = static_cast<size_t>(i);
      outputWS->setSharedY(index, inputWS->sharedY(index));
      outputWS->setSharedE(index, inputWS->sharedE(index));
      prog.report();
      PARALLEL_END_INTERRUPT_REGION
    }
    PARALLEL_CHECK_INTERRUPT_REGION
  }

  // Do the specified spectra only
  auto specLength = static_cast<int>(spectra.size());
  std::vector<double> norm(specLength, 0.0);

  double normConst = getProperty("NormalizationIn");

  std::string status = (normConst == 0) ? "Estimate" : "Fixed";
  std::vector<std::string> methods(specLength, status);
  std::string baseName = (specLength > 1) ? wsName + "_spec_" : wsName;
  std::vector<std::string> wsNames(specLength, baseName);

  PARALLEL_FOR_IF(Kernel::threadSafe(*inputWS, *outputWS))
  for (int i = 0; i < specLength; ++i) {
    PARALLEL_START_INTERRUPT_REGION
    const auto specNum = static_cast<size_t>(spectra[i]);

    if (spectra[i] > static_cast<int>(numSpectra)) {
      g_log.error("The spectral index " + std::to_string(spectra[i]) + " is greater than the number of spectra!");
      throw std::invalid_argument("The spectral index " + std::to_string(spectra[i]) +
                                  " is greater than the number of spectra!");
    }
    // Calculate the normalised counts
    if (normConst == 0.0) {
      normConst = estimateNormalisationConst(inputWS->histogram(specNum), numGoodFrames, startX, endX);
    }
    if (spectra.size() > 1) {
      wsNames[i] += std::to_string(spectra[i]);
    }

    // Calculate the asymmetry
    outputWS->setHistogram(specNum, normaliseCounts(inputWS->histogram(specNum), numGoodFrames));
    if (extraData) {
      unnormWS->setSharedX(specNum, outputWS->sharedX(specNum));
      unnormWS->mutableY(specNum) = outputWS->y(specNum);
      unnormWS->mutableE(specNum) = outputWS->e(specNum);
    }
    outputWS->mutableY(specNum) /= normConst;
    outputWS->mutableY(specNum) -= 1.0;
    outputWS->mutableE(specNum) /= normConst;
    norm[i] = normConst;
    prog.report();
    PARALLEL_END_INTERRUPT_REGION
  }
  PARALLEL_CHECK_INTERRUPT_REGION
  if (extraData) {
    unnormWS->setYUnit("Asymmetry");
    setProperty("OutputUnNormWorkspace", unnormWS);
  }
  // update table
  Mantid::API::ITableWorkspace_sptr table = getProperty("NormalizationTable");
  if (table) {
    updateNormalizationTable(table, wsNames, norm, methods);
    setProperty("NormalizationTable", table);
  }
  // Update Y axis units
  outputWS->setYUnit("Asymmetry");

  std::string normString = std::accumulate(norm.begin() + 1, norm.end(), std::to_string(norm[0]),
                                           [](const std::string &currentString, double valueToAppend) {
                                             return currentString + ',' + std::to_string(valueToAppend);
                                           });
  MuonAlgorithmHelper::addSampleLog(outputWS, "analysis_asymmetry_norm", normString);

  setProperty("OutputWorkspace", outputWS);
}
} // namespace Mantid::Algorithms
