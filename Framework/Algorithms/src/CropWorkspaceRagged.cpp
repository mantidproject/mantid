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

namespace Mantid {
namespace Algorithms {

using namespace Kernel;
using namespace API;

DECLARE_ALGORITHM(CropWorkspaceRagged)

/// Init function
void CropWorkspaceRagged::init() {
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "The input workspace");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "Name to be given to the cropped workspace.");

  auto required = std::make_shared<MandatoryValidator<std::vector<double>>>();
  declareProperty(std::make_unique<ArrayProperty<double>>("XMin", required),
                  ".");
  declareProperty(std::make_unique<ArrayProperty<double>>("XMax", required),
                  "Y-axis data values for workspace (measures).");
}

/// Input validation
std::map<std::string, std::string> CropWorkspaceRagged::validateInputs() {
  std::map<std::string, std::string> issues;
  MatrixWorkspace_sptr ws = getProperty("InputWorkspace");
  auto numSpectra = ws->getNumberHistograms();
  std::vector<double> xMin = getProperty("XMin");
  std::vector<double> xMax = getProperty("XMax");
  if (xMin.size() == 0 || (xMin.size() != numSpectra && xMin.size() > 1)) {
    issues["XMin"] = "Either one XMin value must be a single value or one "
                     "value per spectrum.";
  }
  if (xMax.size() == 0 || (xMax.size() > 1 && xMax.size() != numSpectra)) {
    issues["XMax"] = "Either one XMax value must be a single value or one "
                     "value per spectrum.";
  }
  return issues;
}

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
  bool histogramData = false;
  if (outputWS->isHistogramData()) {
    auto alg = createChildAlgorithm("ConvertToPointData");
    alg->initialize();
    alg->setRethrows(true);
    alg->setProperty("InputWorkspace", outputWS);
    alg->setProperty("OutputWorkspace", outputWS);
    alg->execute();
    outputWS = alg->getProperty("OutputWorkspace");
    histogramData = true;
  }

  MatrixWorkspace_sptr tmp;
  for (int64_t i = 0; i < int64_t(numSpectra); ++i) {

    auto &dataX = outputWS->dataX(i);
    auto &dataY = outputWS->dataY(i);
    auto &dataE = outputWS->dataE(i);

    // get iterators for cropped region
    auto low = std::lower_bound(dataX.begin(), dataX.end(), xMin[i]);
    auto up = std::upper_bound(dataX.begin(), dataX.end(), xMax[i]);
    // convert to index
    auto lowerIndex = std::distance(dataX.begin(), low);
    auto upperIndex = std::distance(dataX.begin(), up);

    // get new vectors
    std::vector<double> newX(low, up);

    low = std::next(dataY.begin(), lowerIndex);
    up = std::next(dataY.begin(), upperIndex);
    std::vector<double> newY(low, up);

    low = std::next(dataE.begin(), lowerIndex);
    up = std::next(dataE.begin(), upperIndex);
    std::vector<double> newE(low, up);

    // resize the data
    dataX.resize(newX.size());
    dataY.resize(newY.size());
    dataE.resize(newE.size());

    // update the data
    outputWS->setPoints(i, std::move(newX));
    outputWS->setCounts(i, std::move(newY));
    outputWS->mutableE(i) = std::move(newE);
  }
  // return the same ws type as we recieved
  if (histogramData) {
    auto alg = createChildAlgorithm("ConvertToHistogram");
    alg->initialize();
    alg->setRethrows(true);
    alg->setProperty("InputWorkspace", outputWS);
    alg->setProperty("OutputWorkspace", outputWS);
    alg->execute();
    outputWS = alg->getProperty("OutputWorkspace");
  }

  setProperty("OutputWorkspace", outputWS);
}

} // namespace Algorithms
} // namespace Mantid
