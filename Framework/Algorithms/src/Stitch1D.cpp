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

#include <algorithm>
#include <boost/format.hpp>
#include <boost/math/special_functions.hpp>
#include <boost/tuple/tuple.hpp>
#include <map>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using Mantid::HistogramData::HistogramE;
using Mantid::HistogramData::HistogramY;

namespace {
/// Returns a tuple holding the first and last x value of the first spectrum and
/// the lhs and rhs workspace, respectively
using MinMaxTuple = boost::tuple<double, double>;
MinMaxTuple calculateXIntersection(MatrixWorkspace_const_sptr &lhsWS,
                                   MatrixWorkspace_const_sptr &rhsWS) {
  return MinMaxTuple(rhsWS->x(0).front(), lhsWS->x(0).back());
}

/// Check if a double is not zero and returns a bool indicating success
bool isNonzero(double i) { return (0 != i); }
} // namespace

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(Stitch1D)

/** Zero out all y and e data that is not in the region a1 to a2.
 * @param a1 : Zero based bin index (first one)
 * @param a2 : Zero based bin index (last one inclusive)
 * @param source : Workspace providing the source data.
 */
MatrixWorkspace_sptr Stitch1D::maskAllBut(int a1, int a2,
                                          MatrixWorkspace_sptr &source) {
  MatrixWorkspace_sptr product = source->clone();
  const int histogramCount = static_cast<int>(source->getNumberHistograms());
  PARALLEL_FOR_IF(Kernel::threadSafe(*source, *product))
  for (int i = 0; i < histogramCount; ++i) {
    PARALLEL_START_INTERUPT_REGION
    // Copy over the bin boundaries
    product->setSharedX(i, source->sharedX(i));
    // Copy over the data
    const auto &sourceY = source->y(i);
    const auto &sourceE = source->e(i);

    // initially zero - out the data.
    product->mutableY(i) = HistogramY(sourceY.size(), 0);
    product->mutableE(i) = HistogramE(sourceE.size(), 0);

    auto &newY = product->mutableY(i);
    auto &newE = product->mutableE(i);

    // Copy over the non-zero stuff
    std::copy(sourceY.begin() + a1 + 1, sourceY.begin() + a2,
              newY.begin() + a1 + 1);
    std::copy(sourceE.begin() + a1 + 1, sourceE.begin() + a2,
              newE.begin() + a1 + 1);

    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
  return product;
}

/** Mask out data in the region between a1 and a2 with zeros. Operation
 * performed on the original workspace
 * @param a1 : start position in X
 * @param a2 : end position in X
 * @param source : Workspace to mask.
 * @return Masked workspace.
 */
void Stitch1D::maskInPlace(int a1, int a2, MatrixWorkspace_sptr &source) {
  const int histogramCount = static_cast<int>(source->getNumberHistograms());
  PARALLEL_FOR_IF(Kernel::threadSafe(*source))
  for (int i = 0; i < histogramCount; ++i) {
    PARALLEL_START_INTERUPT_REGION
    // Copy over the data
    auto &sourceY = source->mutableY(i);
    auto &sourceE = source->mutableE(i);
    for (int binIndex = a1; binIndex < a2; ++binIndex) {
      sourceY[binIndex] = 0;
      sourceE[binIndex] = 0;
    }

    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void Stitch1D::init() {

  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "LHSWorkspace", "", Direction::Input),
                  "LHS input workspace.");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "RHSWorkspace", "", Direction::Input),
                  "RHS input workspace, must be same type as LHSWorkspace "
                  "(histogram or point data).");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "Output stitched workspace.");
  declareProperty(std::make_unique<PropertyWithValue<double>>(
                      "StartOverlap", Mantid::EMPTY_DBL(), Direction::Input),
                  "Start overlap x-value in units of x-axis.");
  declareProperty(std::make_unique<PropertyWithValue<double>>(
                      "EndOverlap", Mantid::EMPTY_DBL(), Direction::Input),
                  "End overlap x-value in units of x-axis.");
  declareProperty(std::make_unique<ArrayProperty<double>>(
                      "Params", boost::make_shared<RebinParamsValidator>(true)),
                  "Rebinning Parameters. See Rebin for format. If only a "
                  "single value is provided, start and end are taken from "
                  "input workspaces.");
  declareProperty(
      std::make_unique<PropertyWithValue<bool>>("ScaleRHSWorkspace", true,
                                                Direction::Input),
      "Scaling either with respect to LHS workspace or RHS workspace");
  declareProperty(std::make_unique<PropertyWithValue<bool>>(
                      "UseManualScaleFactor", false, Direction::Input),
                  "True to use a provided value for the scale factor.");
  auto manualScaleFactorValidator =
      boost::make_shared<BoundedValidator<double>>();
  manualScaleFactorValidator->setLower(0);
  manualScaleFactorValidator->setExclusive(true);
  declareProperty(std::make_unique<PropertyWithValue<double>>(
                      "ManualScaleFactor", 1.0, manualScaleFactorValidator,
                      Direction::Input),
                  "Provided value for the scale factor.");
  declareProperty(std::make_unique<PropertyWithValue<double>>(
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
  }
  return issues;
}

/** Limits of the overlapping region
 @param intersectionMin :: The minimum possible value
 @param intersectionMax :: The maximum possible value
 @return std::pair containing the start and end values
 */
std::pair<double, double>
Stitch1D::getOverlap(const double intersectionMin,
                     const double intersectionMax) const {
  double interSectionMin = this->getProperty("StartOverlap");
  double interSectionMax = this->getProperty("EndOverlap");
  /* Range tolerance is 1.e-9
   * This is required for machine precision reasons. Used to adjust StartOverlap
   * and EndOverlap so that they are inclusive of bin boundaries if they are
   * sitting ontop of the bin boundaries.
   */
  if (isDefault("StartOverlap"))
    interSectionMin = intersectionMin;
  if (isDefault("EndOverlap"))
    interSectionMax = intersectionMax;
  if ((intersectionMin - 1.e-9) > interSectionMin) {
    g_log.warning("StartOverlap outside range, re-determine");
    interSectionMin = intersectionMin;
  }
  if ((intersectionMax + 1.e-9) < interSectionMax) {
    g_log.warning("EndOverlap outside range, re-determine");
    interSectionMax = intersectionMax;
  }
  g_log.information("StartOverlap: " + std::to_string(interSectionMin));
  g_log.information("EndOverlap: " + std::to_string(interSectionMax));
  return std::make_pair(interSectionMin, interSectionMax);
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
  std::vector<double> inputParams = this->getProperty("Params");
  Property *prop = this->getProperty("Params");
  const bool areParamsDefault = prop->isDefault();

  const auto &lhsX = lhsWS->x(0);
  auto it = std::min_element(lhsX.begin(), lhsX.end());
  const double minLHSX = *it;

  const auto &rhsX = rhsWS->x(0);
  it = std::max_element(rhsX.begin(), rhsX.end());
  const double maxRHSX = *it;

  std::vector<double> result;
  if (areParamsDefault) {
    std::vector<double> calculatedParams;

    // Calculate the step size based on the existing step size of the LHS
    // workspace. That way scale factors should be reasonably maintained.
    double calculatedStep = 0;
    if (scaleRHS) {
      // Calculate the step from the workspace that will not be scaled. The LHS
      // workspace.
      calculatedStep = lhsX[1] - lhsX[0];
    } else {
      // Calculate the step from the workspace that will not be scaled. The RHS
      // workspace.
      calculatedStep = rhsX[1] - rhsX[0];
    }

    calculatedParams.push_back(minLHSX);
    calculatedParams.push_back(calculatedStep);
    calculatedParams.push_back(maxRHSX);
    result = calculatedParams;
  } else {
    if (inputParams.size() == 1) {
      std::vector<double> calculatedParams;
      calculatedParams.push_back(minLHSX);
      calculatedParams.push_back(inputParams.front()); // Use the step supplied.
      calculatedParams.push_back(maxRHSX);
      result = calculatedParams;
    } else {
      result = inputParams; // user has provided params. Use those.
    }
  }
  return result;
}

/** Runs the Rebin Algorithm as a child and replaces special values
 @param input :: The input workspace
 @param params :: a vector<double> containing rebinning parameters
 @return A shared pointer to the resulting MatrixWorkspace
 */
MatrixWorkspace_sptr Stitch1D::rebin(MatrixWorkspace_sptr &input,
                                     const std::vector<double> &params) {
  auto rebin = this->createChildAlgorithm("Rebin");
  rebin->setProperty("InputWorkspace", input);
  rebin->setProperty("Params", params);
  std::stringstream ssParams;
  ssParams << params[0] << "," << params[1] << "," << params[2];
  g_log.information("Rebinning Params: " + ssParams.str());
  rebin->execute();
  MatrixWorkspace_sptr outWS = rebin->getProperty("OutputWorkspace");

  const int histogramCount = static_cast<int>(outWS->getNumberHistograms());

  // Record special values and then mask them out as zeros. Special values are
  // remembered and then replaced post processing.
  PARALLEL_FOR_IF(Kernel::threadSafe(*outWS))
  for (int i = 0; i < histogramCount; ++i) {
    PARALLEL_START_INTERUPT_REGION
    std::vector<size_t> &nanEIndexes = m_nanEIndexes[i];
    std::vector<size_t> &nanYIndexes = m_nanYIndexes[i];
    std::vector<size_t> &infEIndexes = m_infEIndexes[i];
    std::vector<size_t> &infYIndexes = m_infYIndexes[i];
    // Copy over the data
    auto &sourceY = outWS->mutableY(i);
    auto &sourceE = outWS->mutableE(i);

    for (size_t j = 0; j < sourceY.size(); ++j) {
      const double value = sourceY[j];
      if (std::isnan(value)) {
        nanYIndexes.push_back(j);
        sourceY[j] = 0;
      } else if (std::isinf(value)) {
        infYIndexes.push_back(j);
        sourceY[j] = 0;
      }

      const double eValue = sourceE[j];
      if (std::isnan(eValue)) {
        nanEIndexes.push_back(j);
        sourceE[j] = 0;
      } else if (std::isinf(eValue)) {
        infEIndexes.push_back(j);
        sourceE[j] = 0;
      }
    }

    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  return outWS;
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
  g_log.information("Integration RangeLower: " +
                    boost::lexical_cast<std::string>(start));
  g_log.information("Integration RangeUpper: " +
                    boost::lexical_cast<std::string>(stop));
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

/** Finds the bins containing the ends of the overlapping region
 @param startOverlap :: The start of the overlapping region
 @param endOverlap :: The end of the overlapping region
 @param workspace :: The workspace to determine the overlaps inside
 @return a boost::tuple<int,int> containing the bin indexes of the overlaps
 */
boost::tuple<int, int>
Stitch1D::findStartEndIndexes(double startOverlap, double endOverlap,
                              MatrixWorkspace_sptr &workspace) {
  int a1 = static_cast<int>(workspace->yIndexOfX(startOverlap));
  int a2 = static_cast<int>(workspace->yIndexOfX(endOverlap));
  if (a1 == a2) {
    throw std::runtime_error("The Params you have provided for binning yield a "
                             "workspace in which start and end overlap appear "
                             "in the same bin. Make binning finer via input "
                             "Params.");
  }
  return boost::tuple<int, int>(a1, a2);
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
      const auto &e = ws->e(i);
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
  if (m_scaleFactor < 1e-2 || m_scaleFactor > 1e2 ||
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
  const MinMaxTuple intesectionXRegion = calculateXIntersection(lhsWS, rhsWS);

  const size_t histogramCount = rhsWS->getNumberHistograms();
  m_nanYIndexes.resize(histogramCount);
  m_infYIndexes.resize(histogramCount);
  m_nanEIndexes.resize(histogramCount);
  m_infEIndexes.resize(histogramCount);

  const double intersectionMin = intesectionXRegion.get<0>();
  const double intersectionMax = intesectionXRegion.get<1>();
  std::pair<double, double> overlap =
      getOverlap(intersectionMin, intersectionMax);
  double startOverlap = overlap.first;
  double endOverlap = overlap.second;
  if (startOverlap > endOverlap) {
    if (lhsWS->isHistogramData()) {
      g_log.error("EndOverlap is smaller than StartOverlap");
      throw std::runtime_error("EndOverlap is smaller than StartOverlap");
    } else {
      startOverlap = overlap.second;
      endOverlap = overlap.first;
    }
  }
  const bool scaleRHS = this->getProperty("ScaleRHSWorkspace");

  MatrixWorkspace_sptr lhs = lhsWS->clone();
  MatrixWorkspace_sptr rhs = rhsWS->clone();
  if (lhsWS->isHistogramData()) {
    MantidVec params = getRebinParams(lhsWS, rhsWS, scaleRHS);
    const double xMin = params.front();
    const double xMax = params.back();

    if (std::abs(xMin - startOverlap) < 1E-6)
      startOverlap = xMin;

    if (std::abs(xMax - endOverlap) < 1E-6)
      endOverlap = xMax;

    if (startOverlap < xMin) {
      std::string message = boost::str(
          boost::format("StartOverlap is outside the available X range. "
                        "StartOverlap: %10.9f, X min: %10.9f") %
          startOverlap % xMin);
      throw std::runtime_error(message);
    }
    if (endOverlap > xMax) {
      std::string message = boost::str(
          boost::format("EndOverlap is outside the available X range. "
                        "EndOverlap: %10.9f, X max: %10.9f") %
          endOverlap % xMax);
      throw std::runtime_error(message);
    }
    lhs = rebin(lhs, params);
    rhs = rebin(rhs, params);
  }

  m_scaleFactor = this->getProperty("ManualScaleFactor");
  m_errorScaleFactor = m_scaleFactor;
  const bool useManualScaleFactor = this->getProperty("UseManualScaleFactor");
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
      auto scalingFactors = lhsOverlapIntegrated / rhsOverlapIntegrated;
      scaleWorkspace(rhs, scalingFactors, rhsWS);
    } else {
      auto scalingFactors = rhsOverlapIntegrated / lhsOverlapIntegrated;
      scaleWorkspace(lhs, scalingFactors, lhsWS);
    }
  }
  // Provide log information about the scale factors used in the calculations.
  std::stringstream messageBuffer;
  messageBuffer << "Scale Factor Y is: " << m_scaleFactor
                << " Scale Factor E is: " << m_errorScaleFactor;
  g_log.notice(messageBuffer.str());

  MatrixWorkspace_sptr result;
  if (lhsWS->isHistogramData()) { // If the input workspaces are histograms ...
    boost::tuple<int, int> startEnd =
        findStartEndIndexes(startOverlap, endOverlap, lhs);
    int a1 = boost::tuples::get<0>(startEnd);
    int a2 = boost::tuples::get<1>(startEnd);
    // Mask out everything BUT the overlap region as a new workspace.
    MatrixWorkspace_sptr overlap1 = maskAllBut(a1, a2, lhs);
    // Mask out everything BUT the overlap region as a new workspace.
    MatrixWorkspace_sptr overlap2 = maskAllBut(a1, a2, rhs);
    // Mask out everything AFTER the overlap region as a new workspace.
    maskInPlace(a1 + 1, static_cast<int>(lhs->blocksize()), lhs);
    // Mask out everything BEFORE the overlap region as a new workspace.
    maskInPlace(0, a2, rhs);
    MatrixWorkspace_sptr overlapave;
    if (hasNonzeroErrors(overlap1) && hasNonzeroErrors(overlap2)) {
      overlapave = weightedMean(overlap1, overlap2);
    } else {
      g_log.information("Using un-weighted mean scaling for overlap mean");
      MatrixWorkspace_sptr sum = overlap1 + overlap2;
      MatrixWorkspace_sptr denominator = singleValueWS(2.0);
      overlapave = sum / denominator;
    }
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

    for (auto j : m_nanYIndexes[i]) {
      sourceY[j] = std::numeric_limits<double>::quiet_NaN();
    }

    for (auto j : m_infYIndexes[i]) {
      sourceY[j] = std::numeric_limits<double>::infinity();
    }

    for (auto j : m_nanEIndexes[i]) {
      sourceY[j] = std::numeric_limits<double>::quiet_NaN();
    }

    for (auto j : m_infEIndexes[i]) {
      sourceY[j] = std::numeric_limits<double>::infinity();
    }

    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
}

} // namespace Algorithms
} // namespace Mantid
