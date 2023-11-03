// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
// NScD Oak Ridge National Laboratory, European Spallation Source,
// Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAlgorithms/FindPeaksConvolve.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/IValidator.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/MultiThreaded.h"

#include <algorithm>
#include <boost/math/distributions/normal.hpp>
#include <cmath>

namespace {
Mantid::Kernel::Logger g_log("FindPeaksConvolve");
}

namespace Mantid::Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(FindPeaksConvolve)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string FindPeaksConvolve::name() const { return "FindPeaksConvolve"; }

/// Algorithm's version for identification. @see Algorithm::version
int FindPeaksConvolve::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string FindPeaksConvolve::category() const { return "Optimization\\PeakFinding"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string FindPeaksConvolve::summary() const {
  return "Finds peaks in a dataset through the use of a convolution vector";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void FindPeaksConvolve::init() {
  initiateValidators();

  declareProperty(std::make_unique<API::WorkspaceProperty<>>("InputWorkspace", "", Kernel::Direction::Input),
                  "An input workspace.");
  declareProperty(
      std::make_unique<API::WorkspaceProperty<API::ITableWorkspace>>("OutputWorkspace", "", Kernel::Direction::Output),
      "An output workspace.");
  declareProperty("CreateIntermediateWorkspaces", false, "Output workspaces showing intermediate working steps");
  declareProperty("WorkspaceIndex", EMPTY_INT(), m_validators["mustBeNonNegative"],
                  "If set, only this spectrum will be searched for peaks "
                  "(otherwise all are)");
  declareProperty("EstimatedPeakExtent", EMPTY_DBL(), m_validators["mustBeGreaterThanZero"],
                  "Estimated PeakExtent of the peaks to be found");
  declareProperty("EstimatedPeakExtentNBins", EMPTY_INT(), m_validators["mustBeGreaterThanOne"],
                  "Optional: Estimated PeakExtent of the peaks to be found in number of bins");
  declareProperty("IOversigmaThreshold", 2.0, m_validators["mustBeGreaterThanZero"],
                  "Minimum Signal/Noise ratio for a peak to be considered significant");
  declareProperty("PerformBinaryClosing", true,
                  "Attempt to remove inflections in the data, where a local"
                  " minima/maxima occurs which is not signficiant enough to be considered a peak");
  declareProperty("FindHighestDataPointInPeak", false,
                  "When searching for peaks in the raw data around the iOverSigma maxima, take the highest value,"
                  " rather than favouring datapoints closer to the maxima");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void FindPeaksConvolve::exec() {
  storeClassProperties();
  auto validation_res = secondaryValidation();
  if (validation_res.second != 0) {
    throw std::invalid_argument("Validation error: " + validation_res.first);
  }
  PARALLEL_FOR_IF(Kernel::threadSafe(*m_inputDataWS))
  for (int i = 0; i < static_cast<int>(m_specCount); i++) {
    performConvolution(i);
  }
  outputResults();
}

void FindPeaksConvolve::initiateValidators() {
  m_validators.reserve(2); // Update depending upon number of validators.

  auto mustBeNonNegative{std::make_shared<Kernel::BoundedValidator<int>>()};
  mustBeNonNegative->setLower(0);
  m_validators.emplace(std::make_pair("mustBeNonNegative", std::move(mustBeNonNegative)));

  auto mustBeGreaterThanZero{std::make_shared<Kernel::BoundedValidator<double>>()};
  mustBeGreaterThanZero->setLowerExclusive(0.0);
  m_validators.emplace("mustBeGreaterThanZero", std::move(mustBeGreaterThanZero));

  auto mustBeGreaterThanOne{std::make_shared<Kernel::BoundedValidator<int>>()};
  mustBeGreaterThanOne->setLowerExclusive(1);
  m_validators.emplace("mustBeGreaterThanOne", std::move(mustBeGreaterThanOne));
}

std::pair<std::string, int> FindPeaksConvolve::secondaryValidation() const {
  std::string err_str{};
  int err_code{0};

  err_str += validatePeakExtentInput();

  if (err_str != "") {
    err_code = 1;
  }
  return std::make_pair(err_str, err_code);
}

const std::string FindPeaksConvolve::validatePeakExtentInput() const {
  std::string err_str{};
  const double peakExtent = getProperty("EstimatedPeakExtent");
  const int peakExtentNBins = getProperty("EstimatedPeakExtentNBins");
  if (peakExtent != EMPTY_DBL() && peakExtentNBins != EMPTY_INT()) {
    err_str += "Peak Extent has been given in x units and in number of bins. Please specify one or the other. ";
  }

  return err_str;
}

void FindPeaksConvolve::storeClassProperties() {
  m_inputDataWS = getProperty("InputWorkspace");
  m_createIntermediateWorkspaces = getProperty("CreateIntermediateWorkspaces");
  m_findHighestDatapointInPeak = getProperty("FindHighestDataPointInPeak");
  m_iOverSigmaThreshold = getProperty("IOversigmaThreshold");
  m_performBinaryClosing = getProperty("PerformBinaryClosing");

  int wsIndex = getProperty("WorkspaceIndex");
  if (wsIndex == EMPTY_INT()) {
    m_specCount = static_cast<int>(m_inputDataWS->getNumberHistograms());
    m_specNums.resize(m_specCount);
    std::iota(std::begin(m_specNums), std::end(m_specNums), 0);
  } else {
    m_specCount = 1;
    m_specNums.push_back(wsIndex);
  }
  m_peakResults.resize(m_specCount);
}

void FindPeaksConvolve::performConvolution(const size_t dataIndex) {
  const auto specNum{m_specNums[dataIndex]};
  const HistogramData::HistogramX *xData{&m_inputDataWS->x(specNum)};
  const auto kernelBinCount{getKernelBinCount(xData)};
  const Tensor1D kernel{createKernel(static_cast<int>(kernelBinCount))};

  const auto binCount{m_inputDataWS->getNumberBins(specNum)};
  const double paddingSize = (std::ceil(static_cast<double>(kernelBinCount) * 1.5) - 2) / 2;
  const TensorMap_const yData{&m_inputDataWS->y(specNum).front(), binCount};
  Eigen::array<std::pair<double, double>, 1> paddings{std::make_pair(std::ceil(paddingSize), std::floor(paddingSize))};
  const auto yData_padded{yData.pad(paddings)};
  const Eigen::array<ptrdiff_t, 1> dims({0});
  const Tensor1D yConvOutput{yData_padded.convolve(kernel, dims)};

  const auto eData{TensorMap_const{&m_inputDataWS->e(specNum).front(), binCount}.pad(paddings)};
  const Tensor1D eConvOutput{eData.square().convolve(kernel.square(), dims).sqrt()};

  const Tensor1D smoothKernel{
      createSmoothKernel(static_cast<size_t>(std::ceil(static_cast<double>(kernelBinCount) / 2.0)))};
  const Tensor1D iOverSigConvOutput{(yConvOutput / eConvOutput).convolve(smoothKernel, dims)};

  std::unique_ptr<EigenMap_const> xDataPostConv;
  Eigen::VectorXd xDataCentredBins;
  if (xData->size() != static_cast<size_t>(yData.size())) {
    xDataCentredBins = centreBinsXData(xData);
    xDataPostConv = std::make_unique<EigenMap_const>(EigenMap_const{xDataCentredBins.data(), xDataCentredBins.size()});
  } else {
    xDataPostConv = std::make_unique<EigenMap_const>(EigenMap_const(&xData->front(), xData->size()));
  }
  extractPeaks(dataIndex, iOverSigConvOutput, *xDataPostConv, yData, kernelBinCount / 2);

  if (m_createIntermediateWorkspaces) {
    createIntermediateWorkspaces(dataIndex, kernel, iOverSigConvOutput, *xDataPostConv);
  }
}

Tensor1D FindPeaksConvolve::createKernel(const int binCount) {
  Tensor1D kernel(binCount);
  for (int i{0}; i < binCount; i++) {
    // consider if these 0.25/0.75 values should be inputs
    if (i < std::ceil(binCount) * 0.25 || i >= binCount * 0.75) {
      kernel.data()[i] = -1.0;
    } else {
      kernel.data()[i] = 1.0;
    }
  }
  return kernel;
}

Tensor1D FindPeaksConvolve::createSmoothKernel(const size_t kernelSize) {
  Tensor1D kernel(kernelSize);
  for (size_t i{0}; i < kernelSize; i++) {
    kernel.data()[i] = 1.0 / static_cast<double>(kernelSize);
  }
  return kernel;
}

size_t FindPeaksConvolve::getKernelBinCount(const HistogramData::HistogramX *xData) const {
  // What is the minimum number of data points this works for - add validation
  const double peakExtent = getProperty("EstimatedPeakExtent");
  const int peakExtentNBins = getProperty("EstimatedPeakExtentNBins");

  if (peakExtent != EMPTY_DBL()) {
    const double x1{xData->rawData()[static_cast<size_t>(std::floor((xData->size() - 1) / 2))]};
    const double x2{xData->rawData()[static_cast<size_t>(std::floor((xData->size() - 1) / 2)) + 1]};
    return static_cast<size_t>(std::floor(peakExtent * 2 / (x2 - x1)));
  } else {
    return static_cast<size_t>(peakExtentNBins);
  }
}

Eigen::VectorXd FindPeaksConvolve::centreBinsXData(const HistogramData::HistogramX *xData) {
  Eigen::VectorXd xDataVec{0.5 * (EigenMap_const(&xData->front(), xData->size() - 1) +
                                  EigenMap_const(&xData->front() + 1, xData->size() - 1))};
  return xDataVec;
}

void FindPeaksConvolve::extractPeaks(const size_t dataIndex, const Tensor1D &iOverSigma, const EigenMap_const &xData,
                                     const TensorMap_const &yData, const size_t peakExtentBinNumber) {
  auto filteredData = filterDataForSignificantPoints(iOverSigma);
  int dataPointCount{0};
  std::pair<int, double> dataRegionMax{0, 0.0};
  std::vector<FindPeaksConvolve::PeakResult> peakCentres;
  for (const auto &dataPoint : filteredData) {
    if (dataPoint.second != 0) {
      if (dataPointCount == 0) {
        dataRegionMax = dataPoint;
      } else {
        if (dataPoint.second > dataRegionMax.second) {
          dataRegionMax = dataPoint;
        }
      }
      dataPointCount++;
    } else {
      if (dataPointCount >= 2) {
        size_t rawPeakIndex{findPeakInRawData(dataRegionMax.first, yData, peakExtentBinNumber)};
        peakCentres.push_back(
            FindPeaksConvolve::PeakResult{xData[rawPeakIndex], yData.data()[rawPeakIndex], dataRegionMax.second});
      }
      if (dataPointCount > 0) {
        dataPointCount = 0;
        dataRegionMax = std::make_pair(0, 0.0);
      }
    }
  }
  storePeakResults(dataIndex, peakCentres);
}

void FindPeaksConvolve::storePeakResults(const size_t dataIndex,
                                         std::vector<FindPeaksConvolve::PeakResult> &peakCentres) {
  const size_t peakCount = peakCentres.size();
  if (peakCount) {
    if (peakCount > m_maxPeakCount) {
      m_maxPeakCount = peakCount;
    }
    m_peakResults[dataIndex] = std::move(peakCentres);
  }
}

std::vector<std::pair<const int, const double>>
FindPeaksConvolve::filterDataForSignificantPoints(const Tensor1D &iOverSigma) {
  std::vector<std::pair<const int, const double>> closedData;
  for (auto i{0}; i < iOverSigma.size(); i++) {
    if (iOverSigma(i) > m_iOverSigmaThreshold) {
      closedData.emplace_back(i, iOverSigma(i));
    } else if (iOverSigma(i) <= 0 || !m_performBinaryClosing) {
      closedData.emplace_back(i, 0);
    }
  }
  closedData.emplace_back(static_cast<int>(closedData.size()), 0); // Guarentee data region close
  return closedData;
}

size_t FindPeaksConvolve::findPeakInRawData(const int xIndex, const TensorMap_const &yData,
                                            size_t peakExtentBinNumber) {
  peakExtentBinNumber = (peakExtentBinNumber % 2 == 0) ? peakExtentBinNumber + 1 : peakExtentBinNumber;
  int sliceStart{xIndex - static_cast<int>(std::floor(static_cast<double>(peakExtentBinNumber) / 2.0))};
  size_t adjPeakExtentBinNumber{peakExtentBinNumber};
  int startAdj{0};

  if (sliceStart < 0) {
    startAdj = -sliceStart;
    adjPeakExtentBinNumber = peakExtentBinNumber - startAdj;
    sliceStart = 0;
  }
  if (sliceStart + adjPeakExtentBinNumber > static_cast<size_t>(yData.size()) - 1) {
    adjPeakExtentBinNumber = static_cast<size_t>(yData.size()) - sliceStart;
  }

  Eigen::VectorXd::Index maxIndex;
  if (m_findHighestDatapointInPeak) {
    const auto unweightedYData = EigenMap_const(yData.data() + sliceStart, adjPeakExtentBinNumber);
    unweightedYData.maxCoeff(&maxIndex);
  } else {
    Eigen::VectorXd pdf{generateNormalPDF(static_cast<int>(peakExtentBinNumber))};
    const auto weightedYData = EigenMap_const(yData.data() + sliceStart, adjPeakExtentBinNumber)
                                   .cwiseProduct(EigenMap_const(pdf.data() + startAdj, adjPeakExtentBinNumber));
    weightedYData.maxCoeff(&maxIndex);
  }
  return static_cast<size_t>(maxIndex) + sliceStart;
}

Eigen::VectorXd FindPeaksConvolve::generateNormalPDF(const int peakExtentBinNumber) {
  // assert vector has odd size.
  Eigen::VectorXd pdf{peakExtentBinNumber};
  boost::math::normal_distribution<> dist(0.0, peakExtentBinNumber / 2.0); // assures 2 stddevs in the resultant vector
  const int meanIdx{peakExtentBinNumber / 2};
  for (int i{0}; i < peakExtentBinNumber; ++i) {
    int x{i - meanIdx};
    pdf(i) = boost::math::pdf(dist, x);
  }
  return pdf;
}

void FindPeaksConvolve::createIntermediateWorkspaces(const size_t dataIndex, const Tensor1D &kernel,
                                                     const Tensor1D &iOverSigma, const EigenMap_const &xData) {
  API::Algorithm_sptr alg{createChildAlgorithm("CreateWorkspace")};

  alg->setProperty("OutputWorkspace", "iOverSigma");
  alg->setProperty("DataX", std::vector<double>(xData.data(), xData.data() + xData.rows()));
  alg->setProperty("DataY", std::vector<double>(iOverSigma.data(), iOverSigma.data() + iOverSigma.size()));
  alg->execute();
  API::MatrixWorkspace_sptr algOutput = alg->getProperty("OutputWorkspace");
  API::AnalysisDataService::Instance().addOrReplace(
      m_inputDataWS->getName() + "_iOverSigma_" + std::to_string(m_specNums[dataIndex]), algOutput);

  std::vector<double> xKernelData(kernel.size());
  std::iota(std::begin(xKernelData), std::end(xKernelData), 0.0);
  alg->resetProperties();
  alg->setProperty("OutputWorkspace", "kernel");
  alg->setProperty("DataX", std::move(xKernelData));
  alg->setProperty("DataY", std::vector<double>(kernel.data(), kernel.data() + kernel.size()));
  API::MatrixWorkspace_sptr algKernelOutput = alg->getProperty("OutputWorkspace");
  API::AnalysisDataService::Instance().addOrReplace(
      m_inputDataWS->getName() + "_kernel_" + std::to_string(m_specNums[dataIndex]), algKernelOutput);
}

void FindPeaksConvolve::outputResults() {
  API::ITableWorkspace_sptr table{API::WorkspaceFactory::Instance().createTable("TableWorkspace")};
  table->addColumn("int", "SpecIndex");
  for (size_t i{0}; i < m_maxPeakCount; i++) {
    table->addColumn("double", "PeakCentre_" + std::to_string(i));
    table->addColumn("double", "PeakHeight_" + std::to_string(i));
    table->addColumn("double", "PeakIOverSigma_" + std::to_string(i));
  }

  std::string noPeaksStr{""};
  for (size_t i{0}; i < m_peakResults.size(); i++) {
    const auto spec = std::move(m_peakResults[i]);
    if (!spec.empty()) {
      API::TableRow row{table->appendRow()};
      row << m_specNums[i];
      for (const auto &peak : spec) {
        row << peak.centre << peak.height << peak.iOverSigma;
      }
    } else {
      noPeaksStr += std::to_string(i) + ", ";
    }
  }
  if (noPeaksStr != "") {
    g_log.notice("No peaks found for spectrum index: " + noPeaksStr);
  }
  setProperty("OutputWorkspace", table);
}
} // namespace Mantid::Algorithms