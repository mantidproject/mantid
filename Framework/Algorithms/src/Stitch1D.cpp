// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/Stitch1D.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAlgorithms/RunCombinationHelpers/RunCombinationHelper.h"
#include "MantidDataObjects/WorkspaceSingleValue.h"
#include "MantidHistogramData/HistogramDx.h"
#include "MantidHistogramData/HistogramE.h"
#include "MantidHistogramData/HistogramX.h"
#include "MantidHistogramData/HistogramY.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/FloatingPointComparison.h"
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
using Mantid::DataObjects::WorkspaceSingleValue;
using Mantid::HistogramData::HistogramE;
using Mantid::HistogramData::HistogramY;

namespace {
/// Returns a tuple holding the first and last x value of the first spectrum and
/// the lhs and rhs workspace, respectively
using MinMaxTuple = boost::tuple<double, double>;
MinMaxTuple calculateXIntersection(MatrixWorkspace_const_sptr &lhsWS, MatrixWorkspace_const_sptr &rhsWS) {
  return MinMaxTuple(rhsWS->x(0).front(), lhsWS->x(0).back());
}
} // namespace

namespace Mantid::Algorithms {

/** Range tolerance
 * This is required for machine precision reasons. Used to adjust StartOverlap
 *and EndOverlap so that they are
 * inclusive of bin boundaries if they are sitting ontop of the bin boundaries.
 */
const double Stitch1D::range_tolerance = 1e-9;
// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(Stitch1D)

/** Zero out all y and e data that is not in the region a1 to a2.
 * @param a1 : Zero based bin index (first one)
 * @param a2 : Zero based bin index (last one inclusive)
 * @param source : Workspace providing the source data.
 */
MatrixWorkspace_sptr Stitch1D::maskAllBut(int a1, int a2, MatrixWorkspace_sptr &source) {
  MatrixWorkspace_sptr product = source->clone();
  const auto histogramCount = static_cast<int>(source->getNumberHistograms());
  PARALLEL_FOR_IF(Kernel::threadSafe(*source, *product))
  for (int i = 0; i < histogramCount; ++i) {
    PARALLEL_START_INTERRUPT_REGION
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
    std::copy(sourceY.begin() + a1 + 1, sourceY.begin() + a2, newY.begin() + a1 + 1);
    std::copy(sourceE.begin() + a1 + 1, sourceE.begin() + a2, newE.begin() + a1 + 1);

    PARALLEL_END_INTERRUPT_REGION
  }
  PARALLEL_CHECK_INTERRUPT_REGION
  return product;
}

/** Mask out data in the region between a1 and a2 with zeros. Operation
 * performed on the original workspace
 * @param a1 : start position in X
 * @param a2 : end position in X
 * @param source : Workspace to mask.
 */
void Stitch1D::maskInPlace(int a1, int a2, MatrixWorkspace_sptr &source) {
  const auto histogramCount = static_cast<int>(source->getNumberHistograms());
  PARALLEL_FOR_IF(Kernel::threadSafe(*source))
  for (int i = 0; i < histogramCount; ++i) {
    PARALLEL_START_INTERRUPT_REGION
    // Copy over the data
    auto &sourceY = source->mutableY(i);
    auto &sourceE = source->mutableE(i);
    for (int binIndex = a1; binIndex < a2; ++binIndex) {
      sourceY[binIndex] = 0;
      sourceE[binIndex] = 0;
    }

    PARALLEL_END_INTERRUPT_REGION
  }
  PARALLEL_CHECK_INTERRUPT_REGION
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void Stitch1D::init() {

  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("LHSWorkspace", "", Direction::Input),
                  "LHS input workspace.");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("RHSWorkspace", "", Direction::Input),
                  "RHS input workspace, must be same type as LHSWorkspace "
                  "(histogram or point data).");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "Output stitched workspace.");
  declareProperty(std::make_unique<PropertyWithValue<double>>("StartOverlap", Mantid::EMPTY_DBL(), Direction::Input),
                  "Start overlap x-value in units of x-axis.");
  declareProperty(std::make_unique<PropertyWithValue<double>>("EndOverlap", Mantid::EMPTY_DBL(), Direction::Input),
                  "End overlap x-value in units of x-axis.");
  declareProperty(std::make_unique<ArrayProperty<double>>("Params", std::make_shared<RebinParamsValidator>(true)),
                  "Rebinning Parameters. See Rebin for format. If only a "
                  "single value is provided, start and end are taken from "
                  "input workspaces.");
  declareProperty(std::make_unique<PropertyWithValue<bool>>("ScaleRHSWorkspace", true, Direction::Input),
                  "Scaling either with respect to LHS workspace or RHS workspace");
  declareProperty(std::make_unique<PropertyWithValue<bool>>("UseManualScaleFactor", false, Direction::Input),
                  "True to use a provided value for the scale factor.");
  declareProperty(
      "OutputScalingWorkspace", "",
      "Output WorkspaceSingleValue containing the scale factor and its error. No workspace creation if left empty.");
  auto manualScaleFactorValidator = std::make_shared<BoundedValidator<double>>();
  manualScaleFactorValidator->setLower(0);
  manualScaleFactorValidator->setExclusive(true);
  declareProperty(std::make_unique<PropertyWithValue<double>>("ManualScaleFactor", 1.0, manualScaleFactorValidator,
                                                              Direction::Input),
                  "Provided value for the scale factor.");
  declareProperty(std::make_unique<PropertyWithValue<double>>("OutScaleFactor", Mantid::EMPTY_DBL(), Direction::Output),
                  "The actual used value for the scaling factor.");
}

/** Validate the algorithm's properties.
 * @return A map of property names and their issues.
 */
std::map<std::string, std::string> Stitch1D::validateInputs(void) {
  std::map<std::string, std::string> issues;
  MatrixWorkspace_sptr lhs = getProperty("LHSWorkspace");
  MatrixWorkspace_sptr rhs = getProperty("RHSWorkspace");
  if (!lhs) {
    issues["LHSWorkspace"] = "The LHSWorkspace must be a MatrixWorkspace.";
  }
  if (!rhs) {
    issues["RHSWorkspace"] = "The RHSWorkspace must be a MatrixWorkspace.";
  }
  if (!issues.empty()) {
    return issues;
  }
  RunCombinationHelper combHelper;
  combHelper.setReferenceProperties(lhs);
  std::string compatible = combHelper.checkCompatibility(rhs, true);
  if (!compatible.empty()) {
    // histograms: no recalculation of Dx implemented -> do not treat Dx
    if (!(compatible == "spectra must have either Dx values or not; ") ||
        (rhs->isHistogramData())) // Issue only for point data
      issues["RHSWorkspace"] = "Workspace " + rhs->getName() + " is not compatible: " + compatible + "\n";
  }
  return issues;
}

/** Gets the start of the overlapping region
 @param intesectionMin :: The minimum possible value for the overlapping region
 to inhabit
 @param intesectionMax :: The maximum possible value for the overlapping region
 to inhabit
 @return a double contianing the start of the overlapping region
 */
double Stitch1D::getStartOverlap(const double intesectionMin, const double intesectionMax) const {
  Property const *startOverlapProp = this->getProperty("StartOverlap");
  double startOverlapVal = this->getProperty("StartOverlap");
  startOverlapVal -= this->range_tolerance;
  const bool startOverlapBeyondRange = (startOverlapVal < intesectionMin) || (startOverlapVal > intesectionMax);
  if (startOverlapProp->isDefault() || startOverlapBeyondRange) {
    if (!startOverlapProp->isDefault() && startOverlapBeyondRange) {
      char message[200];
      std::sprintf(message,
                   "StartOverlap is outside range at %0.4f, Min is "
                   "%0.4f, Max is %0.4f . Forced to be: %0.4f",
                   startOverlapVal, intesectionMin, intesectionMax, intesectionMin);
      g_log.warning(std::string(message));
    }
    startOverlapVal = intesectionMin;
    std::stringstream buffer;
    buffer << "StartOverlap calculated to be: " << startOverlapVal;
    g_log.information(buffer.str());
  }
  return startOverlapVal;
}

/** Gets the end of the overlapping region
 @param intesectionMin :: The minimum possible value for the overlapping region
 to inhabit
 @param intesectionMax :: The maximum possible value for the overlapping region
 to inhabit
 @return a double contianing the end of the overlapping region
 */
double Stitch1D::getEndOverlap(const double intesectionMin, const double intesectionMax) const {
  Property const *endOverlapProp = this->getProperty("EndOverlap");
  double endOverlapVal = this->getProperty("EndOverlap");
  endOverlapVal += this->range_tolerance;
  const bool endOverlapBeyondRange = (endOverlapVal < intesectionMin) || (endOverlapVal > intesectionMax);
  if (endOverlapProp->isDefault() || endOverlapBeyondRange) {
    if (!endOverlapProp->isDefault() && endOverlapBeyondRange) {
      char message[200];
      std::sprintf(message,
                   "EndOverlap is outside range at %0.4f, Min is "
                   "%0.4f, Max is %0.4f . Forced to be: %0.4f",
                   endOverlapVal, intesectionMin, intesectionMax, intesectionMax);
      g_log.warning(std::string(message));
    }
    endOverlapVal = intesectionMax;
    std::stringstream buffer;
    buffer << "EndOverlap calculated to be: " << endOverlapVal;
    g_log.information(buffer.str());
  }
  return endOverlapVal;
}

/** Gets the rebinning parameters and calculates any missing values
 @param lhsWS :: The left hand side input workspace
 @param rhsWS :: The right hand side input workspace
 @param scaleRHS :: Scale the right hand side workspace
 @return a vector<double> contianing the rebinning parameters
 */
std::vector<double> Stitch1D::getRebinParams(MatrixWorkspace_const_sptr &lhsWS, MatrixWorkspace_const_sptr &rhsWS,
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

    calculatedParams.emplace_back(minLHSX);
    calculatedParams.emplace_back(calculatedStep);
    calculatedParams.emplace_back(maxRHSX);
    result = calculatedParams;
  } else {
    if (inputParams.size() == 1) {
      std::vector<double> calculatedParams;
      calculatedParams.emplace_back(minLHSX);
      calculatedParams.emplace_back(inputParams.front()); // Use the step supplied.
      calculatedParams.emplace_back(maxRHSX);
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
MatrixWorkspace_sptr Stitch1D::rebin(MatrixWorkspace_sptr &input, const std::vector<double> &params) {
  auto rebin = this->createChildAlgorithm("Rebin");
  rebin->setProperty("InputWorkspace", input);
  rebin->setProperty("Params", params);
  std::stringstream ssParams;
  ssParams << params[0] << "," << params[1] << "," << params[2];
  g_log.information("Rebinning Params: " + ssParams.str());
  rebin->execute();
  MatrixWorkspace_sptr outWS = rebin->getProperty("OutputWorkspace");

  const auto histogramCount = static_cast<int>(outWS->getNumberHistograms());

  // Record special values and then mask them out as zeros. Special values are
  // remembered and then replaced post processing.
  PARALLEL_FOR_IF(Kernel::threadSafe(*outWS))
  for (int i = 0; i < histogramCount; ++i) {
    PARALLEL_START_INTERRUPT_REGION
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
        nanYIndexes.emplace_back(j);
        sourceY[j] = 0;
      } else if (std::isinf(value)) {
        infYIndexes.emplace_back(j);
        sourceY[j] = 0;
      }

      const double eValue = sourceE[j];
      if (std::isnan(eValue)) {
        nanEIndexes.emplace_back(j);
        sourceE[j] = 0;
      } else if (std::isinf(eValue)) {
        infEIndexes.emplace_back(j);
        sourceE[j] = 0;
      }
    }

    PARALLEL_END_INTERRUPT_REGION
  }
  PARALLEL_CHECK_INTERRUPT_REGION

  return outWS;
}

/** Runs the Integration Algorithm as a child.
 @param input :: The input workspace
 @param start :: a double defining the start of the region to integrate
 @param stop :: a double defining the end of the region to integrate
 @return A shared pointer to the resulting MatrixWorkspace
 */
MatrixWorkspace_sptr Stitch1D::integration(MatrixWorkspace_sptr const &input, const double start, const double stop) {
  auto integration = this->createChildAlgorithm("Integration");
  integration->initialize();
  integration->setProperty("InputWorkspace", input);
  integration->setProperty("RangeLower", start);
  integration->setProperty("RangeUpper", stop);
  g_log.information("Integration RangeLower: " + boost::lexical_cast<std::string>(start));
  g_log.information("Integration RangeUpper: " + boost::lexical_cast<std::string>(stop));
  integration->execute();
  return integration->getProperty("OutputWorkspace");
}

/** Runs the WeightedMean Algorithm as a child
 @param inOne :: The first input workspace
 @param inTwo :: The second input workspace
 @return A shared pointer to the resulting MatrixWorkspace
 */
MatrixWorkspace_sptr Stitch1D::weightedMean(MatrixWorkspace_sptr const &inOne, MatrixWorkspace_sptr const &inTwo) {
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
MatrixWorkspace_sptr Stitch1D::conjoinXAxis(MatrixWorkspace_sptr const &inOne, MatrixWorkspace_sptr const &inTwo) {
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
  return std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(ws);
}
/** Finds the bins containing the ends of the overlapping region
 @param startOverlap :: The start of the overlapping region
 @param endOverlap :: The end of the overlapping region
 @param workspace :: The workspace to determine the overlaps inside
 @return a boost::tuple<int,int> containing the bin indexes of the overlaps
 */
boost::tuple<int, int> Stitch1D::findStartEndIndexes(double startOverlap, double endOverlap,
                                                     MatrixWorkspace_sptr &workspace) {
  auto a1 = static_cast<int>(workspace->yIndexOfX(startOverlap));
  auto a2 = static_cast<int>(workspace->yIndexOfX(endOverlap));
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
  for (auto i = 0u; i < ws->getNumberHistograms(); ++i) {
    const auto &e = ws->e(i);
    // It's faster to sum and then check the result is non-zero than to check each element is non-zero individually
    if (std::accumulate(e.begin(), e.end(), 0.0) != 0.0) {
      return true;
    }
  }
  return false;
}

/**
 * @brief scaleWorkspace will set m_scaleFactor and m_errorScaleFactor
 * @param ws :: Input workspace
 * @param scaleFactorWS :: A MatrixWorkspace holding the scaling factor
 * @param dxWS :: A MatrixWorkspace (size of ws) containing Dx values
 */
void Stitch1D::scaleWorkspace(MatrixWorkspace_sptr &ws, MatrixWorkspace_sptr &scaleFactorWS,
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
  if (m_scaleFactor < 1e-2 || m_scaleFactor > 1e2 || std::isnan(m_scaleFactor)) {
    std::stringstream messageBuffer;
    messageBuffer << "Stitch1D calculated scale factor is: " << m_scaleFactor
                  << ". Check the overlap region is non-zero.";
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
  double startOverlap = getStartOverlap(intersectionMin, intersectionMax);
  double endOverlap = getEndOverlap(intersectionMin, intersectionMax);
  if (startOverlap > endOverlap) {
    std::string message = boost::str(boost::format("Stitch1D cannot have a StartOverlap > EndOverlap. "
                                                   "StartOverlap: %0.9f, EndOverlap: %0.9f") %
                                     startOverlap % endOverlap);
    throw std::runtime_error(message);
  }

  const bool scaleRHS = this->getProperty("ScaleRHSWorkspace");

  MatrixWorkspace_sptr lhs = lhsWS->clone();
  MatrixWorkspace_sptr rhs = rhsWS->clone();
  if (lhsWS->isHistogramData()) {
    MantidVec params = getRebinParams(lhsWS, rhsWS, scaleRHS);
    const double xMin = params.front();
    const double xMax = params.back();

    if (Kernel::withinAbsoluteDifference(xMin, startOverlap, 1E-6))
      startOverlap = xMin;

    if (Kernel::withinAbsoluteDifference(xMax, endOverlap, 1E-6))
      endOverlap = xMax;

    if (startOverlap < xMin) {
      std::string message = boost::str(boost::format("Stitch1D StartOverlap is outside the available X range. "
                                                     "StartOverlap: %10.9f, X min: %10.9f") %
                                       startOverlap % xMin);
      throw std::runtime_error(message);
    }
    if (endOverlap > xMax) {
      std::string message = boost::str(boost::format("Stitch1D EndOverlap is outside the available X range. "
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
    if (scaleRHS)
      rhs *= m_scaleFactor;
    else
      lhs *= m_scaleFactor;
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
  messageBuffer << "Scale Factor Y is: " << m_scaleFactor << " Scale Factor E is: " << m_errorScaleFactor;
  g_log.notice(messageBuffer.str());

  MatrixWorkspace_sptr result;
  if (lhsWS->isHistogramData()) { // If the input workspaces are histograms ...
    boost::tuple<int, int> startEnd = findStartEndIndexes(startOverlap, endOverlap, lhs);
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
      g_log.information("Using un-weighted mean for Stitch1D overlap mean");
      MatrixWorkspace_sptr sum = overlap1 + overlap2;
      overlapave = sum * 0.5;
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

  // if required, create the output single value workspace containing the scaling factor and its error
  const std::string scalingWsName = this->getPropertyValue("OutputScalingWorkspace");
  if (!scalingWsName.empty()) {
    auto createSingleAlg = createChildAlgorithm("CreateSingleValuedWorkspace");
    createSingleAlg->setProperty("DataValue", m_scaleFactor);
    createSingleAlg->setProperty("ErrorValue", m_errorScaleFactor);
    createSingleAlg->executeAsChildAlg();
    MatrixWorkspace_sptr singleWS = createSingleAlg->getProperty("OutputWorkspace");
    AnalysisDataService::Instance().addOrReplace(scalingWsName, singleWS);
  }
}
/** Put special values back.
 * @param ws : MatrixWorkspace to resinsert special values into.
 */
void Stitch1D::reinsertSpecialValues(const MatrixWorkspace_sptr &ws) {
  auto histogramCount = static_cast<int>(ws->getNumberHistograms());
  PARALLEL_FOR_IF(Kernel::threadSafe(*ws))
  for (int i = 0; i < histogramCount; ++i) {
    PARALLEL_START_INTERRUPT_REGION
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

    PARALLEL_END_INTERRUPT_REGION
  }
  PARALLEL_CHECK_INTERRUPT_REGION
}

} // namespace Mantid::Algorithms
