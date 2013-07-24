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
      : m_cspline(boost::make_shared<CubicSpline>())
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
      declareProperty(new WorkspaceProperty<>("WorkspaceToMatch", "", Direction::Input),
          "The workspace which defines the points of the spline.");

      declareProperty(new WorkspaceProperty<>("WorkspaceToInterpolate", "", Direction::Input),
          "The workspace on which to perform the interpolation algorithm.");

      declareProperty(new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output),
          "The workspace containing the calculated points and derivatives");

      declareProperty(new WorkspaceProperty<WorkspaceGroup>("OutputWorkspaceDeriv","",
          Direction::Output, PropertyMode::Optional),
          "The workspace containing the calculated derivatives");

      auto validator = boost::make_shared<BoundedValidator<int> >();
      validator->setLower(0);
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

      int histNo = static_cast<int>(iws->getNumberHistograms());

      std::vector<MatrixWorkspace_sptr> derivs(histNo);

      if(mws->getNumberHistograms() > 1)
      {
        g_log.warning() << "Algorithm can only interpolate against a single data set. "
            "Only the first data set will be used." << std::endl;
      }

      MatrixWorkspace_const_sptr mwspt = convertBinnedData(mws);
      MatrixWorkspace_const_sptr iwspt = convertBinnedData(iws);

      MatrixWorkspace_sptr outputWorkspace = setupOutputWorkspace(iws, histNo);


      for (int i = 0; i < histNo; ++i)
      {
        //Create and instance of the cubic spline function
        m_cspline = boost::make_shared<CubicSpline>();
        //set the interpolation points
        setInterpolationPoints(mwspt, i);

        //compare the data set against our spline
        calculateSpline(iwspt, outputWorkspace, i);
        outputWorkspace->setX(i, iws->readX(i));

        if(order > 0)
        {
          derivs[i] = setupOutputWorkspace(iws, order);
          for(int j = 0; j < order; ++j)
          {

            derivs[i]->setX(j, iws->readX(i));

            if(j >= 2)
            {
              setYPoints(derivs[i]->readY(j-1).data());
            }

            calculateDerivatives(iwspt, derivs[i], j+1, i);
          }
        }
      }

      if(order > 0)
      {
        WorkspaceGroup_sptr wsg = WorkspaceGroup_sptr(new WorkspaceGroup);

        for(int i = 0; i < histNo; ++i)
        {
          wsg->addWorkspace(derivs[i]);
        }

        setProperty("OutputWorkspaceDeriv", wsg);
      }

      setProperty("OutputWorkspace", outputWorkspace);
    }

    API::MatrixWorkspace_sptr SplineInterpolation::setupOutputWorkspace(API::MatrixWorkspace_sptr inws, int size) const
    {
      MatrixWorkspace_sptr outputWorkspace = WorkspaceFactory::Instance().create(inws,size);

      //create labels for output workspace
      API::TextAxis* tAxis = new API::TextAxis(size);
      for(int i=0; i < size; ++i)
      {
        std::string index = boost::lexical_cast<std::string>(i);
        tAxis->setLabel(i, "Y"+index);
      }
      outputWorkspace->replaceAxis(1, tAxis);

      return outputWorkspace;
    }


    MatrixWorkspace_sptr SplineInterpolation::convertBinnedData(MatrixWorkspace_sptr workspace) const
    {
      if(workspace->isHistogramData())
      {
        size_t histNo = workspace->getNumberHistograms();
        size_t size = workspace->readY(0).size();

        //make a new workspace for the point data
        MatrixWorkspace_sptr pointWorkspace = WorkspaceFactory::Instance().create(workspace,histNo,size,size);

        //loop over each histogram
        for(size_t i=0; i < histNo; ++i)
        {
          const auto & xValues = workspace->readX(i);
          const auto & yValues = workspace->readY(i);

          auto & newXValues = pointWorkspace->dataX(i);
          auto & newYValues = pointWorkspace->dataY(i);

          //set x values to be average of bin bounds
          for(size_t j = 0; j < size; ++j)
          {
            newXValues[j] = (xValues[j] + xValues[j+1])/2;
            newYValues[j] = yValues[j];
          }
        }

        return pointWorkspace;
      }

      return workspace;
    }

    void SplineInterpolation::setYPoints(const double* ys) const
    {
      int n = m_cspline->getAttribute("n").asInt();
      for (int i =0; i < n; ++i)
      {
        m_cspline->setParameter(i, ys[i]);
      }
    }

    void SplineInterpolation::setInterpolationPoints(MatrixWorkspace_const_sptr inputWorkspace, const int row) const
    {
      const auto & xIn = inputWorkspace->readX(row);
      const auto & yIn = inputWorkspace->readY(row);
      int size = static_cast<int>(xIn.size());

      //pass x attributes and y parameters to CubicSpline
      m_cspline->setAttributeValue("n", size);

      for (int i = 0; i < size; ++i)
      {
        m_cspline->setXAttribute(i, xIn[i]);
        m_cspline->setParameter(i, yIn[i]);
      }
    }

    void SplineInterpolation::calculateDerivatives(API::MatrixWorkspace_const_sptr inputWorkspace,
        API::MatrixWorkspace_sptr outputWorkspace, int order, int row) const
    {
      size_t nData = inputWorkspace->readY(row).size();
      const double* xValues = inputWorkspace->readX(row).data();
      double* yValues = outputWorkspace->dataY(order-1).data();

      m_cspline->derivative1D(yValues, xValues, nData, order);
    }

    void SplineInterpolation::calculateSpline(MatrixWorkspace_const_sptr inputWorkspace,
        MatrixWorkspace_sptr outputWorkspace, int row) const
    {
      //setup input parameters

      size_t nData = inputWorkspace->readY(row).size();
      const double* xValues = inputWorkspace->readX(row).data();
      double* yValues = outputWorkspace->dataY(row).data();

      //calculate the interpolation
      m_cspline->function1D(yValues, xValues, nData);
    }

  } // namespace CurveFitting
} // namespace Mantid
