// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/Stitch1D.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAlgorithms/RunCombinationHelpers/RunCombinationHelper.h"
#include "MantidHistogramData/HistogramDx.h"
#include "MantidHistogramData/HistogramE.h"
#include "MantidHistogramData/HistogramX.h"
#include "MantidHistogramData/HistogramY.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/RebinParamsValidator.h"
#include "MantidKernel/Strings.h"

#include <algorithm>
#include <boost/math/special_functions.hpp>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using Mantid::HistogramData::HistogramE;
using Mantid::HistogramData::HistogramY;

namespace {
/// Check if a double is not zero and returns a bool indicating success
bool isNonzero(double i) { return (0. != i); }
} // namespace

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(Stitch1D)

// Overlap Value tolerance is 1.e-9:
// This is required for machine precision reasons. Used to adjust overlap
// value value to be inclusive of bin boundaries if sitting ontop of the bin
// boundaries.

/** Zero out all y and e data outside the region starting from a1 to a2.
 * @param a1 : start workspace index
 * @param a2 : end workspace index
 * @param source : Workspace providing the source data.
 */
MatrixWorkspace_sptr Stitch1D::maskAllBut(size_t a1, size_t a2,
                                          MatrixWorkspace_sptr &source) {
  MatrixWorkspace_sptr outWS = source->clone();
  int histogramCount = static_cast<int>(source->getNumberHistograms());
  PARALLEL_FOR_IF(Kernel::threadSafe(*source, *outWS))
  for (int i = 0; i < histogramCount; ++i) {
    PARALLEL_START_INTERUPT_REGION
    // Copy over the bin boundaries
    outWS->setSharedX(i, source->sharedX(i));
    // Copy over the data
    const auto &sourceY = source->y(i);
    const auto &sourceE = source->e(i);

    // initially zero - out the data.
    outWS->mutableY(i) = HistogramY(sourceY.size(), 0.);
    outWS->mutableE(i) = HistogramE(sourceE.size(), 0.);

    auto &newY = outWS->mutableY(i);
    auto &newE = outWS->mutableE(i);

    // Copy over the non-zero stuff
    std::copy(sourceY.begin() + a1, sourceY.begin() + a2, newY.begin() + a1);
    std::copy(sourceE.begin() + a1, sourceE.begin() + a2, newE.begin() + a1);

    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
  return outWS;
}

/** Zero out all y and e data inside the region starting from a1 to a2.
 * @param a1 : start workspace index
 * @param a2 : end workspace index
 * @param source : Workspace to mask.
 */
void Stitch1D::maskInPlace(size_t a1, size_t a2, MatrixWorkspace_sptr &source) {
  int histogramCount = static_cast<int>(source->getNumberHistograms());
  int start = static_cast<int>(a1);
  int stop = static_cast<int>(a2);
  PARALLEL_FOR_IF(Kernel::threadSafe(*source))
  for (int i = 0; i < histogramCount; ++i) {
    PARALLEL_START_INTERUPT_REGION
    // Copy over the data
    auto &sourceY = source->mutableY(i);
    auto &sourceE = source->mutableE(i);
    for (int i = start; i < stop; ++i) {
      sourceY[i] = 0.;
      sourceE[i] = 0.;
    }

    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void Stitch1D::init() {

  declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "LHSWorkspace", "", Direction::Input),
                  "LHS input workspace.");
  declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "RHSWorkspace", "", Direction::Input),
                  "RHS input workspace, must be same type as LHSWorkspace "
                  "(histogram or point data).");
  declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "Output stitched workspace.");
  declareProperty(make_unique<PropertyWithValue<double>>(
                      "StartOverlap", Mantid::EMPTY_DBL(), Direction::Input),
                  "Start overlap x-value in units of x-axis.");
  declareProperty(make_unique<PropertyWithValue<double>>(
                      "EndOverlap", Mantid::EMPTY_DBL(), Direction::Input),
                  "End overlap x-value in units of x-axis.");
  declareProperty(make_unique<ArrayProperty<double>>(
                      "Params", boost::make_shared<RebinParamsValidator>(true)),
                  "Rebinning Parameters. See Rebin for format. If only a "
                  "single value is provided, start and end are taken from "
                  "input workspaces.");
  declareProperty(make_unique<PropertyWithValue<bool>>("ScaleRHSWorkspace",
                                                       true, Direction::Input),
                  "If true, the RHSWorkspace will be scaled, if false the "
                  "LHSWorkspace will be scaled; scaling means multiplied by "
                  "the scale factor");
  declareProperty(make_unique<PropertyWithValue<bool>>("UseManualScaleFactor",
                                                       false, Direction::Input),
                  "True to use a provided value for the scale factor.");
  auto manualScaleFactorValidator =
      boost::make_shared<BoundedValidator<double>>();
  manualScaleFactorValidator->setLower(0);
  manualScaleFactorValidator->setExclusive(true);
  declareProperty(make_unique<PropertyWithValue<double>>(
                      "ManualScaleFactor", 1.0, manualScaleFactorValidator,
                      Direction::Input),
                  "Provided value for the scale factor.");
  declareProperty(make_unique<PropertyWithValue<double>>(
                      "OutScaleFactor", Mantid::EMPTY_DBL(), Direction::Output),
                  "The actual used value for the scaling factor.");
}

/** Validate the algorithm's properties.
 * @return A map of property names and their issues.
 */
std::map<std::string, std::string> Stitch1D::validateInputs(void) {
  std::map<std::string, std::string> issues;
  MatrixWorkspace_sptr lhs = getProperty("LHSWorkspace");
  MatrixWorkspace_sptr rhs = getProperty("RHSWorkspace");
  if (!lhs)
    issues["LHSWorkspace"] = "Cannot retrieve workspace";
  if (!rhs)
    issues["RHSWorkspace"] = "Cannot retrieve workspace";
  RunCombinationHelper combHelper;
  combHelper.setReferenceProperties(lhs);
  std::string compatible = combHelper.checkCompatibility(rhs, true);
  if (!compatible.empty()) {
    // histograms: no recalculation of Dx implemented -> do not treat Dx
    if (!(compatible == "spectra must have either Dx values or not; ") ||
        (rhs->isHistogramData())) // Issue only for point data
      issues["RHSWorkspace"] = "Workspace " + rhs->getName() +
                               " is not compatible: " + compatible + "\n";
  }
  if (!isDefault("StartOverlap") && !isDefault("EndOverlap")) {
    double startOverlap = this->getProperty("StartOverlap");
    double endOverlap = this->getProperty("EndOverlap");
    if (endOverlap < startOverlap)
      issues["StartOverlap"] = "Must be smaller than EndOverlap";
    // With regard to x values
    const auto &lhsX = lhs->x(0);
    const auto &rhsX = rhs->x(0);
    const auto lhsMin = lhsX.front();
    const auto rhsMin = rhsX.front();
    const auto lhsMax = lhsX.back();
    const auto rhsMax = rhsX.back();
    // A global view on x values
    const double minVal = std::min(lhsMin, rhsMin);
    if ((minVal - 1.e-9) > startOverlap)
      issues["StartOverlap"] =
          "Must be greater or equal than the minimum x value (" +
          std::to_string(minVal) + ")";
    const double maxVal = std::max(lhsMax, rhsMax);
    if ((maxVal + 1.e-9) < endOverlap)
      issues["EndOverlap"] =
          "Must be smaller or equal than the maximum x value (" +
          std::to_string(maxVal) + ")";
    if (rhs->isHistogramData()) {
      // For the current implementation of binned data, lhs and rhs cannot be
      // exchanged:
      if (lhsMin > startOverlap)
        issues["StartOverlap"] = "Must be greater or equal than the minimum x "
                                 "value of the LHS workspace";
      if (rhsMax < endOverlap)
        issues["EndOverlap"] = "Must be smaller or equal than the maximum x "
                               "value of the RHS workspace";
    }
  }
  return issues;
}

/** Gets the rebinning parameters and calculates any missing values
 @param lhsWS :: The left hand side input workspace
 @param rhsWS :: The right hand side input workspace
 @param scaleRHS :: Scale the right hand side workspace
 @return a vector<double> containing the rebinning parameters
 */
std::vector<double> Stitch1D::getRebinParams(MatrixWorkspace_const_sptr &lhsWS,
                                             MatrixWorkspace_const_sptr &rhsWS,
                                             const bool scaleRHS) const {
  const auto &lhsX = lhsWS->x(0);
  const double minLHSX = lhsX.front();
  const auto &rhsX = rhsWS->x(0);
  const double maxRHSX = rhsX.back();
  std::vector<double> result;
  if (isDefault("Params")) {
    result.emplace_back(minLHSX);
    if (scaleRHS) { // Step from the workspace that will be scaled.
      result.emplace_back(rhsX[1] - rhsX[0]);
    } else { // Step from the workspace that will not be scaled.
      result.emplace_back(lhsX[1] - lhsX[0]);
    }
    result.emplace_back(maxRHSX);
  } else {
    std::vector<double> inputParams = this->getProperty("Params");
    if (inputParams.size() == 1) {
      result.emplace_back(minLHSX);
      result.emplace_back(inputParams.front()); // Use the step supplied.
      result.emplace_back(maxRHSX);
    } else {
      result = inputParams; // user has provided params. Use those.
    }
  }
  if (result[1] < 0.)
    throw(std::runtime_error("The rebin step must be greater than zero."));
  return result;
}

/** Runs the Rebin Algorithm as a child
 @param ws :: The input workspace
 @param params :: a vector<double> containing rebinning parameters
 */
void Stitch1D::rebin(MatrixWorkspace_sptr &ws,
                     const std::vector<double> &params) {
  auto rebin = this->createChildAlgorithm("Rebin");
  rebin->setProperty("InputWorkspace", ws);
  rebin->setProperty("Params", params);
  std::stringstream ssParams;
  ssParams << Mantid::Kernel::Strings::join(params.begin(), params.end(), ",");
  g_log.information("Rebinning Params: " + ssParams.str());
  rebin->execute();
  ws = rebin->getProperty("OutputWorkspace");
}

/** Replaces special values
 @param ws :: The input workspace
 */
void Stitch1D::replaceSpecialValues(MatrixWorkspace_sptr &ws) {
  int histogramCount = static_cast<int>(ws->getNumberHistograms());
  // Record special values and then mask them out as zeros. Special values are
  // remembered and then replaced post processing.
  PARALLEL_FOR_IF(Kernel::threadSafe(*ws))
  for (int i = 0; i < histogramCount; ++i) {
    PARALLEL_START_INTERUPT_REGION
    std::vector<size_t> &nanEIndexes = m_nanEIndexes[i];
    std::vector<size_t> &nanYIndexes = m_nanYIndexes[i];
    std::vector<size_t> &infEIndexes = m_infEIndexes[i];
    std::vector<size_t> &infYIndexes = m_infYIndexes[i];
    // Copy over the data
    auto &sourceY = ws->mutableY(i);
    auto &sourceE = ws->mutableE(i);

    for (size_t j = 0; j < sourceY.size(); ++j) {
      const double value = sourceY[j];
      if (std::isnan(value)) {
        nanYIndexes.emplace_back(j);
        sourceY[j] = 0.;
      } else if (std::isinf(value)) {
        infYIndexes.emplace_back(j);
        sourceY[j] = 0.;
      }

      const double eValue = sourceE[j];
      if (std::isnan(eValue)) {
        nanEIndexes.emplace_back(j);
        sourceE[j] = 0.;
      } else if (std::isinf(eValue)) {
        infEIndexes.emplace_back(j);
        sourceE[j] = 0.;
      }
    }

    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
}

/** Runs the Integration Algorithm as a child.
 @param input :: The input workspace
 @param start :: a double defining the start of the region to integrate
 @param stop :: a double defining the end of the region to integrate
 @return A shared pointer to the resulting MatrixWorkspace
 */
MatrixWorkspace_sptr Stitch1D::integration(MatrixWorkspace_sptr &input,
                                           const double start,
                                           const double stop) {
  auto integration = this->createChildAlgorithm("Integration");
  integration->initialize();
  integration->setProperty("InputWorkspace", input);
  integration->setProperty("RangeLower", start);
  integration->setProperty("RangeUpper", stop);
  g_log.information("Integration RangeLower: " + std::to_string(start));
  g_log.information("Integration RangeUpper: " + std::to_string(stop));
  integration->execute();
  return integration->getProperty("OutputWorkspace");
}

/** Runs the WeightedMean Algorithm as a child
 @param inOne :: The first input workspace
 @param inTwo :: The second input workspace
 @return A shared pointer to the resulting MatrixWorkspace
 */
MatrixWorkspace_sptr Stitch1D::weightedMean(MatrixWorkspace_sptr &inOne,
                                            MatrixWorkspace_sptr &inTwo) {
  auto weightedMean = this->createChildAlgorithm("WeightedMean");
  weightedMean->initialize();
  weightedMean->setProperty("InputWorkspace1", inOne);
  weightedMean->setProperty("InputWorkspace2", inTwo);
  weightedMean->execute();
  return weightedMean->getProperty("OutputWorkspace");
}

/** Runs the ConjoinXRuns Algorithm as a child
 @param inOne :: First input workspace
 @param inTwo :: Second input workspace
 @return A shared pointer to the resulting MatrixWorkspace
 */
MatrixWorkspace_sptr Stitch1D::conjoinXAxis(MatrixWorkspace_sptr &inOne,
                                            MatrixWorkspace_sptr &inTwo) {
  const std::string in1 = "__Stitch1D_intermediate_workspace_1__";
  const std::string in2 = "__Stitch1D_intermediate_workspace_2__";
  Mantid::API::AnalysisDataService::Instance().addOrReplace(in1, inOne);
  Mantid::API::AnalysisDataService::Instance().addOrReplace(in2, inTwo);
  auto conjoinX = this->createChildAlgorithm("ConjoinXRuns");
  conjoinX->initialize();
  conjoinX->setProperty("InputWorkspaces", std::vector<std::string>{in1, in2});
  conjoinX->execute();
  Mantid::API::AnalysisDataService::Instance().remove(in1);
  Mantid::API::AnalysisDataService::Instance().remove(in2);
  API::Workspace_sptr ws = conjoinX->getProperty("OutputWorkspace");
  return boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(ws);
}

/** Runs the CreateSingleValuedWorkspace Algorithm as a child
 @param val :: The double to convert to a single value workspace
 @return A shared pointer to the resulting MatrixWorkspace
 */
MatrixWorkspace_sptr Stitch1D::singleValueWS(const double val) {
  auto singleValueWS =
      this->createChildAlgorithm("CreateSingleValuedWorkspace");
  singleValueWS->initialize();
  singleValueWS->setProperty("DataValue", val);
  singleValueWS->execute();
  return singleValueWS->getProperty("OutputWorkspace");
}

/** Determines if a workspace has non zero errors
 @param ws :: The input workspace
 @return True if there are any non-zero errors in the workspace
 */
bool Stitch1D::hasNonzeroErrors(MatrixWorkspace_sptr &ws) {
  int64_t ws_size = static_cast<int64_t>(ws->getNumberHistograms());
  bool hasNonZeroErrors = false;
  PARALLEL_FOR_IF(Kernel::threadSafe(*ws))
  for (int64_t i = 0; i < ws_size; ++i) {
    PARALLEL_START_INTERUPT_REGION
    if (!hasNonZeroErrors) // Keep checking
    {
      auto e = ws->e(i);
      auto it = std::find_if(e.begin(), e.end(), isNonzero);
      if (it != e.end()) {
        PARALLEL_CRITICAL(has_non_zero) {
          hasNonZeroErrors =
              true; // Set flag. Should not need to check any more spectra.
        }
      }
    }
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  return hasNonZeroErrors;
}

/**
 * @brief scaleWorkspace will set m_scaleFactor and m_errorScaleFactor
 * @param ws :: Input workspace
 * @param scaleFactorWS :: A MatrixWorkspace holding the scaling factor
 * @param dxWS :: A MatrixWorkspace (size of ws) containing Dx values
 */
void Stitch1D::scaleWorkspace(MatrixWorkspace_sptr &ws,
                              MatrixWorkspace_sptr &scaleFactorWS,
                              MatrixWorkspace_const_sptr &dxWS) {
  ws *= scaleFactorWS;
  // We lost Dx values (Multiply) and need to get them back for point data
  if (ws->size() == dxWS->size()) {
    for (size_t i = 0; i < ws->getNumberHistograms(); ++i) {
      if (dxWS->hasDx(i) && !ws->hasDx(i) && !ws->isHistogramData()) {
        ws->setSharedDx(i, dxWS->sharedDx(i));
      }
    }
  }
  m_scaleFactor = scaleFactorWS->y(0).front();
  m_errorScaleFactor = scaleFactorWS->e(0).front();
  if (m_scaleFactor < 1.e-2 || m_scaleFactor > 1.e2 ||
      std::isnan(m_scaleFactor)) {
    std::stringstream messageBuffer;
    messageBuffer << "Calculated scale factor is: " << m_scaleFactor
                  << ". Check that in both input workspaces the integrated "
                     "overlap region is non-zero.";
    g_log.warning(messageBuffer.str());
  }
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void Stitch1D::exec() {
  MatrixWorkspace_const_sptr lhsWS = this->getProperty("LHSWorkspace");
  MatrixWorkspace_const_sptr rhsWS = this->getProperty("RHSWorkspace");

  const size_t histogramCount = rhsWS->getNumberHistograms();
  m_nanYIndexes.resize(histogramCount);
  m_infYIndexes.resize(histogramCount);
  m_nanEIndexes.resize(histogramCount);
  m_infEIndexes.resize(histogramCount);

  const bool scaleRHS = this->getProperty("ScaleRHSWorkspace");
  // At this point, we know already that lhsWS contains the global minimum x
  // value and rhsWS the global maximum x value for binned data
  const double tol = 1.e-9;
  const double intersectionMin = rhsWS->x(0).front() - tol;
  const double intersectionMax = lhsWS->x(0).back() + tol;
  // We get the correct limits for binned data already
  double startOverlap =
      overlap(intersectionMin, "StartOverlap", std::greater<double>());
  double endOverlap =
      overlap(intersectionMax, "EndOverlap", std::less<double>());
  if (startOverlap > endOverlap) { // can only be point data!
    startOverlap = overlap(intersectionMax, "EndOverlap", std::less<double>());
    endOverlap =
        overlap(intersectionMin, "StartOverlap", std::greater<double>());
  }
  m_scaleFactor = this->getProperty("ManualScaleFactor");
  m_errorScaleFactor = m_scaleFactor;
  const bool useManualScaleFactor = this->getProperty("UseManualScaleFactor");
  MatrixWorkspace_sptr lhs = lhsWS->clone();
  MatrixWorkspace_sptr rhs = rhsWS->clone();
  replaceSpecialValues(lhs);
  replaceSpecialValues(rhs);
  const MantidVec params = getRebinParams(lhsWS, rhsWS, scaleRHS);
  if (lhsWS->isHistogramData()) {
    rebin(lhs, params);
    rebin(rhs, params);
  }
  if (useManualScaleFactor) {
    MatrixWorkspace_sptr manualScaleFactorWS = singleValueWS(m_scaleFactor);
    if (scaleRHS)
      rhs *= manualScaleFactorWS;
    else
      lhs *= manualScaleFactorWS;
  } else {
    auto rhsOverlapIntegrated = integration(rhs, startOverlap, endOverlap);
    auto lhsOverlapIntegrated = integration(lhs, startOverlap, endOverlap);
    if (scaleRHS) {
      auto scaleFactor = lhsOverlapIntegrated / rhsOverlapIntegrated;
      scaleWorkspace(rhs, scaleFactor, rhsWS);
    } else {
      auto scaleFactor = rhsOverlapIntegrated / lhsOverlapIntegrated;
      scaleWorkspace(lhs, scaleFactor, lhsWS);
    }
  }
  // Provide log information about the scale factors used in the calculations.
  std::stringstream messageBuffer;
  messageBuffer << "Scale Factor Y is: " << m_scaleFactor
                << " Scale Factor E is: " << m_errorScaleFactor;
  g_log.notice(messageBuffer.str());
  MatrixWorkspace_sptr result;
  if (lhsWS->isHistogramData()) { // If the input workspaces are histograms ...
    size_t a1 = 0;
    size_t a2 = lhs->blocksize();
    try {
      a1 = lhs->binIndexOf(startOverlap + params[1]);
    } catch (std::out_of_range) {
    }
    try {
      a2 = lhs->binIndexOf(endOverlap);
    } catch (std::out_of_range) {
    }
    if (a1 >= a2) {
      g_log.warning("The Params you have provided for binning yield a "
                    "workspace in which start and end overlap appear "
                    "in the same bin. Make binning finer via input "
                    "Params. Stitching remains possible in the sense of "
                    "manual scaling and joining.");
      a1 = a2; // in case a1 > a2
    }
    // Overlap region of lhs and rhs
    MatrixWorkspace_sptr overlap1 = maskAllBut(a1, a2, lhs);
    MatrixWorkspace_sptr overlap2 = maskAllBut(a1, a2, rhs);
    MatrixWorkspace_sptr overlapave;
    if (hasNonzeroErrors(overlap1) && hasNonzeroErrors(overlap2)) {
      overlapave = weightedMean(overlap1, overlap2);
    } else {
      g_log.information("Using un-weighted mean scaling for overlap mean");
      MatrixWorkspace_sptr sum = overlap1 + overlap2;
      MatrixWorkspace_sptr denominator = singleValueWS(2.0);
      overlapave = sum / denominator;
    }
    // Non-overlap region of lhs, rhs
    maskInPlace(a1, lhs->blocksize(), lhs);
    maskInPlace(0, a2, rhs);
    result = lhs + overlapave + rhs;
    reinsertSpecialValues(result);
  } else { // The input workspaces are point data ... join & sort
    result = conjoinXAxis(lhs, rhs);
    if (!result)
      g_log.error("Could not retrieve joined workspace.");

    // Sort the X Axis
    Mantid::API::Algorithm_sptr childAlg = createChildAlgorithm("SortXAxis");
    childAlg->setProperty("InputWorkspace", result);
    childAlg->execute();
    result = childAlg->getProperty("OutputWorkspace");
    if (!result)
      g_log.error("Could not retrieve sorted workspace.");
  }
  setProperty("OutputWorkspace", result);
  setProperty("OutScaleFactor", m_scaleFactor);
}

/** Put special values back.
 * @param ws : MatrixWorkspace to resinsert special values into.
 */
void Stitch1D::reinsertSpecialValues(MatrixWorkspace_sptr ws) {
  int histogramCount = static_cast<int>(ws->getNumberHistograms());
  PARALLEL_FOR_IF(Kernel::threadSafe(*ws))
  for (int i = 0; i < histogramCount; ++i) {
    PARALLEL_START_INTERUPT_REGION
    // Copy over the data
    auto &sourceY = ws->mutableY(i);

    for (const auto &j : m_nanYIndexes[i]) {
      sourceY[j] = std::numeric_limits<double>::quiet_NaN();
    }

    for (const auto &j : m_infYIndexes[i]) {
      sourceY[j] = std::numeric_limits<double>::infinity();
    }

    for (const auto &j : m_nanEIndexes[i]) {
      sourceY[j] = std::numeric_limits<double>::quiet_NaN();
    }

    for (const auto &j : m_infEIndexes[i]) {
      sourceY[j] = std::numeric_limits<double>::infinity();
    }

    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
}

} // namespace Algorithms
} // namespace Mantid
