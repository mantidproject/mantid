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
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidHistogramData/Histogram.h"
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
      std::make_unique<API::WorkspaceProperty<API::WorkspaceGroup>>("OutputWorkspace", "", Kernel::Direction::Output),
      "An output workspace.");
  declareProperty("CreateIntermediateWorkspaces", false, "Output workspaces showing intermediate working steps");
  declareProperty("StartWorkspaceIndex", EMPTY_INT(), m_validators["mustBeNonNegative"]);
  declareProperty("EndWorkspaceIndex", EMPTY_INT(), m_validators["mustBeNonNegative"]);
  declareProperty("EstimatedPeakExtent", EMPTY_DBL(), m_validators["mustBeGreaterThanZero"],
                  "Estimated PeakExtent of the peaks to be found");
  declareProperty("EstimatedPeakExtentNBins", EMPTY_INT(), m_validators["mustBeGreaterThanOne"],
                  "Optional: Estimated PeakExtent of the peaks to be found in number of bins");
  declareProperty("IOverSigmaThreshold", 3.0, m_validators["mustBeGreaterThanZero"],
                  "Minimum Signal/Noise ratio for a peak to be considered significant");
  declareProperty("MergeNearbyPeaks", true,
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
    PARALLEL_START_INTERRUPT_REGION
    performConvolution(i);
    PARALLEL_END_INTERRUPT_REGION
  }
  PARALLEL_CHECK_INTERRUPT_REGION

  outputResults();
}

void FindPeaksConvolve::initiateValidators() {
  m_validators.reserve(3); // Update depending upon number of validators.

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
  err_str += validateWorkspaceIndexInput();

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
  } else if (peakExtent == EMPTY_DBL() && peakExtentNBins == EMPTY_INT()) {
    err_str += "You must specify either peakExtent or peakExtentNBins. ";
  }
  return err_str;
}

const std::string FindPeaksConvolve::validateWorkspaceIndexInput() const {
  std::string err_str{};
  if (m_specNums.empty()) {
    err_str += "If both specified, EndWorkspaceIndex must be greater than StartWorkspaceIndex. ";
  } else {
    const int startWSIndex = m_specNums.front();
    const int endWSIndex = m_specNums.back();
    int specCount = static_cast<int>(m_inputDataWS->getNumberHistograms());
    if (startWSIndex > specCount - 1 || endWSIndex > specCount - 1) {
      err_str += "Specified Workspace indicies out of range. ";
    }
  }
  return err_str;
}

void FindPeaksConvolve::storeClassProperties() {
  m_inputDataWS = getProperty("InputWorkspace");
  m_createIntermediateWorkspaces = getProperty("CreateIntermediateWorkspaces");
  m_findHighestDatapointInPeak = getProperty("FindHighestDataPointInPeak");
  m_iOverSigmaThreshold = getProperty("IOverSigmaThreshold");
  m_mergeNearbyPeaks = getProperty("MergeNearbyPeaks");
  // can we assume all spectra are either centred or not?
  if (m_inputDataWS->x(0).size() != m_inputDataWS->y(0).size()) {
    m_centreBins = true;
  } else {
    m_centreBins = false;
  }

  int startWSIndex = getProperty("StartWorkspaceIndex");
  int endWSIndex = getProperty("EndWorkspaceIndex");
  int specCount = static_cast<int>(m_inputDataWS->getNumberHistograms());
  if (startWSIndex == EMPTY_INT()) {
    startWSIndex = 0;
  }
  if (endWSIndex == EMPTY_INT()) {
    endWSIndex = specCount - 1;
  }
  m_specCount = endWSIndex - startWSIndex + 1;
  m_specNums.resize(m_specCount);
  std::iota(std::begin(m_specNums), std::end(m_specNums), startWSIndex);
  m_peakResults.resize(m_specCount);
}

void FindPeaksConvolve::performConvolution(const size_t dataIndex) {
  const auto specNum{m_specNums[dataIndex]};
  const HistogramData::HistogramX *xData{&m_inputDataWS->x(specNum)};
  std::pair<const size_t, const bool> kernelBinCount{getKernelBinCount(xData)};
  if (kernelBinCount.second) {
    g_log.error("The kernel size for spectrum " + std::to_string(m_specNums[dataIndex]) +
                " exceeds the range of the x axis. Please reduce the peak extent.");
  } else {
    const Tensor1D kernel{createKernel(static_cast<int>(kernelBinCount.first))};
    const auto binCount{m_inputDataWS->getNumberBins(specNum)};
    // Edge handling is performed by padding the input data with 0 values. Each convolution requires a padding of kernel
    // size + 1. The 1st conv is performed with a kernel of size n, the second size n/2. The resultant pad is split
    // either side of the data.
    const double paddingSize = (std::ceil(static_cast<double>(kernelBinCount.first) * 1.5) - 2) / 2;
    const TensorMap_const yData{&m_inputDataWS->y(specNum).front(), binCount};
    Eigen::array<std::pair<double, double>, 1> paddings{
        std::make_pair(std::ceil(paddingSize), std::floor(paddingSize))};
    const auto yData_padded{yData.pad(paddings)};
    const Eigen::array<ptrdiff_t, 1> dims({0});
    const Tensor1D yConvOutput{yData_padded.convolve(kernel, dims)};
    const auto eData{TensorMap_const{&m_inputDataWS->e(specNum).front(), binCount}.pad(paddings)};
    const Tensor1D eConvOutput{eData.square().convolve(kernel.square(), dims).sqrt()};
    const Tensor1D smoothKernel{
        createSmoothKernel(static_cast<size_t>(std::ceil(static_cast<double>(kernelBinCount.first) / 2.0)))};
    Tensor1D iOverSig{(yConvOutput / eConvOutput).unaryExpr([](double val) { return std::isfinite(val) ? val : 0.0; })};
    const Tensor1D iOverSigConvOutput{iOverSig.convolve(smoothKernel, dims)};
    extractPeaks(dataIndex, iOverSigConvOutput, xData, yData, kernelBinCount.first / 2);
    if (m_createIntermediateWorkspaces) {
      auto wsNames = createIntermediateWorkspaces(dataIndex, kernel, iOverSigConvOutput, xData);
      std::lock_guard<std::mutex> lock(m_mtx);
      m_intermediateWsNames.insert(m_intermediateWsNames.end(), wsNames.cbegin(), wsNames.cend());
    }
  }
}

Tensor1D FindPeaksConvolve::createKernel(const int binCount) const {
  Tensor1D kernel(binCount);
  for (int i{0}; i < binCount; i++) {
    // create integrating shoebox kernel with central positive region & negative background shell with ~the same
    // elements in each.
    if (i < std::ceil(binCount) * 0.25 || i >= binCount * 0.75) {
      kernel.data()[i] = -1.0;
    } else {
      kernel.data()[i] = 1.0;
    }
  }
  return kernel;
}

Tensor1D FindPeaksConvolve::createSmoothKernel(const size_t kernelSize) const {
  Tensor1D kernel(kernelSize);
  for (size_t i{0}; i < kernelSize; i++) {
    kernel.data()[i] = 1.0 / static_cast<double>(kernelSize);
  }
  return kernel;
}

std::pair<size_t, bool> FindPeaksConvolve::getKernelBinCount(const HistogramData::HistogramX *xData) const {
  const double peakExtent = getProperty("EstimatedPeakExtent");
  const int peakExtentNBins = getProperty("EstimatedPeakExtentNBins");
  bool sizeError{false};

  size_t kernelBinCount{0};
  if (peakExtent != EMPTY_DBL()) {
    const double x1{xData->rawData()[static_cast<size_t>(std::floor((xData->size() - 1) / 2))]};
    const double x2{xData->rawData()[static_cast<size_t>(std::floor((xData->size() - 1) / 2)) + 1]};
    kernelBinCount = static_cast<size_t>(std::floor(peakExtent * 2 / (x2 - x1)));
  } else {
    kernelBinCount = static_cast<size_t>(peakExtentNBins);
  }
  if (kernelBinCount > xData->size()) {
    sizeError = true;
  }
  return std::make_pair(kernelBinCount, sizeError);
}

Eigen::VectorXd FindPeaksConvolve::centreBinsXData(const HistogramData::HistogramX *xData) const {
  Eigen::VectorXd xDataVec{0.5 * (EigenMap_const(&xData->front(), xData->size() - 1) +
                                  EigenMap_const(&xData->front() + 1, xData->size() - 1))};
  return xDataVec;
}

void FindPeaksConvolve::extractPeaks(const size_t dataIndex, const Tensor1D &iOverSigma,
                                     const HistogramData::HistogramX *xData, const TensorMap_const &yData,
                                     const size_t peakExtentBinNumber) {
  int dataPointCount{0};
  std::pair<int, double> dataRegionMax{0, 0.0};
  std::vector<FindPeaksConvolve::PeakResult> peakCentres;
  for (auto i{0}; i < iOverSigma.size(); i++) {
    if (iOverSigma(i) > m_iOverSigmaThreshold) {
      if (dataPointCount == 0) {
        dataRegionMax = std::make_pair(i, iOverSigma(i));
      } else {
        if (iOverSigma.data()[i] > dataRegionMax.second) {
          dataRegionMax = {i, iOverSigma(i)};
        }
      }
      dataPointCount++;
    } else if (iOverSigma(i) <= 0 || !m_mergeNearbyPeaks || i == iOverSigma.size() - 1) {
      if (dataPointCount >= 2) {
        size_t rawPeakIndex{findPeakInRawData(dataRegionMax.first, yData, peakExtentBinNumber)};
        peakCentres.push_back(FindPeaksConvolve::PeakResult{getXDataValue(xData, rawPeakIndex),
                                                            yData.data()[rawPeakIndex], dataRegionMax.second});
      }
      if (dataPointCount > 0) {
        dataPointCount = 0;
        dataRegionMax = std::make_pair(0, 0.0);
      }
    }
  }
  storePeakResults(dataIndex, peakCentres);
}

double FindPeaksConvolve::getXDataValue(const HistogramData::HistogramX *xData, const size_t xIndex) const {
  if (m_centreBins) {
    return (xData->rawData()[xIndex] + xData->rawData()[xIndex + 1]) / 2;
  } else {
    return xData->rawData()[xIndex];
  }
}

void FindPeaksConvolve::storePeakResults(const size_t dataIndex,
                                         std::vector<FindPeaksConvolve::PeakResult> &peakCentres) {
  const size_t peakCount = peakCentres.size();
  if (peakCount) {
    if (peakCount > m_maxPeakCount) {
      m_maxPeakCount = peakCount;
    }
    std::lock_guard<std::mutex> lock(m_mtx);
    m_peakResults[dataIndex] = std::move(peakCentres);
  }
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
    generateNormalPDF(static_cast<int>(peakExtentBinNumber));
    const auto weightedYData = EigenMap_const(yData.data() + sliceStart, adjPeakExtentBinNumber)
                                   .cwiseProduct(EigenMap_const(m_pdf.data() + startAdj, adjPeakExtentBinNumber));
    weightedYData.maxCoeff(&maxIndex);
  }
  return static_cast<size_t>(maxIndex) + sliceStart;
}

void FindPeaksConvolve::generateNormalPDF(const int peakExtentBinNumber) {
  if (m_pdf.size() == 0) {
    std::lock_guard<std::mutex> lock(m_mtx);
    m_pdf.resize(peakExtentBinNumber);
    boost::math::normal_distribution<> dist(0.0,
                                            peakExtentBinNumber / 2.0); // assures 2 stddevs in the resultant vector
    const int meanIdx{peakExtentBinNumber / 2};
    for (int i{0}; i < peakExtentBinNumber; ++i) {
      int x{i - meanIdx};
      m_pdf(i) = boost::math::pdf(dist, x);
    }
  }
}

std::vector<std::string> FindPeaksConvolve::createIntermediateWorkspaces(const size_t dataIndex, const Tensor1D &kernel,
                                                                         const Tensor1D &iOverSigma,
                                                                         const HistogramData::HistogramX *xData) {
  std::unique_ptr<EigenMap_const> xDataMap;
  Eigen::VectorXd xDataCentredBins;
  if (m_centreBins) {
    xDataCentredBins = centreBinsXData(xData);
    xDataMap = std::make_unique<EigenMap_const>(xDataCentredBins.data(), xDataCentredBins.size());
  } else {
    xDataMap = std::make_unique<EigenMap_const>(&xData->front(), xData->size());
  }

  const std::string iOverSigmaOutputName =
      m_inputDataWS->getName() + "_" + "iOverSigma" + "_" + std::to_string(m_specNums[dataIndex]);
  outputIntermediateWorkspace(iOverSigmaOutputName,
                              std::vector<double>(xDataMap->data(), xDataMap->data() + xDataMap->rows()),
                              std::vector<double>(iOverSigma.data(), iOverSigma.data() + iOverSigma.size()));

  std::vector<double> xKernelData(kernel.size());
  std::iota(std::begin(xKernelData), std::end(xKernelData), 0.0);
  const std::string kernelOutputName =
      m_inputDataWS->getName() + "_" + "kernel" + "_" + std::to_string(m_specNums[dataIndex]);
  outputIntermediateWorkspace(kernelOutputName, std::move(xKernelData),
                              std::vector<double>(kernel.data(), kernel.data() + kernel.size()));
  return std::vector<std::string>{iOverSigmaOutputName, kernelOutputName};
}

void FindPeaksConvolve::outputIntermediateWorkspace(const std::string &outputWsName, const std::vector<double> &xData,
                                                    const std::vector<double> &yData) {
  API::Algorithm_sptr alg{createChildAlgorithm("CreateWorkspace")};
  alg->setProperty("OutputWorkspace", outputWsName);
  alg->setProperty("DataX", xData);
  alg->setProperty("DataY", yData);
  alg->execute();
  API::MatrixWorkspace_sptr algOutput = alg->getProperty("OutputWorkspace");
  API::AnalysisDataService::Instance().addOrReplace(outputWsName, algOutput);
}

void FindPeaksConvolve::outputResults() {
  const std::vector<std::string> outputTblNames{"PeakCentre", "PeakYPosition", "PeakIOverSigma"};
  std::unordered_map<std::string, API::ITableWorkspace_sptr> outputTbls{createOutputTables(outputTblNames)};

  std::string noPeaksStr{populateOutputWorkspaces(outputTblNames, outputTbls)};
  if (noPeaksStr != "") {
    g_log.warning("No peaks found for spectrum index: " + noPeaksStr);
  }

  API::WorkspaceGroup_sptr groupWs{groupOutputWorkspaces("resultsOutput", outputTblNames)};
  setProperty("OutputWorkspace", groupWs);

  if (m_intermediateWsNames.size() > 0) {
    std::sort(m_intermediateWsNames.begin(), m_intermediateWsNames.end(),
              [](const std::string &a, const std::string &b) {
                return std::stoi(a.substr(a.find_last_of("_") + 1, a.size())) <
                       std::stoi(b.substr(b.find_last_of("_") + 1, b.size()));
              });
    auto groupedOutput = groupOutputWorkspaces("IntermediateWorkspaces", m_intermediateWsNames);
    API::AnalysisDataService::Instance().addOrReplace("IntermediateWorkspaces", groupedOutput);
  }
}

std::unordered_map<std::string, API::ITableWorkspace_sptr>
FindPeaksConvolve::createOutputTables(const std::vector<std::string> &outputTblNames) {
  std::unordered_map<std::string, API::ITableWorkspace_sptr> outputTbls;
  for (const auto &outputTblName : outputTblNames) {
    auto tbl = outputTbls.emplace(outputTblName, API::WorkspaceFactory::Instance().createTable("TableWorkspace"));
    tbl.first->second->addColumn("int", "SpecIndex");
    for (size_t i{0}; i < m_maxPeakCount; i++) {
      tbl.first->second->addColumn("double", outputTblName + "_" + std::to_string(i));
    }
    API::AnalysisDataService::Instance().addOrReplace(outputTblName, tbl.first->second);
  }
  return outputTbls;
}

API::WorkspaceGroup_sptr FindPeaksConvolve::groupOutputWorkspaces(const std::string &outputName,
                                                                  const std::vector<std::string> &outputTblNames) {
  API::Algorithm_sptr alg{createChildAlgorithm("GroupWorkspaces")};
  alg->initialize();
  alg->setProperty("InputWorkspaces", outputTblNames);
  alg->setProperty("OutputWorkspace", outputName);
  alg->execute();
  API::WorkspaceGroup_sptr groupWs = alg->getProperty("OutputWorkspace");
  return groupWs;
}

std::string FindPeaksConvolve::populateOutputWorkspaces(
    const std::vector<std::string> &outputTblNames,
    const std::unordered_map<std::string, API::ITableWorkspace_sptr> &outputTbls) {
  std::string noPeaksStr{""};
  for (size_t i{0}; i < m_peakResults.size(); i++) {
    const auto spec = std::move(m_peakResults[i]);
    if (!spec.empty()) {
      for (const auto &outputTblName : outputTblNames) {
        auto tbl = outputTbls.find(outputTblName);
        API::TableRow row{tbl->second->appendRow()};
        row << m_specNums[i];
        for (size_t peak_i{0}; peak_i < m_maxPeakCount; peak_i++) {
          if (peak_i < spec.size()) {
            const auto &peak = spec[peak_i];
            row << peak.getAttribute(outputTblName);
          } else {
            row << std::nan("");
          }
        }
      }
    } else {
      noPeaksStr += std::to_string(i) + ", ";
    }
  }
  return noPeaksStr;
}

double FindPeaksConvolve::PeakResult::getAttribute(const std::string &attrString) const {
  if (attrString == "PeakCentre") {
    return centre;
  } else if (attrString == "PeakYPosition") {
    return height;
  } else if (attrString == "PeakIOverSigma") {
    return iOverSigma;
  } else {
    return -1.0;
  }
}
} // namespace Mantid::Algorithms
