/*WIKI*
TODO: Enter a full wiki-markup description of your algorithm here. You can then use the Build/wiki_maker.py script to generate your full wiki page.
*WIKI*/

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
  const std::string SplineInterpolation::name() const { return "SplineInterpolation";};
  
  /// Algorithm's version for identification. @see Algorithm::version
  int SplineInterpolation::version() const { return 1;};
  
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string SplineInterpolation::category() const { return "General";}

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
    declareProperty(new WorkspaceProperty<>("WorkspaceToInterpolate","",Direction::Input), "The workspace on which to perform interpolation the algorithm.");
    declareProperty(new WorkspaceProperty<>("WorkspaceToMatch","",Direction::Input), "The workspace which defines the points of the spline.");
    declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output), "The workspace containing the calculated points and derivatives");

    auto validator = boost::make_shared<BoundedValidator<int> >(0,2);
    declareProperty("Order", 2, validator, "Order to derivatives to calculate.");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void SplineInterpolation::exec()
  {
    //read in algorithm parameters
    int order = static_cast<int>(getProperty("Order"));

    MatrixWorkspace_const_sptr interpolateWorkspace = getProperty("WorkspaceToInterpolate");
    MatrixWorkspace_const_sptr matchWorkspace = getProperty("WorkspaceToMatch");
    MatrixWorkspace_sptr outputWorkspace = WorkspaceFactory::Instance().create(interpolateWorkspace,order+1);

    //Create and instance of the cubic spline function
    auto cspline = boost::make_shared<CubicSpline>();

    //set the interpolation points
    setInterpolationPoints(cspline, matchWorkspace);

    //compare the data set against our spline
    calculateSpline(cspline, interpolateWorkspace, outputWorkspace, order);

    //store the output workspace
    setProperty("OutputWorkspace", outputWorkspace);
  }

  void SplineInterpolation::setInterpolationPoints(const boost::shared_ptr<CubicSpline> cspline,
      MatrixWorkspace_const_sptr inputWorkspace) const
  {
    const auto & xIn = inputWorkspace->readX(0);
    const auto & yIn = inputWorkspace->readY(0);
    int xSize = static_cast<int>(xIn.size());

    cspline->setAttributeValue("n", xSize);

    for(int i = 0; i < xSize; ++i)
    {
      cspline->setXAttribute(i, xIn[i]);

      cspline->setParameter(i, yIn[i]);
    }

  }

  void SplineInterpolation::calculateSpline(const boost::shared_ptr<CubicSpline> cspline,
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
      cspline->derivative1D(yValues, xValues, nData, order);
    }
  }



} // namespace CurveFitting
} // namespace Mantid
