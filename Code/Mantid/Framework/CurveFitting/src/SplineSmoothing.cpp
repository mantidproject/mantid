/*WIKI*
The algorithm performs a smoothing of the input data using a cubic spline. The algorithm takes a 2D workspace and generates a spline for each of the spectra to approximate a fit of the data.

Optionally, this algorithm can also calculate the first and second derivatives of each of the interpolated points as a side product. Setting the DerivOrder property to zero will force the algorithm to calculate no derivatives.

=== For Histogram Workspaces ===

If the input workspace contains histograms, rather than data points, then SplineInterpolation will automatically convert the input to point data. The output returned with be in the same format as the input.

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
    : M_START_SMOOTH_POINTS(10),
      m_cspline(boost::make_shared<CubicSpline>())
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
  const std::string SplineSmoothing::category() const { return "Optimization;CorrectionFunctions\\BackgroundCorrections";}

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void SplineSmoothing::initDocs()
  {
    this->setWikiSummary("Smoothes a set of spectra using a cubic spline. Optionally, this algorithm can also calculate derivatives up to order 2 as a side product");
    this->setOptionalMessage("Smoothes a set of spectra using a cubic spline. Optionally, this algorithm can also calculate derivatives up to order 2 as a side product");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void SplineSmoothing::init()
  {
    declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input),
        "The workspace on which to perform the smoothing algorithm.");

    declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output),
        "The workspace containing the calculated points");

    declareProperty(new WorkspaceProperty<WorkspaceGroup>("OutputWorkspaceDeriv","",
        Direction::Output, PropertyMode::Optional),
        "The workspace containing the calculated derivatives");

    auto validator = boost::make_shared<BoundedValidator<int> >(0,2);
    declareProperty("DerivOrder", 2, validator, "Order to derivatives to calculate.");

    auto errorSizeValidator = boost::make_shared<BoundedValidator<double> >();
    errorSizeValidator->setLower(0.0);
    declareProperty("Error", 0.05, errorSizeValidator, "The amount of error we wish to tolerate in smoothing");
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
    int histNo = static_cast<int>(inputWorkspaceBinned->getNumberHistograms());

    //convert binned data to point data is necessary
    MatrixWorkspace_sptr inputWorkspacePt = convertBinnedData(inputWorkspaceBinned);

    //output workspaces for points and derivs
    MatrixWorkspace_sptr outputWorkspace = setupOutputWorkspace(inputWorkspaceBinned, histNo);
    std::vector<MatrixWorkspace_sptr> derivs (histNo);

    for(int i = 0; i < histNo; ++i)
    {
      m_cspline = boost::make_shared<CubicSpline>();

      //choose some smoothing points from input workspace
      std::set<int> xPoints;
      selectSmoothingPoints(xPoints,inputWorkspacePt, i);
      performAdditionalFitting(inputWorkspacePt, i);

      //compare the data set against our spline
      outputWorkspace->setX(i, inputWorkspaceBinned->readX(i));
      calculateSmoothing(inputWorkspacePt, outputWorkspace, i);


      //calculate the derivatives, if required
      if(order > 0)
      {
        derivs[i] = setupOutputWorkspace(inputWorkspaceBinned, order);
        for(int j = 0; j < order; ++j)
        {
          derivs[i]->setX(j, inputWorkspaceBinned->readX(i));
          calculateDerivatives(inputWorkspacePt, derivs[i], j+1, i);
        }
      }
    }

    //prefix to name of deriv output workspaces
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

  /** Use a child fitting algorithm to tidy the smoothing
   *
   * @param ws :: The input workspace
   * @param row :: The row of spectra to use
   */
  void SplineSmoothing::performAdditionalFitting(const MatrixWorkspace_sptr& ws, const int row)
  {
    //perform additional fitting of the points
    auto fit = createChildAlgorithm("Fit");
    fit->setProperty("Function", boost::dynamic_pointer_cast<IFunction>(m_cspline));
    fit->setProperty("InputWorkspace",boost::static_pointer_cast<Workspace>(ws));
    fit->setProperty("WorkspaceIndex",row);
    fit->execute();
  }

  /**Copy the meta data for the input workspace to an output workspace and create it with the desired number of spectra.
   * Also labels the axis of each spectra with Yi, where i is the index
   *
   * @param inws :: The input workspace
   * @param size :: The number of spectra the workspace should be created with
   * @return The pointer to the newly created workspace
   */
  API::MatrixWorkspace_sptr SplineSmoothing::setupOutputWorkspace(API::MatrixWorkspace_sptr inws, int size) const
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

  /**Convert a binned workspace to point data
   *
   * @param workspace :: The input workspace
   * @return the converted workspace containing point data
   */
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

  /** Calculate smoothing of the data using the spline
   * Wraps CubicSpline function1D
   *
   * @param inputWorkspace :: The input workspace
   * @param outputWorkspace :: The output workspace
   * @param row :: The row of spectra to use
   */
  void SplineSmoothing::calculateSmoothing(
      MatrixWorkspace_const_sptr inputWorkspace,
      MatrixWorkspace_sptr outputWorkspace, size_t row) const
  {
    //define the spline's parameters
    const auto & xIn = inputWorkspace->readX(row);
    size_t nData = xIn.size();
    const double* xValues = xIn.data();
    double* yValues = outputWorkspace->dataY(row).data();

    //calculate the smoothing
    m_cspline->function1D(yValues, xValues, nData);
  }

  /** Calculate the derivatives of the newly smoothed data using the spline
   * Wraps CubicSpline derivative1D
   *
   * @param inputWorkspace :: The input workspace
   * @param outputWorkspace :: The output workspace
   * @param order :: The order of derivatives to calculate
   * @param row :: The row of spectra to use
   */
  void SplineSmoothing::calculateDerivatives(
      API::MatrixWorkspace_const_sptr inputWorkspace,
      API::MatrixWorkspace_sptr outputWorkspace, int order, size_t row) const
  {
      const auto & xIn = inputWorkspace->readX(row);
      const double* xValues = xIn.data();
      double* yValues = outputWorkspace->dataY(order-1).data();
      size_t nData = xIn.size();

      m_cspline->derivative1D(yValues, xValues, nData, order);
  }

  /** Sets the points defining the spline
   *
   * @param index :: The index of the attribute/parameter to set
   * @param xpoint :: The value of the x attribute
   * @param ypoint :: The value of the y parameter
   */
  void SplineSmoothing::setSmoothingPoint(const int index, const double xpoint, const double ypoint) const
  {
    //set the x and y values defining a point on the spline
     m_cspline->setXAttribute(index, xpoint);
     m_cspline->setParameter(index, ypoint);
  }

  /** Checks if the difference of each data point between the smoothing points falls within
   * the error tolerance.
   *
   * @param start :: The index of start checking accuracy at
   * @param end :: The index to stop checking accuracy at
   * @param ys :: The y data points from the noisy data
   * @param ysmooth :: The corresponding y data points defined by the spline
   * @return Boolean meaning if the values were accurate enough
   */
  bool SplineSmoothing::checkSmoothingAccuracy(const int start, const int end,
      const double* ys, const double* ysmooth) const
  {
    double error = getProperty("Error");

    //for all values between the selected indices
    for(int i = start; i < end; ++i)
    {
      //check if the difference between points is greater than our error tolerance
      double ydiff = fabs(ys[i] - ysmooth[i]);
      if(ydiff > error)
      {
        return false;
      }
    }

    return true;
  }

  /** Redefines the spline with a new set of points
   *
   * @param points :: The indices of the x/y points used to define the spline
   * @param xs :: The x data points from the noisy data
   * @param ys :: The y data points for the noisy data
   */
  void SplineSmoothing::addSmoothingPoints(const std::set<int>& points,
      const double* xs, const double* ys) const
  {
    //resize the number of attributes
    int size = static_cast<int>(points.size());
    m_cspline->setAttributeValue("n", size);

    //set each of the x and y points to redefine the spline
    std::set<int>::const_iterator iter;
    int i(0);
    for(iter = points.begin(); iter != points.end(); ++iter)
    {
      setSmoothingPoint(i, xs[*iter], ys[*iter]);
      ++i;
    }
  }

  /** Defines the points used to make the spline by iteratively creating more smoothing points
   * until all smoothing points fall within a certain error tolerance
   *
   * @param smoothPts :: The set of indices of the x/y points defining the spline
   * @param inputWorkspace :: The input workspace containing noisy data
   * @param row :: The row of spectra to use
   */
  void SplineSmoothing::selectSmoothingPoints(std::set<int>& smoothPts,
      MatrixWorkspace_const_sptr inputWorkspace, size_t row) const
  {
      const auto & xs = inputWorkspace->readX(row);
      const auto & ys = inputWorkspace->readY(row);

      int xSize = static_cast<int>(xs.size());

      //number of points to start with
      int numSmoothPts(M_START_SMOOTH_POINTS);

      //evenly space initial points over data set
      int delta = xSize / numSmoothPts;
      for (int i = 0; i < xSize; i+=delta)
      {
        smoothPts.insert(i);
      }
      smoothPts.insert(xSize-1); //add largest element to end of spline.

      addSmoothingPoints(smoothPts, xs.data(), ys.data());

      bool resmooth(true);
      while(resmooth)
      {
        resmooth = false;

        //calculate the spline and retrieve smoothed points
        boost::shared_array<double> ysmooth(new double[xSize]);
        m_cspline->function1D(ysmooth.get(),xs.data(),xSize);

        //iterate over smoothing points
        std::set<int>::const_iterator iter = smoothPts.begin();
        int start = *iter;
        int end(0);
        bool accurate(true);

        for(++iter; iter != smoothPts.end(); ++iter)
        {
          end = *iter;

          //check each point falls within our range of error.
          accurate = checkSmoothingAccuracy(start,end,ys.data(),ysmooth.get());

          //if not, flag for resmoothing and add another point between these two data points
          if(!accurate)
          {
            resmooth = true;
            smoothPts.insert((start+end)/2);
          }

          start = end;
          accurate = true;
        }

        //add new smoothing points if necessary
        if(resmooth)
        {
          addSmoothingPoints(smoothPts, xs.data(), ys.data());
        }
      }
  }

} // namespace CurveFitting
} // namespace Mantid
