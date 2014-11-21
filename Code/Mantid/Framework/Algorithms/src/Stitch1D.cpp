#include "MantidAlgorithms/Stitch1D.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/RebinParamsValidator.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/BoundedValidator.h"

#include <boost/make_shared.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/math/special_functions.hpp>
#include <vector>
#include <algorithm>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using Mantid::MantidVec;

namespace
{

  typedef boost::tuple<double, double> MinMaxTuple;
  MinMaxTuple calculateXIntersection(MatrixWorkspace_sptr lhsWS, MatrixWorkspace_sptr rhsWS)
  {
    MantidVec lhs_x = lhsWS->readX(0);
    MantidVec rhs_x = rhsWS->readX(0);
    return MinMaxTuple(rhs_x.front(), lhs_x.back());
  }

  bool isNonzero(double i)
  {
    return (0 != i);
  }

  bool isNan(const double& value)
  {
    return boost::math::isnan(value);
  }

  bool isInf(const double& value)
  {
    return std::abs(value) == std::numeric_limits<double>::infinity();
  }

}

namespace Mantid
{
  namespace Algorithms
  {

    /**
     * Range tolerance
     *
     * This is required for machine precision reasons. Used to adjust StartOverlap and EndOverlap so that they are
     * inclusive of bin boundaries if they are sitting ontop of the bin boundaries.
     */
    const double Stitch1D::range_tolerance = 1e-9;
// Register the algorithm into the AlgorithmFactory
    DECLARE_ALGORITHM(Stitch1D)

    /**
     * Zero out all y and e data that is not in the region a1 to a2.
     * @param a1 : Zero based bin index (first one)
     * @param a2 : Zero based bin index (last one inclusive)
     * @param source : Workspace providing the source data.
     * @return Masked workspace.
     */
    MatrixWorkspace_sptr Stitch1D::maskAllBut(int a1, int a2, MatrixWorkspace_sptr & source)
    {
      MatrixWorkspace_sptr product = WorkspaceFactory::Instance().create(source);
      const int histogramCount = static_cast<int>(source->getNumberHistograms());
      PARALLEL_FOR2(source,product)
      for (int i = 0; i < histogramCount; ++i)
      {
        PARALLEL_START_INTERUPT_REGION
        // Copy over the bin boundaries
        product->setX(i, source->refX(i));
        // Copy over the data
        const MantidVec& sourceY = source->readY(i);
        const MantidVec& sourceE = source->readE(i);

        // initially zero - out the data.
        product->dataY(i) = MantidVec(sourceY.size(), 0);
        product->dataE(i) = MantidVec(sourceE.size(), 0);

        MantidVec& newY = product->dataY(i);
        MantidVec& newE = product->dataE(i);

        // Copy over the non-zero stuff
        std::copy(sourceY.begin() + a1 + 1, sourceY.begin() + a2, newY.begin() + a1 + 1);
        std::copy(sourceE.begin() + a1 + 1, sourceE.begin() + a2, newE.begin() + a1 + 1);

        PARALLEL_END_INTERUPT_REGION
      }
      PARALLEL_CHECK_INTERUPT_REGION
      return product;
    }

    /**
     * Mask out data in the region between a1 and a2 with zeros. Operation performed on the original workspace
     * @param a1 : start position in X
     * @param a2 : end position in X
     * @param source : Workspace to mask.
     */
    void Stitch1D::maskInPlace(int a1, int a2, MatrixWorkspace_sptr source)
    {
      MatrixWorkspace_sptr product = WorkspaceFactory::Instance().create(source);
      const int histogramCount = static_cast<int>(source->getNumberHistograms());
      PARALLEL_FOR2(source,product)
      for (int i = 0; i < histogramCount; ++i)
      {
        PARALLEL_START_INTERUPT_REGION
        // Copy over the data
        MantidVec& sourceY = source->dataY(i);
        MantidVec& sourceE = source->dataE(i);

        for (int i = a1; i < a2; ++i)
        {
          sourceY[i] = 0;
          sourceE[i] = 0;
        }

        PARALLEL_END_INTERUPT_REGION
      }
      PARALLEL_CHECK_INTERUPT_REGION
    }

//----------------------------------------------------------------------------------------------
    /** Initialize the algorithm's properties.
     */
    void Stitch1D::init()
    {
      Kernel::IValidator_sptr histogramValidator = boost::make_shared<HistogramValidator>();

      declareProperty(
          new WorkspaceProperty<MatrixWorkspace>("LHSWorkspace", "", Direction::Input,
              histogramValidator->clone()), "LHS input workspace.");
      declareProperty(
          new WorkspaceProperty<MatrixWorkspace>("RHSWorkspace", "", Direction::Input,
              histogramValidator->clone()), "RHS input workspace.");
      declareProperty(new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace", "", Direction::Output),
          "Output stitched workspace.");
      declareProperty(
          new PropertyWithValue<double>("StartOverlap", Mantid::EMPTY_DBL(), Direction::Input),
          "Start overlap x-value in units of x-axis. Optional.");
      declareProperty(new PropertyWithValue<double>("EndOverlap", Mantid::EMPTY_DBL(), Direction::Input),
          "End overlap x-value in units of x-axis. Optional.");
      declareProperty(
          new ArrayProperty<double>("Params", boost::make_shared<RebinParamsValidator>(true)),
          "Rebinning Parameters. See Rebin for format. If only a single value is provided, start and end are taken from input workspaces.");
      declareProperty(new PropertyWithValue<bool>("ScaleRHSWorkspace", true, Direction::Input),
          "Scaling either with respect to workspace 1 or workspace 2");
      declareProperty(new PropertyWithValue<bool>("UseManualScaleFactor", false, Direction::Input),
          "True to use a provided value for the scale factor.");
      auto manualScaleFactorValidator = boost::make_shared<BoundedValidator<double> >();
      manualScaleFactorValidator->setLower(0);
      manualScaleFactorValidator->setExclusive(true);
      declareProperty(
          new PropertyWithValue<double>("ManualScaleFactor", 1.0, manualScaleFactorValidator,
              Direction::Input), "Provided value for the scale factor. Optional.");
      declareProperty(
          new PropertyWithValue<double>("OutScaleFactor", Mantid::EMPTY_DBL(), Direction::Output),
          "The actual used value for the scaling factor.");
    }

    /**Gets the start of the overlapping region
     @param intesectionMin :: The minimum possible value for the overlapping region to inhabit
     @param intesectionMax :: The maximum possible value for the overlapping region to inhabit
     @return a double contianing the start of the overlapping region
     */
    double Stitch1D::getStartOverlap(const double& intesectionMin, const double& intesectionMax) const
    {
      Property* startOverlapProp = this->getProperty("StartOverlap");
      double startOverlapVal = this->getProperty("StartOverlap");
      startOverlapVal -= this->range_tolerance;
      const bool startOverlapBeyondRange = (startOverlapVal < intesectionMin)
          || (startOverlapVal > intesectionMax);
      if (startOverlapProp->isDefault() || startOverlapBeyondRange)
      {
        if (!startOverlapProp->isDefault() && startOverlapBeyondRange)
        {
          char message[200];
          std::sprintf(message,
              "StartOverlap is outside range at %0.4f, Min is %0.4f, Max is %0.4f . Forced to be: %0.4f",
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

    /**Gets the end of the overlapping region
     @param intesectionMin :: The minimum possible value for the overlapping region to inhabit
     @param intesectionMax :: The maximum possible value for the overlapping region to inhabit
     @return a double contianing the end of the overlapping region
     */
    double Stitch1D::getEndOverlap(const double& intesectionMin, const double& intesectionMax) const
    {
      Property* endOverlapProp = this->getProperty("EndOverlap");
      double endOverlapVal = this->getProperty("EndOverlap");
      endOverlapVal += this->range_tolerance;
      const bool endOverlapBeyondRange = (endOverlapVal < intesectionMin)
          || (endOverlapVal > intesectionMax);
      if (endOverlapProp->isDefault() || endOverlapBeyondRange)
      {
        if (!endOverlapProp->isDefault() && endOverlapBeyondRange)
        {
          char message[200];
          std::sprintf(message,
              "EndOverlap is outside range at %0.4f, Min is %0.4f, Max is %0.4f . Forced to be: %0.4f",
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

    /**Gets the rebinning parameters and calculates any missing values
     @param lhsWS :: The left hand side input workspace
     @param rhsWS :: The right hand side input workspace
     @param scaleRHS :: Scale the right hand side workspace
     @return a vector<double> contianing the rebinning parameters
     */
    MantidVec Stitch1D::getRebinParams(MatrixWorkspace_sptr& lhsWS, MatrixWorkspace_sptr& rhsWS,
        const bool scaleRHS) const
    {
      MantidVec inputParams = this->getProperty("Params");
      Property* prop = this->getProperty("Params");
      const bool areParamsDefault = prop->isDefault();

      const MantidVec& lhsX = lhsWS->readX(0);
      auto it = std::min_element(lhsX.begin(), lhsX.end());
      const double minLHSX = *it;

      const MantidVec& rhsX = rhsWS->readX(0);
      it = std::max_element(rhsX.begin(), rhsX.end());
      const double maxRHSX = *it;

      MantidVec result;
      if (areParamsDefault)
      {
        MantidVec calculatedParams;

        // Calculate the step size based on the existing step size of the LHS workspace. That way scale factors should be reasonably maintained.
        double calculatedStep = 0;
        if (scaleRHS)
        {
          // Calculate the step from the workspace that will not be scaled. The LHS workspace.
          calculatedStep = lhsX[1] - lhsX[0];
        }
        else
        {
          // Calculate the step from the workspace that will not be scaled. The RHS workspace.
          calculatedStep = rhsX[1] - rhsX[0];
        }

        calculatedParams.push_back(minLHSX);
        calculatedParams.push_back(calculatedStep);
        calculatedParams.push_back(maxRHSX);
        result = calculatedParams;
      }
      else
      {
        if (inputParams.size() == 1)
        {
          MantidVec calculatedParams;
          calculatedParams.push_back(minLHSX);
          calculatedParams.push_back(inputParams.front()); // Use the step supplied.
          calculatedParams.push_back(maxRHSX);
          result = calculatedParams;
        }
        else
        {
          result = inputParams; // user has provided params. Use those.
        }
      }
      return result;
    }

    /**Runs the Rebin Algorithm as a child
     @param input :: The input workspace
     @param params :: a vector<double> containing rebinning parameters
     @return A shared pointer to the resulting MatrixWorkspace
     */
    MatrixWorkspace_sptr Stitch1D::rebin(MatrixWorkspace_sptr& input, const MantidVec& params)
    {
      auto rebin = this->createChildAlgorithm("Rebin");
      rebin->setProperty("InputWorkspace", input);
      rebin->setProperty("Params", params);
      std::stringstream ssParams;
      ssParams << params[0] << "," << params[1] << "," << params[2];
      g_log.information("Rebinning Params: " + ssParams.str());
      rebin->execute();
      MatrixWorkspace_sptr outWS = rebin->getProperty("OutputWorkspace");

      const int histogramCount = static_cast<int>(outWS->getNumberHistograms());

      // Record special values and then mask them out as zeros. Special values are remembered and then replaced post processing.
      PARALLEL_FOR1(outWS)
      for (int i = 0; i < histogramCount; ++i)
      {
        PARALLEL_START_INTERUPT_REGION
        std::vector<size_t>& nanYIndexes = m_nanYIndexes[i];
        std::vector<size_t>& nanEIndexes = m_nanEIndexes[i];
        std::vector<size_t>& infYIndexes = m_infYIndexes[i];
        std::vector<size_t>& infEIndexes = m_infEIndexes[i];
        // Copy over the data
        MantidVec& sourceY = outWS->dataY(i);
        MantidVec& sourceE = outWS->dataE(i);

        for (size_t j = 0; j < sourceY.size(); ++j)
        {
          const double& value = sourceY[j];
          const double& eValue = sourceE[j];
          if (isNan(value))
          {
            nanYIndexes.push_back(j);
            sourceY[j] = 0;
          }
          else if (isInf(value))
          {
            infYIndexes.push_back(j);
            sourceY[j] = 0;
          }

          if (isNan(eValue))
          {
            nanEIndexes.push_back(j);
            sourceE[j] = 0;
          }
          else if (isInf(eValue))
          {
            infEIndexes.push_back(j);
            sourceE[j] = 0;
          }

        }

      PARALLEL_END_INTERUPT_REGION
      }
      PARALLEL_CHECK_INTERUPT_REGION

      return outWS;
    }

    /**Runs the Integration Algorithm as a child.
     @param input :: The input workspace
     @param start :: a double defining the start of the region to integrate
     @param stop :: a double defining the end of the region to integrate
     @return A shared pointer to the resulting MatrixWorkspace
     */
    MatrixWorkspace_sptr Stitch1D::integration(MatrixWorkspace_sptr& input, const double& start,
        const double& stop)
    {
      auto integration = this->createChildAlgorithm("Integration");
      integration->setProperty("InputWorkspace", input);
      integration->setProperty("RangeLower", start);
      integration->setProperty("RangeUpper", stop);
      g_log.information("Integration RangeLower: " + boost::lexical_cast<std::string>(start));
      g_log.information("Integration RangeUpper: " + boost::lexical_cast<std::string>(stop));
      integration->execute();
      MatrixWorkspace_sptr outWS = integration->getProperty("OutputWorkspace");
      return outWS;
    }

    /**Runs the MultiplyRange Algorithm as a child defining an end bin
     @param input :: The input workspace
     @param startBin :: The first bin int eh range to multiply
     @param endBin :: The last bin in the range to multiply
     @param factor :: The multiplication factor
     @return A shared pointer to the resulting MatrixWorkspace
     */
    MatrixWorkspace_sptr Stitch1D::multiplyRange(MatrixWorkspace_sptr& input, const int& startBin,
        const int& endBin, const double& factor)
    {
      auto multiplyRange = this->createChildAlgorithm("MultiplyRange");
      multiplyRange->setProperty("InputWorkspace", input);
      multiplyRange->setProperty("StartBin", startBin);
      multiplyRange->setProperty("EndBin", endBin);
      multiplyRange->setProperty("Factor", factor);
      g_log.information("MultiplyRange StartBin: " + boost::lexical_cast<std::string>(startBin));
      g_log.information("MultiplyRange EndBin: " + boost::lexical_cast<std::string>(endBin));
      g_log.information("MultiplyRange Factor: " + boost::lexical_cast<std::string>(factor));
      multiplyRange->execute();
      MatrixWorkspace_sptr outWS = multiplyRange->getProperty("OutputWorkspace");
      return outWS;
    }

    /**Runs the MultiplyRange Algorithm as a child
     @param input :: The input workspace
     @param startBin :: The first bin int eh range to multiply
     @param factor :: The multiplication factor
     @return A shared pointer to the resulting MatrixWorkspace
     */
    MatrixWorkspace_sptr Stitch1D::multiplyRange(MatrixWorkspace_sptr& input, const int& startBin,
        const double& factor)
    {
      auto multiplyRange = this->createChildAlgorithm("MultiplyRange");
      multiplyRange->setProperty("InputWorkspace", input);
      multiplyRange->setProperty("StartBin", startBin);
      multiplyRange->setProperty("Factor", factor);
      g_log.information("MultiplyRange StartBin: " + boost::lexical_cast<std::string>(startBin));
      g_log.information("MultiplyRange Factor: " + boost::lexical_cast<std::string>(factor));
      multiplyRange->execute();
      MatrixWorkspace_sptr outWS = multiplyRange->getProperty("OutputWorkspace");
      return outWS;
    }

    /**Runs the WeightedMean Algorithm as a child
     @param inOne :: The first input workspace
     @param inTwo :: The second input workspace
     @return A shared pointer to the resulting MatrixWorkspace
     */
    MatrixWorkspace_sptr Stitch1D::weightedMean(MatrixWorkspace_sptr& inOne, MatrixWorkspace_sptr& inTwo)
    {
      auto weightedMean = this->createChildAlgorithm("WeightedMean");
      weightedMean->setProperty("InputWorkspace1", inOne);
      weightedMean->setProperty("InputWorkspace2", inTwo);
      weightedMean->execute();
      MatrixWorkspace_sptr outWS = weightedMean->getProperty("OutputWorkspace");
      return outWS;
    }

    /**Runs the CreateSingleValuedWorkspace Algorithm as a child
     @param val :: The double to convert to a single value workspace
     @return A shared pointer to the resulting MatrixWorkspace
     */
    MatrixWorkspace_sptr Stitch1D::singleValueWS(double val)
    {
      auto singleValueWS = this->createChildAlgorithm("CreateSingleValuedWorkspace");
      singleValueWS->setProperty("DataValue", val);
      singleValueWS->execute();
      MatrixWorkspace_sptr outWS = singleValueWS->getProperty("OutputWorkspace");
      return outWS;
    }

    /**finds the bins containing the ends of the overlappign region
     @param startOverlap :: The start of the overlapping region
     @param endOverlap :: The end of the overlapping region
     @param workspace :: The workspace to determine the overlaps inside
     @return a boost::tuple<int,int> containing the bin indexes of the overlaps
     */
    boost::tuple<int, int> Stitch1D::findStartEndIndexes(double startOverlap, double endOverlap,
        MatrixWorkspace_sptr& workspace)
    {
      int a1 = static_cast<int>(workspace->binIndexOf(startOverlap));
      int a2 = static_cast<int>(workspace->binIndexOf(endOverlap));
      if (a1 == a2)
      {
        throw std::runtime_error(
            "The Params you have provided for binning yield a workspace in which start and end overlap appear in the same bin. Make binning finer via input Params.");
      }
      return boost::tuple<int, int>(a1, a2);

    }

    /**Determines if a workspace has non zero errors
     @param ws :: The input workspace
     @return True if there are any non-zero errors in the workspace
     */
    bool Stitch1D::hasNonzeroErrors(MatrixWorkspace_sptr ws)
    {
      int64_t ws_size = static_cast<int64_t>(ws->getNumberHistograms());
      bool hasNonZeroErrors = false;
      PARALLEL_FOR1(ws)
      for (int i = 0; i < ws_size; ++i)
      {
        PARALLEL_START_INTERUPT_REGION
        if (!hasNonZeroErrors) // Keep checking
        {
          auto e = ws->readE(i);
          auto it = std::find_if(e.begin(), e.end(), isNonzero);
          if (it != e.end())
          {
            PARALLEL_CRITICAL(has_non_zero)
            {
              hasNonZeroErrors = true; // Set flag. Should not need to check any more spectra.
            }
          }
        }
      PARALLEL_END_INTERUPT_REGION
    }
    PARALLEL_CHECK_INTERUPT_REGION

    return hasNonZeroErrors;
  }

//----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void Stitch1D::exec()
  {
    MatrixWorkspace_sptr rhsWS = this->getProperty("RHSWorkspace");
    MatrixWorkspace_sptr lhsWS = this->getProperty("LHSWorkspace");
    const MinMaxTuple intesectionXRegion = calculateXIntersection(lhsWS, rhsWS);

    const double intersectionMin = intesectionXRegion.get<0>();
    const double intersectionMax = intesectionXRegion.get<1>();

    const double startOverlap = getStartOverlap(intersectionMin, intersectionMax);
    const double endOverlap = getEndOverlap(intersectionMin, intersectionMax);

    if (startOverlap > endOverlap)
    {
      std::string message = boost::str(
          boost::format(
              "Stitch1D cannot have a StartOverlap > EndOverlap. StartOverlap: %0.9f, EndOverlap: %0.9f")
              % startOverlap % endOverlap);
      throw std::runtime_error(message);
    }

    const bool scaleRHS = this->getProperty("ScaleRHSWorkspace");
    MantidVec params = getRebinParams(lhsWS, rhsWS, scaleRHS);

    const double& xMin = params.front();
    const double& xMax = params.back();

    if (startOverlap < xMin)
    {
      std::string message =
          boost::str(
              boost::format(
                  "Stitch1D StartOverlap is outside the available X range after rebinning. StartOverlap: %10.9f, X min: %10.9f")
                  % startOverlap % xMin);

      throw std::runtime_error(message);
    }
    if (endOverlap > xMax)
    {
      std::string message =
          boost::str(
              boost::format(
                  "Stitch1D EndOverlap is outside the available X range after rebinning. EndOverlap: %10.9f, X max: %10.9f")
                  % endOverlap % xMax);

      throw std::runtime_error(message);
    }

    const size_t histogramCount = rhsWS->getNumberHistograms();
    m_nanYIndexes.resize(histogramCount);
    m_infYIndexes.resize(histogramCount);
    m_nanEIndexes.resize(histogramCount);
    m_infEIndexes.resize(histogramCount);
    auto rebinnedLHS = rebin(lhsWS, params);
    auto rebinnedRHS = rebin(rhsWS, params);

    boost::tuple<int, int> startEnd = findStartEndIndexes(startOverlap, endOverlap, rebinnedLHS);

    const bool useManualScaleFactor = this->getProperty("UseManualScaleFactor");
    double scaleFactor = 0;
    double errorScaleFactor = 0;

    if (useManualScaleFactor)
    {
      double manualScaleFactor = this->getProperty("ManualScaleFactor");
      MatrixWorkspace_sptr manualScaleFactorWS = singleValueWS(manualScaleFactor);

      if (scaleRHS)
      {
        rebinnedRHS = rebinnedRHS * manualScaleFactorWS;
      }
      else
      {
        rebinnedLHS = rebinnedLHS * manualScaleFactorWS;
      }
      scaleFactor = manualScaleFactor;
      errorScaleFactor = manualScaleFactor;
    }
    else
    {
      auto rhsOverlapIntegrated = integration(rebinnedRHS, startOverlap, endOverlap);
      auto lhsOverlapIntegrated = integration(rebinnedLHS, startOverlap, endOverlap);

      MatrixWorkspace_sptr ratio;
      if (scaleRHS)
      {
        ratio = lhsOverlapIntegrated / rhsOverlapIntegrated;
        rebinnedRHS = rebinnedRHS * ratio;
      }
      else
      {
        ratio = rhsOverlapIntegrated / lhsOverlapIntegrated;
        rebinnedLHS = rebinnedLHS * ratio;
      }
      scaleFactor = ratio->readY(0).front();
      errorScaleFactor = ratio->readE(0).front();
      if (scaleFactor < 1e-2 || scaleFactor > 1e2 || boost::math::isnan(scaleFactor))
      {
        std::stringstream messageBuffer;
        messageBuffer << "Stitch1D calculated scale factor is: " << scaleFactor
            << ". Check that in both input workspaces the integrated overlap region is non-zero.";
        g_log.warning(messageBuffer.str());
      }

    }

    int a1 = boost::tuples::get<0>(startEnd);
    int a2 = boost::tuples::get<1>(startEnd);

    // Mask out everything BUT the overlap region as a new workspace.
    MatrixWorkspace_sptr overlap1 = maskAllBut(a1, a2, rebinnedLHS);
    // Mask out everything BUT the overlap region as a new workspace.
    MatrixWorkspace_sptr overlap2 = maskAllBut(a1, a2, rebinnedRHS);
    // Mask out everything AFTER the overlap region as a new workspace.
    maskInPlace(a1 + 1, static_cast<int>(rebinnedLHS->blocksize()), rebinnedLHS);
    // Mask out everything BEFORE the overlap region as a new workspace.
    maskInPlace(0, a2, rebinnedRHS);

    MatrixWorkspace_sptr overlapave;
    if (hasNonzeroErrors(overlap1) && hasNonzeroErrors(overlap2))
    {
      overlapave = weightedMean(overlap1, overlap2);
    }
    else
    {
      g_log.information("Using un-weighted mean for Stitch1D overlap mean");
      MatrixWorkspace_sptr sum = overlap1 + overlap2;
      MatrixWorkspace_sptr denominator = singleValueWS(2.0);
      overlapave = sum / denominator;
    }

    MatrixWorkspace_sptr result = rebinnedLHS + overlapave + rebinnedRHS;
    reinsertSpecialValues(result);

    // Provide log information about the scale factors used in the calculations.
    std::stringstream messageBuffer;
    messageBuffer << "Scale Factor Y is: " << scaleFactor << " Scale Factor E is: " << errorScaleFactor;
    g_log.notice(messageBuffer.str());

    setProperty("OutputWorkspace", result);
    setProperty("OutScaleFactor", scaleFactor);

  }

  /**
   * Put special values back.
   * @param ws : MatrixWorkspace to resinsert special values into.
   */
  void Stitch1D::reinsertSpecialValues(MatrixWorkspace_sptr ws)
  {
    int histogramCount = static_cast<int>(ws->getNumberHistograms());
    PARALLEL_FOR1(ws)
    for (int i = 0; i < histogramCount; ++i)
    {
      PARALLEL_START_INTERUPT_REGION
      // Copy over the data
      MantidVec& sourceY = ws->dataY(i);

      for (size_t j = 0; j < m_nanYIndexes[i].size(); ++j)
      {
        sourceY[m_nanYIndexes[i][j]] = std::numeric_limits<double>::quiet_NaN();
      }

      for (size_t j = 0; j < m_infYIndexes[i].size(); ++j)
      {
        sourceY[m_infYIndexes[i][j]] = std::numeric_limits<double>::infinity();
      }

      for (size_t j = 0; j < m_nanEIndexes[i].size(); ++j)
      {
        sourceY[m_nanEIndexes[i][j]] = std::numeric_limits<double>::quiet_NaN();
      }

      for (size_t j = 0; j < m_infEIndexes[i].size(); ++j)
      {
        sourceY[m_infEIndexes[i][j]] = std::numeric_limits<double>::infinity();
      }

    PARALLEL_END_INTERUPT_REGION
  }
PARALLEL_CHECK_INTERUPT_REGION
}

} // namespace Algorithms
} // namespace Mantid
