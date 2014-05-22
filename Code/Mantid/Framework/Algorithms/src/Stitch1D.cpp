/*WIKI*
 TODO: Enter a full wiki-markup description of your algorithm here. You can then use the Build/wiki_maker.py script to generate your full wiki page.
 *WIKI*/

#include "MantidAlgorithms/Stitch1D.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/RebinParamsValidator.h"

#include <boost/make_shared.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
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
}

namespace Mantid
{
  namespace Algorithms
  {

    // Register the algorithm into the AlgorithmFactory
    DECLARE_ALGORITHM(Stitch1D)

    //----------------------------------------------------------------------------------------------
    /** Constructor
     */
    Stitch1D::Stitch1D()
    {
    }

    //----------------------------------------------------------------------------------------------
    /** Destructor
     */
    Stitch1D::~Stitch1D()
    {
    }

    //----------------------------------------------------------------------------------------------
    /// Algorithm's name for identification. @see Algorithm::name
    const std::string Stitch1D::name() const
    {
      return "Stitch1D";
    }
    ;

    /// Algorithm's version for identification. @see Algorithm::version
    int Stitch1D::version() const
    {
      return 4;
    }
    ;

    /// Algorithm's category for identification. @see Algorithm::category
    const std::string Stitch1D::category() const
    {
      return "Reflectometry";
    }

    //----------------------------------------------------------------------------------------------
    /// Sets documentation strings for this algorithm
    void Stitch1D::initDocs()
    {
      this->setWikiSummary("Stitches single histogram matrix workspaces together");
      this->setOptionalMessage(this->getWikiSummary());
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
      declareProperty(new ArrayProperty<double>("Params", boost::make_shared<RebinParamsValidator>(true)),
          "Rebinning Parameters. See Rebin for format. If only a single value is provided, start and end are taken from input workspaces.");
      declareProperty(new PropertyWithValue<bool>("ScaleRHSWorkspace", true, Direction::Input),
          "Scaling either with respect to workspace 1 or workspace 2");
      declareProperty(new PropertyWithValue<bool>("UseManualScaleFactor", false, Direction::Input),
          "True to use a provided value for the scale factor.");
      declareProperty(
          new PropertyWithValue<double>("OutScaleFactor", Mantid::EMPTY_DBL(), Direction::Output),
          "The actual used value for the scaling factor.");

    }

    double Stitch1D::getStartOverlap(const double& intesectionMin, const double& intesectionMax) const
    {
      Property* startOverlapProp = this->getProperty("StartOverlap");
      double startOverlapVal = this->getProperty("StartOverlap");
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

    double Stitch1D::getEndOverlap(const double& intesectionMin, const double& intesectionMax) const
    {
      Property* endOverlapProp = this->getProperty("EndOverlap");
      double endOverlapVal = this->getProperty("EndOverlap");
      const bool endOverlapBeyondRange = (endOverlapVal < intesectionMin)
          || (endOverlapVal > intesectionMax);
      if (endOverlapProp->isDefault() || endOverlapBeyondRange)
      {
        if (!endOverlapProp->isDefault() && endOverlapBeyondRange)
        {
          char message[200];
          std::sprintf(message,
              "EndOverlap is outside range at %0.4f, Min is %0.4f, Max is %0.4f . Forced to be: %0.4f",
              endOverlapVal, intesectionMin, intesectionMax, intesectionMin);
          g_log.warning(std::string(message));
        }
        endOverlapVal = intesectionMax;
        std::stringstream buffer;
        buffer << "StartOverlap calculated to be: " << endOverlapVal;
        g_log.information(buffer.str());
      }
      return endOverlapVal;
    }

    MantidVec Stitch1D::getRebinParams(MatrixWorkspace_sptr& lhsWS, MatrixWorkspace_sptr& rhsWS) const
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

        const double calculatedStep = (maxRHSX - minLHSX) / 100;
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

    MatrixWorkspace_sptr Stitch1D::rebin(MatrixWorkspace_sptr& input, const MantidVec& params)
    {
      auto rebin = this->createChildAlgorithm("Rebin");
      rebin->setProperty("InputWorkspace", input);
      rebin->setProperty("Params", params);
      rebin->execute();
      MatrixWorkspace_sptr outWS = rebin->getProperty("OutputWorkspace");
      return outWS;
    }

    //----------------------------------------------------------------------------------------------
    /** Execute the algorithm.
     */
    void Stitch1D::exec()
    {
      MatrixWorkspace_sptr rhsWS = this->getProperty("RHSWorkspace");
      MatrixWorkspace_sptr lhsWS = this->getProperty("LHSWorkspace");
      const MinMaxTuple intesectionXRegion = calculateXIntersection(rhsWS, lhsWS);

      const double intersectionMin = intesectionXRegion.get<0>();
      const double intersectionMax = intesectionXRegion.get<1>();

      const double startOverlap = getStartOverlap(intersectionMin, intersectionMax);
      const double endOverlap = getEndOverlap(intersectionMin, intersectionMax);

      if (startOverlap > endOverlap)
      {
        std::string message =
            boost::str(
                boost::format(
                    "Stitch1D cannot have a StartOverlap > EndOverlap. StartOverlap: %0.9f, EndOverlap: %0.9f")
                    % startOverlap % endOverlap);
        throw std::runtime_error(message);
      }

      MantidVec params = getRebinParams(lhsWS, rhsWS);

      auto rebinnedLHS = rebin(lhsWS, params);
      auto rebinnedRHS = rebin(rhsWS, params);

      setProperty("OutputWorkspace", rhsWS);
      setProperty("OutScaleFactor", 99.0);

    }

  } // namespace Algorithms
} // namespace Mantid
