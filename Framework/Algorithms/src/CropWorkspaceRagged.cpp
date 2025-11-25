// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/CropWorkspaceRagged.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/MandatoryValidator.h"

#include <algorithm>

namespace {
std::vector<double> getSubVector(Mantid::MantidVec const &data, const int64_t &lowerIndex, const int64_t &upperIndex) {
  auto low = std::next(data.begin(), lowerIndex);
  auto up = std::next(data.begin(), upperIndex);
  // get new vectors
  std::vector<double> newData(low, up);
  return newData;
}

} // namespace

namespace Mantid::Algorithms {

using namespace Kernel;
using namespace API;

DECLARE_ALGORITHM(CropWorkspaceRagged)

/// Init function
void CropWorkspaceRagged::init() {
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("InputWorkspace", "", Direction::Input),
                  "The input workspace");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "Name to be given to the cropped workspace.");

  auto required = std::make_shared<MandatoryValidator<std::vector<double>>>();
  declareProperty(std::make_unique<ArrayProperty<double>>("XMin", required),
                  "The value(s) to start the cropping from. Should be either a "
                  "single value or a list.");
  declareProperty(std::make_unique<ArrayProperty<double>>("XMax", required),
                  "The value(s) to end the cropping at. Should be either a "
                  "single value or a list.");
}

/// Input validation
std::map<std::string, std::string> CropWorkspaceRagged::validateInputs() {
  std::map<std::string, std::string> issues;
  MatrixWorkspace_sptr ws = getProperty("InputWorkspace");
  auto numSpectra = ws->getNumberHistograms();
  std::vector<double> xMin = getProperty("XMin");
  std::vector<double> xMax = getProperty("XMax");
  if (xMin.size() == 0 || (xMin.size() != numSpectra && xMin.size() > 1)) {
    issues["XMin"] = "XMin must be a single value or one value per sepctrum.";
  }
  if (xMax.size() == 0 || (xMax.size() > 1 && xMax.size() != numSpectra)) {
    issues["XMax"] = "XMax must be a single value or one value per sepctrum.";
  }
  if (xMin.size() == 1 && xMax.size() == 1 && xMin[0] > xMax[0]) {
    issues["XMax"] = "XMax must be greater than XMin.";
  } else if (xMin.size() == 1 && xMax.size() > 1) {
    auto it = std::find_if(xMax.cbegin(), xMax.cend(), [&xMin](auto max) { return max < xMin[0]; });
    if (it != xMax.cend()) {
      issues["XMax"] = "XMax must be greater than XMin.";
      return issues;
    }
  } else if (xMin.size() > 1 && xMax.size() == 1) {
    auto it = std::find_if(xMin.cbegin(), xMin.cend(), [&xMax](auto min) { return min > xMax[0]; });
    if (it != xMin.cend()) {
      issues["XMin"] = "XMin must be less than XMax.";
      return issues;
    }
  } else if (xMin.size() > 1 && xMax.size() > 1) {
    for (size_t k = 0; k < xMin.size(); k++) {
      if (xMin[k] > xMax[k]) {
        issues["XMin"] = "XMin must be less than XMax.";
        return issues;
      }
    }
  }
  return issues;
} // namespace Algorithms

/// Exec function
void CropWorkspaceRagged::exec() {
  MatrixWorkspace_sptr ws = getProperty("InputWorkspace");
  auto numSpectra = ws->getNumberHistograms();
  // clone ws to copy logs etc.
  MatrixWorkspace_sptr outputWS = ws->clone();

  std::vector<double> xMin = getProperty("XMin");
  std::vector<double> xMax = getProperty("XMax");
  if (xMin.size() == 1) {
    auto value = xMin[0];
    xMin.assign(numSpectra, value);
  }
  if (xMax.size() == 1) {
    auto value = xMax[0];
    xMax.assign(numSpectra, value);
  }

  // Its easier to work with point data -> index is same for x, y, E
  MatrixWorkspace_sptr tmp = outputWS;
  bool histogram = false;
  if (outputWS->isHistogramData()) {
    auto alg = createChildAlgorithm("ConvertToPointData");
    alg->initialize();
    alg->setRethrows(true);
    alg->setProperty("InputWorkspace", outputWS);
    alg->setProperty("OutputWorkspace", outputWS);
    alg->execute();
    tmp = alg->getProperty("OutputWorkspace");
    histogram = true;
  }
  PARALLEL_FOR_IF(Kernel::threadSafe(*tmp, *outputWS))
  for (int64_t i = 0; i < int64_t(numSpectra); ++i) {
    PARALLEL_START_INTERRUPT_REGION
    auto points = tmp->points(i);
    auto &dataX = outputWS->dataX(i);
    auto &dataY = outputWS->dataY(i);
    auto &dataE = outputWS->dataE(i);

    // get iterators for cropped region using points
    auto low = std::lower_bound(points.begin(), points.end(), xMin[i]);
    auto up = std::upper_bound(points.begin(), points.end(), xMax[i]);
    // convert to index
    int64_t lowerIndex = std::distance(points.begin(), low);
    int64_t upperIndex = std::distance(points.begin(), up);

    // get new vectors
    std::vector<double> newY = getSubVector(dataY, lowerIndex, upperIndex);
    std::vector<double> newE = getSubVector(dataE, lowerIndex, upperIndex);
    if (histogram && upperIndex + (size_t)1 <= dataX.size()) {
      // the offset adds one to the upper index for histograms
      // only use the offset if the end is cropped
      upperIndex += 1;
    }
    std::vector<double> newX = getSubVector(dataX, lowerIndex, upperIndex);

    // resize the data
    dataX.resize(newX.size());
    dataY.resize(newY.size());
    dataE.resize(newE.size());

    // update the data
    outputWS->mutableX(i) = newX;
    outputWS->mutableY(i) = newY;
    outputWS->mutableE(i) = newE;
    PARALLEL_END_INTERRUPT_REGION
  }
  PARALLEL_CHECK_INTERRUPT_REGION

  setProperty("OutputWorkspace", outputWS);
}

} // namespace Mantid::Algorithms
