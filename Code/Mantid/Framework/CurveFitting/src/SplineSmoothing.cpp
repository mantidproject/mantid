/*WIKI*
TODO: Enter a full wiki-markup description of your algorithm here. You can then use the Build/wiki_maker.py script to generate your full wiki page.
*WIKI*/

#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidCurveFitting/SplineSmoothing.h"


#include <algorithm>

namespace Mantid
{
namespace CurveFitting
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(SplineSmoothing);
  
  using namespace API;
  using namespace Kernel;

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  SplineSmoothing::SplineSmoothing()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  SplineSmoothing::~SplineSmoothing()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string SplineSmoothing::name() const { return "SplineSmoothing";};
  
  /// Algorithm's version for identification. @see Algorithm::version
  int SplineSmoothing::version() const { return 1;};
  
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string SplineSmoothing::category() const { return "General";}

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void SplineSmoothing::initDocs()
  {
    this->setWikiSummary("TODO: Enter a quick description of your algorithm.");
    this->setOptionalMessage("TODO: Enter a quick description of your algorithm.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void SplineSmoothing::init()
  {
    declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input), "The workspace on which to perform the smoothing algorithm.");
    declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output), "The workspace containing the calculated points and derivatives");

    auto validator = boost::make_shared<BoundedValidator<int> >(0,2);
    declareProperty("DerivOrder", 2, validator, "Order to derivatives to calculate.");

    auto splineSizeValidator = boost::make_shared<BoundedValidator<int> >();
    splineSizeValidator->setLower(3);
    declareProperty("SplineSize", 3, splineSizeValidator, "Number of points defining the spline.");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void SplineSmoothing::exec()
  {
    MatrixWorkspace_sptr inputWorkspaceBinned = getProperty("InputWorkspace");

    //read in algorithm parameters
    int order = static_cast<int>(getProperty("DerivOrder"));
    //number of input histograms.
    size_t histNo = inputWorkspaceBinned->getNumberHistograms();

    //convert binned data to point data is necessary
    MatrixWorkspace_sptr inputWorkspacePt = convertBinnedData(inputWorkspaceBinned);

    //output workspaces for points and derivs
    MatrixWorkspace_sptr outputWorkspace = setupOutputWorkspace(inputWorkspaceBinned);
    std::vector<MatrixWorkspace_sptr> derivs (order);

    //set up workspaces for output derivatives
    for(int i=1; i<=order; ++i)
    {
      derivs[i-1] = setupOutputWorkspace(inputWorkspaceBinned);
    }

    for(size_t i = 0; i < histNo; ++i)
    {
      //Create and instance of the cubic spline function
      auto cspline = boost::make_shared<CubicSpline>();

      //choose some smoothing points from input workspace
      setSmoothingPoints(cspline, inputWorkspacePt, i);

      //compare the data set against our spline
      outputWorkspace->setX(i, inputWorkspaceBinned->readX(i));
      calculateSmoothing(cspline, inputWorkspacePt, outputWorkspace, i);

      for(int j = 1; j <= order; ++j)
      {
        derivs[j-1]->setX(i, inputWorkspaceBinned->readX(i));
        calculateDerivatives(cspline, inputWorkspacePt, derivs[j-1], j, i);
      }
    }

    //store the output workspaces
    setProperty("OutputWorkspace", outputWorkspace);
    //prefix to name of deriv output workspaces
    std::string owsPrefix = getPropertyValue("OutputWorkspace") + "_";
    for(int i = 0; i < order; ++i)
    {
      std::string index = boost::lexical_cast<std::string>(i+1);
      std::string ows = "OutputWorkspace_" + index;

      //declare new output workspace for derivatives
      declareProperty(new WorkspaceProperty<>(ows,owsPrefix+index,Direction::Output),
          "Workspace containing the order " + index + " derivatives");

      setProperty(ows, derivs[i]);
    }
  }

  API::MatrixWorkspace_sptr SplineSmoothing::setupOutputWorkspace(API::MatrixWorkspace_sptr inws) const
  {
    size_t histNo = inws->getNumberHistograms();
    MatrixWorkspace_sptr outputWorkspace = WorkspaceFactory::Instance().create(inws,histNo);

    //create labels for output workspace
    API::TextAxis* tAxis = new API::TextAxis(histNo);
    for(size_t i=0; i < histNo; ++i)
    {
      std::string index = boost::lexical_cast<std::string>(i);
      tAxis->setLabel(i, "Y"+index);
    }
    outputWorkspace->replaceAxis(1, tAxis);

    return outputWorkspace;

  }

  MatrixWorkspace_sptr SplineSmoothing::convertBinnedData(MatrixWorkspace_sptr workspace) const
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

  void SplineSmoothing::calculateSmoothing(const boost::shared_ptr<CubicSpline> cspline,
      MatrixWorkspace_const_sptr inputWorkspace,
      MatrixWorkspace_sptr outputWorkspace, size_t row) const
  {
    //define the spline's parameters
    const auto & xIn = inputWorkspace->readX(row);
    size_t nData = xIn.size();
    const double* xValues = xIn.data();
    double* yValues = outputWorkspace->dataY(row).data();

    //calculate the smoothing
    cspline->function1D(yValues, xValues, nData);
  }

  void SplineSmoothing::calculateDerivatives(const boost::shared_ptr<CubicSpline> cspline,
      API::MatrixWorkspace_const_sptr inputWorkspace,
      API::MatrixWorkspace_sptr outputWorkspace, int order, size_t row) const
  {
      const auto & xIn = inputWorkspace->readX(row);
      const double* xValues = xIn.data();
      double* yValues = outputWorkspace->dataY(row).data();
      size_t nData = xIn.size();

      cspline->derivative1D(yValues, xValues, nData, order);
  }

  void SplineSmoothing::setSmoothingPoints(const boost::shared_ptr<CubicSpline> cspline,
      MatrixWorkspace_const_sptr inputWorkspace, size_t row) const
  {
      //define the spline's parameters
      const auto & xIn = inputWorkspace->readX(row);
      const auto & yIn = inputWorkspace->readY(row);

      int xSize = static_cast<int>(xIn.size());
      int numPoints = getProperty("SplineSize");

      //check number of spline points is within a valid range
      if(numPoints > xSize)
      {
        throw std::range_error("SplineSmoothing: Spline size cannot be larger than the number of data points.");
      }

      //choose number of smoothing points
      double deltaX = (xIn.back() - xIn.front()) / (numPoints-1);
      double targetX = 0;
      int lastIndex = 0;
      int attrCount = 0;

      for (int i = 0; i < numPoints; ++i)
      {
        //increment x position
        targetX = xIn[0] + (i * deltaX);

        //find closest x point
        while(lastIndex < xSize-1 && xIn[lastIndex] <= targetX)
        {
          ++lastIndex;
        }

        //get x value with minimum difference.
        int index = (xIn[lastIndex] - targetX < targetX - xIn[lastIndex-1]) ? lastIndex : lastIndex-1;

        std::string attrName = "x" + boost::lexical_cast<std::string>(attrCount-1);
        if(i == 0 || cspline->getAttribute(attrName).asDouble() != xIn[index])
        {
            if(attrCount >= 3)
            {
              cspline->setAttributeValue("n", attrCount+1);
            }

            cspline->setXAttribute(attrCount, xIn[index]);
            cspline->setParameter(attrCount, yIn[index]);

            attrCount++;
        }
      }
  }

} // namespace CurveFitting
} // namespace Mantid
