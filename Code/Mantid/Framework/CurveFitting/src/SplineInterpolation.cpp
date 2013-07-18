/*WIKI*
 TODO: Enter a full wiki-markup description of your algorithm here. You can then use the Build/wiki_maker.py script to generate your full wiki page.
 *WIKI*/

#include "MantidAPI/TextAxis.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidCurveFitting/SplineInterpolation.h"

namespace Mantid
{
  namespace CurveFitting
  {

    // Register the algorithm into the AlgorithmFactory
    DECLARE_ALGORITHM(SplineInterpolation);

    using namespace API;
    using namespace Kernel;

    //----------------------------------------------------------------------------------------------
    /** Constructor
     */
    SplineInterpolation::SplineInterpolation()
    {
    }

    //----------------------------------------------------------------------------------------------
    /** Destructor
     */
    SplineInterpolation::~SplineInterpolation()
    {
    }

    //----------------------------------------------------------------------------------------------
    /// Algorithm's name for identification. @see Algorithm::name
    const std::string SplineInterpolation::name() const
    {
      return "SplineInterpolation";
    }
    ;

    /// Algorithm's version for identification. @see Algorithm::version
    int SplineInterpolation::version() const
    {
      return 1;
    }
    ;

    /// Algorithm's category for identification. @see Algorithm::category
    const std::string SplineInterpolation::category() const
    {
      return "General";
    }

    //----------------------------------------------------------------------------------------------
    /// Sets documentation strings for this algorithm
    void SplineInterpolation::initDocs()
    {
      this->setWikiSummary("TODO: Enter a quick description of your algorithm.");
      this->setOptionalMessage("TODO: Enter a quick description of your algorithm.");
    }

    //----------------------------------------------------------------------------------------------
    /** Initialize the algorithm's properties.
     */
    void SplineInterpolation::init()
    {
      declareProperty(new WorkspaceProperty<>("WorkspaceToInterpolate", "", Direction::Input),
          "The workspace on which to perform interpolation the algorithm.");
      declareProperty(new WorkspaceProperty<>("WorkspaceToMatch", "", Direction::Input),
          "The workspace which defines the points of the spline.");
      declareProperty(new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output),
          "The workspace containing the calculated points and derivatives");

      auto validator = boost::make_shared<BoundedValidator<int> >(0, 2);
      declareProperty("DerivOrder", 2, validator, "Order to derivatives to calculate.");
    }

    //----------------------------------------------------------------------------------------------
    /** Execute the algorithm.
     */
    void SplineInterpolation::exec()
    {
      //read in algorithm parameters
      int order = static_cast<int>(getProperty("DerivOrder"));

      MatrixWorkspace_sptr mws = getProperty("WorkspaceToMatch");
      MatrixWorkspace_sptr iws = getProperty("WorkspaceToInterpolate");

      MatrixWorkspace_const_sptr mwspt = convertBinnedData(mws);
      MatrixWorkspace_const_sptr iwspt = convertBinnedData(iws);

      MatrixWorkspace_sptr outputWorkspace = WorkspaceFactory::Instance().create(mws,
          order + 1);

      //set axis labels for output workspace
      API::TextAxis* tAxis = new API::TextAxis(order+1);
      tAxis->setLabel(0, "Y");
      tAxis->setLabel(1, "Deriv 1");
      tAxis->setLabel(2, "Deriv 2");
      outputWorkspace->replaceAxis(1, tAxis);

      //Create and instance of the cubic spline function
      auto cspline = boost::make_shared<CubicSpline>();

      //set the interpolation points
      setInterpolationPoints(cspline, iwspt);

      //compare the data set against our spline
      calculateSpline(cspline, mwspt, outputWorkspace, order);

      //store the output workspace
      setProperty("OutputWorkspace", outputWorkspace);
    }

    MatrixWorkspace_sptr SplineInterpolation::convertBinnedData(MatrixWorkspace_sptr workspace) const
    {
      if(workspace->isHistogramData())
      {
        const auto & xValues = workspace->readX(0);
        const auto & yValues = workspace->readY(0);
        size_t size = xValues.size()-1;

        //make a new workspace for the point data
        MatrixWorkspace_sptr pointWorkspace = WorkspaceFactory::Instance().create(workspace,1,size,size);
        auto & newXValues = pointWorkspace->dataX(0);
        auto & newYValues = pointWorkspace->dataY(0);

        //take middle point of bin boundaries
        for(size_t i = 0; i < size; ++i)
        {
          newXValues[i] = (xValues[i] + xValues[i+1])/2;
          newYValues[i] = yValues[i];
        }

        return pointWorkspace;
      }

      return workspace;
    }

    void SplineInterpolation::setInterpolationPoints(CubicSpline_const_sptr cspline,
        MatrixWorkspace_const_sptr inputWorkspace) const
    {
      const auto & xIn = inputWorkspace->readX(0);
      const auto & yIn = inputWorkspace->readY(0);
      int size = static_cast<int>(xIn.size());

      //pass x attributes and y parameters to CubicSpline
      cspline->setAttributeValue("n", size);

      for (int i = 0; i < size; ++i)
      {
        cspline->setXAttribute(i, xIn[i]);
        cspline->setParameter(i, yIn[i]);
      }
    }

    void SplineInterpolation::calculateSpline(CubicSpline_const_sptr cspline,
        MatrixWorkspace_const_sptr inputWorkspace, MatrixWorkspace_sptr outputWorkspace, int order) const
    {
      //setup input parameters
      size_t nData = inputWorkspace->readY(0).size();
      const double* xValues = inputWorkspace->readX(0).data();
      double* yValues = outputWorkspace->dataY(0).data();
      outputWorkspace->setX(0, inputWorkspace->readX(0));

      //calculate the interpolation
      cspline->function1D(yValues, xValues, nData);

      //calculate the derivatives
      for (int i = 1; i <= order; ++i)
      {
        outputWorkspace->setX(i, inputWorkspace->readX(0));
        yValues = outputWorkspace->dataY(i).data();
        cspline->derivative1D(yValues, xValues, nData, i);
      }
    }

  } // namespace CurveFitting
} // namespace Mantid
