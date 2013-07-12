/*WIKI*
TODO: Enter a full wiki-markup description of your algorithm here. You can then use the Build/wiki_maker.py script to generate your full wiki page.
*WIKI*/

#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/FunctionFactory.h"
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
    declareProperty("Order", 2, validator, "Order to derivatives to calculate.");

    auto splineSizeValidator = boost::make_shared<BoundedValidator<int> >();
    splineSizeValidator->setLower(3);
    declareProperty("SplineSize", 3, splineSizeValidator, "Number of points defining the spline.");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void SplineSmoothing::exec()
  {
    //read in algorithm parameters
    int order = static_cast<int>(getProperty("Order"));

    MatrixWorkspace_const_sptr inputWorkspace = getProperty("InputWorkspace");
    MatrixWorkspace_sptr outputWorkspace = WorkspaceFactory::Instance().create(inputWorkspace,order+1);

    //Create and instance of the cubic spline function
    auto cspline = boost::make_shared<CubicSpline>();

    //choose somre smoothing points from input workspace
    setSmoothingPoints(cspline, inputWorkspace);

    //compare the data set against our spline
    calculateSpline(cspline, inputWorkspace, outputWorkspace, order);

    //store the output workspace
    setProperty("OutputWorkspace", outputWorkspace);
  }

  void SplineSmoothing::calculateSpline(const boost::shared_ptr<CubicSpline> cspline,
      MatrixWorkspace_const_sptr inputWorkspace,
      MatrixWorkspace_sptr outputWorkspace, int order) const
  {
    //define the spline's parameters
    const auto & xIn = inputWorkspace->readX(0);

    //setup input parameters
    size_t nData = xIn.size();
    const double* xValues = xIn.data();
    double* yValues = outputWorkspace->dataY(0).data();
    outputWorkspace->setX(0, inputWorkspace->readX(0));

    //calculate the interpolation
    cspline->function1D(yValues, xValues, nData);

    //calculate the derivatives
    for(int i = 1; i <= order; ++i)
    {
      outputWorkspace->setX(i, inputWorkspace->readX(i));

      yValues = outputWorkspace->dataY(i).data();
      cspline->derivative1D(yValues, xValues, nData, i);
    }
  }

  void SplineSmoothing::setSmoothingPoints(const boost::shared_ptr<CubicSpline> cspline,
      MatrixWorkspace_const_sptr inputWorkspace) const
  {
      //define the spline's parameters
      const auto & xIn = inputWorkspace->readX(0);
      const auto & yIn = inputWorkspace->readY(0);

      int xSize = static_cast<int>(xIn.size());
      int numPoints = getProperty("SplineSize");

      //check number of spline points is within a valid range
      if(numPoints > xSize)
      {
        throw std::range_error("SplineSmoothing: SplineSmoothing size cannot be larger than the number of data points.");
      }

      //set number of smoothing points


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
