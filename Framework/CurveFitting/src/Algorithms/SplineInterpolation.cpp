#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidCurveFitting/Algorithms/SplineInterpolation.h"

namespace Mantid {
namespace CurveFitting {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SplineInterpolation)

using namespace Kernel;

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string SplineInterpolation::name() const {
  return "SplineInterpolation";
}

/// Algorithm's version for identification. @see Algorithm::version
int SplineInterpolation::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string SplineInterpolation::category() const {
  return "Optimization;CorrectionFunctions\\BackgroundCorrections";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void SplineInterpolation::init() {
  declareProperty(make_unique<WorkspaceProperty<>>("WorkspaceToMatch", "",
                                                   Direction::Input),
                  "The workspace which defines the points of the spline.");

  declareProperty(
      make_unique<WorkspaceProperty<>>("WorkspaceToInterpolate", "",
                                       Direction::Input),
      "The workspace on which to perform the interpolation algorithm.");

  declareProperty(
      make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                       Direction::Output),
      "The workspace containing the calculated points and derivatives");

  declareProperty(make_unique<WorkspaceProperty<WorkspaceGroup>>(
                      "OutputWorkspaceDeriv", "", Direction::Output,
                      PropertyMode::Optional),
                  "The workspace containing the calculated derivatives");

  auto validator = boost::make_shared<BoundedValidator<int>>(0, 2);
  declareProperty("DerivOrder", 0, validator,
                  "Order to derivatives to calculate (default 0 "
                  "omits calculation)");

  declareProperty("Linear2Points", false,
                  "Set to true to perform linear interpolation for 2 points "
                  "instead.");
}

//----------------------------------------------------------------------------------------------
/** Input validation for the WorkspaceToInterpolate
 * If more than two points given, perform cubic spline interpolation
 * If only two points given, check if Linear2Points is true and if true,
 * continue
 * If one point is given interpolation does not make sense
  */
std::map<std::string, std::string> SplineInterpolation::validateInputs() {
  // initialise map (result)
  std::map<std::string, std::string> result;

  // get inputs that need validation
  const bool lin2pts = getProperty("Linear2Points");

  MatrixWorkspace_sptr iws_valid = getProperty("WorkspaceToInterpolate");
  int binsNo = static_cast<int>(iws_valid->blocksize());

  // The minimum number of points for cubic splines is 3,
  // used and set by function CubicSpline as well
  switch (binsNo) {
  case 1:
    result["WorkspaceToInterpolate"] =
        "Workspace must have minimum two points.";
  case 2:
    if (lin2pts == false) {
      result["WorkspaceToInterpolate"] =
          "Workspace has only 2 points, "
          "you can enable linear interpolation by "
          "setting the property Linear2Points. Otherwise "
          "provide a minimum of 3 points.";
    }
  }

  const int deriv_order = getProperty("DerivOrder");
  const std::string derivFileName = getProperty("OutputWorkspaceDeriv");
  if (derivFileName.empty() && (deriv_order > 0)) {
    result["OutputWorkspaceDeriv"] =
        "Enter a name for the OutputWorkspaceDeriv "
        "or set DerivOrder to zero.";
  }

  return result;
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void SplineInterpolation::exec() {
  // read in algorithm parameters
  int order = static_cast<int>(getProperty("DerivOrder"));

  const bool type = getProperty("Linear2Points");

  // set input workspaces
  MatrixWorkspace_sptr mws = getProperty("WorkspaceToMatch");
  MatrixWorkspace_sptr iws = getProperty("WorkspaceToInterpolate");

  // avoid x-value sorting in CubicSpline and ensure sorting if Linear2Points
  ensureXIncreasing(mws);
  ensureXIncreasing(iws);

  int histNo = static_cast<int>(iws->getNumberHistograms());
  int binsNo = static_cast<int>(iws->blocksize());

  if (type == true && binsNo == 2) {
    //m_interp_type = boost::make_shared<Linear>();
    //g_log.information() << "Linear interpolation for 2 points.\n";
  } else {
    m_interp_type = boost::make_shared<CubicSpline>();
  }

  m_interp_type->initialize();

  // vector of multiple derivative workspaces
  std::vector<MatrixWorkspace_sptr> derivs(histNo);

  // warn user that we only use first spectra in matching workspace
  if (mws->getNumberHistograms() > 1) {
    g_log.warning()
        << "Algorithm can only interpolate against a single data set. "
           "Only the first data set will be used.\n";
  }

  // convert data to binned data as required
  MatrixWorkspace_sptr mwspt = convertBinnedData(mws);
  MatrixWorkspace_const_sptr iwspt = convertBinnedData(iws);

  MatrixWorkspace_sptr outputWorkspace = setupOutputWorkspace(mws, iws);

  Progress pgress(this, 0.0, 1.0, histNo);

  // for each histogram in workspace, calculate interpolation and derivatives
  for (int i = 0; i < histNo; ++i) {

    // set the interpolation points
    setInterpolationPoints(iwspt, i);

    // compare the data set against our spline
    calculateSpline(mwspt, outputWorkspace, i);
    outputWorkspace->setSharedX(i, mws->sharedX(0));

    // check if we want derivatives
    if (order > 0) {
      derivs[i] = WorkspaceFactory::Instance().create(mws, order);
      auto vAxis = new NumericAxis(order);

      // calculate the derivatives for each order chosen
      for (int j = 0; j < order; ++j) {
        vAxis->setValue(j, j + 1);
        derivs[i]->setSharedX(j, mws->sharedX(0));
        calculateDerivatives(mwspt, derivs[i], j + 1);
      }
      derivs[i]->replaceAxis(1, vAxis);
    }

    pgress.report();
  }

  // Store the output workspaces
  if (order > 0) {
    // Store derivatives in a grouped workspace
    WorkspaceGroup_sptr wsg = WorkspaceGroup_sptr(new WorkspaceGroup);
    for (int i = 0; i < histNo; ++i) {
      setXRange(derivs[i], iws);
      wsg->addWorkspace(derivs[i]);
    }
    // set y values accorting to integreation range must be set to zero
    setProperty("OutputWorkspaceDeriv", wsg);
  }

  // set y values according to the integreation range
  setXRange(outputWorkspace, iws);
  setProperty("OutputWorkspace", outputWorkspace);
}

/**Copy the meta data for the input workspace to an output workspace and create
 *it with the desired number of spectra.
 * Also labels the axis of each spectra with Yi, where i is the index
 *
 * @param mws :: The input workspace to match
 * @param iws :: The input workspace to interpolate
 * @return The pointer to the newly created workspace
 */
MatrixWorkspace_sptr
SplineInterpolation::setupOutputWorkspace(MatrixWorkspace_sptr mws,
                                          MatrixWorkspace_sptr iws) {
  size_t numSpec = iws->getNumberHistograms();

  MatrixWorkspace_sptr outputWorkspace =
      WorkspaceFactory::Instance().create(mws, numSpec);

  // use the vertical axis from the workspace to interpolate on the output WS
  Axis *vAxis = iws->getAxis(1)->clone(iws.get());
  outputWorkspace->replaceAxis(1, vAxis);

  return outputWorkspace;
}

/**Convert a binned workspace to point data
 *
 * @param workspace :: The input workspace
 * @return the converted workspace containing point data
 */
MatrixWorkspace_sptr
SplineInterpolation::convertBinnedData(MatrixWorkspace_sptr workspace) const {
  if (workspace->isHistogramData()) {
    const size_t histNo = workspace->getNumberHistograms();
    const size_t size = workspace->y(0).size();

    // make a new workspace for the point data
    MatrixWorkspace_sptr pointWorkspace =
        WorkspaceFactory::Instance().create(workspace, histNo, size, size);

    // loop over each histogram
    for (size_t i = 0; i < histNo; ++i) {
      const auto &xValues = workspace->x(i);
      pointWorkspace->setSharedY(i, workspace->sharedY(i));

      auto &newXValues = pointWorkspace->mutableX(i);

      // set x values to be average of bin bounds
      for (size_t j = 0; j < size; ++j) {
        newXValues[j] = (xValues[j] + xValues[j + 1]) / 2;
      }
    }

    return pointWorkspace;
  }

  return workspace;
}

/** Sets the points defining the spline
 *
 * @param inputWorkspace :: The input workspace containing the points of the
 *spline
 * @param row :: The row of spectra to use
 */
void SplineInterpolation::setInterpolationPoints(
    MatrixWorkspace_const_sptr inputWorkspace, const int row) const {
  const auto &xIn = inputWorkspace->x(row);
  const auto &yIn = inputWorkspace->y(row);
  int size = static_cast<int>(xIn.size());

  // pass x attributes and y parameters to CubicSpline
  m_interp_type->setAttributeValue("n", size);

  for (int i = 0; i < size; ++i) {
    // check that setting the x attribute is within our range
    if (i < size) {
      std::string xName = "x" + std::to_string(i);
      m_interp_type->setAttributeValue(xName, xIn[i]);
    } else {
      throw std::range_error("SplineInterpolation: x index out of range.");
    }
    // Call parent setParameter implementation
    m_interp_type->ParamFunction::setParameter(i, yIn[i], true);
  }
}

/** Calculate the derivatives of the given order from the interpolated points
 *
 * @param inputWorkspace :: The input workspace
 * @param outputWorkspace :: The output workspace
 * @param order :: The order of derivatives to calculate
 */
void SplineInterpolation::calculateDerivatives(
    MatrixWorkspace_const_sptr inputWorkspace,
    MatrixWorkspace_sptr outputWorkspace, int order) const {
  // get x and y parameters from workspaces
  const size_t nData = inputWorkspace->y(0).size();
  const double *xValues = &(inputWorkspace->x(0)[0]);
  double *yValues = &(outputWorkspace->mutableY(order - 1)[0]);

  // calculate the derivatives
  m_interp_type->derivative1D(yValues, xValues, nData, order);
}

/** Calculate the interpolation of the input points against the spline
 *
 * @param inputWorkspace :: The input workspace
 * @param outputWorkspace :: The output workspace
 * @param row :: The row of spectra to use
 */
void SplineInterpolation::calculateSpline(
    MatrixWorkspace_const_sptr inputWorkspace,
    MatrixWorkspace_sptr outputWorkspace, int row) const {
  // setup input parameters
  const size_t nData = inputWorkspace->y(0).size();
  const double *xValues = &(inputWorkspace->x(0)[0]);
  double *yValues = &(outputWorkspace->mutableY(row)[0]);

  // calculate the interpolation
  m_interp_type->function1D(yValues, xValues, nData);
}

/** Check if the supplied x value falls within the interpolation range.
 * This is in particular important for the Linear function and not for
 * the CubicSpline
 *
 * @param inputWorkspace :: The input workspace
 * @param interpolationWorkspace :: The interpolation workspace
 */
void SplineInterpolation::setXRange(
    MatrixWorkspace_sptr inputWorkspace,
    MatrixWorkspace_const_sptr interpolationWorkspace) const {
  // setup input parameters
  int histNo = static_cast<int>(inputWorkspace->getNumberHistograms());
  const size_t nData = inputWorkspace->y(0).size();
  const double *xValues = &(inputWorkspace->x(0)[0]);

  const size_t nintegData = interpolationWorkspace->y(0).size();
  const double *xintegValues = &(interpolationWorkspace->x(0)[0]);

  for (int n = 0; n < histNo; ++n) {
    int nOutsideLeft = 0, nOutsideRight = 0;

    for (size_t i = 0; i < nData; ++i) {

      // determine number of values smaller than the integration range
      if (xValues[i] < xintegValues[0])
        nOutsideLeft++;
      else if (xValues[i] > xintegValues[nintegData - 1])
        nOutsideRight++;
    }
    double *yValues = &(inputWorkspace->mutableY(n)[0]);
    if (nOutsideLeft > 0) {
      std::fill_n(yValues, nOutsideLeft, yValues[nOutsideLeft]);
      g_log.warning() << nOutsideLeft << " x value(s) larger than integration "
                                         "range, will not be calculated.\n";
    }
    if (nOutsideRight > 0) {
      nOutsideRight += 1;
      g_log.warning() << nOutsideRight << " x value(s) smaller than "
                                          "integration range, will not be "
                                          "calculated.\n";
    }
    for (size_t k = nData - nOutsideRight; k < nData; ++k)
      yValues[k] = yValues[nData - nOutsideRight];
  }
}

/** Sets up the spline object by with the parameters and attributes
 *
 * @param inputWorkspace :: The input workspace being checked
 */
void SplineInterpolation::ensureXIncreasing(
    MatrixWorkspace_sptr inputWorkspace) {
  // setup input parameters
  const size_t nData = inputWorkspace->y(0).size();
  const double *xValues = &(inputWorkspace->x(0)[0]);

  for (size_t i = 1; i < nData; ++i) {
    // x values must be stricly increasing
    if (xValues[i] < xValues[i - 1]) {
      g_log.warning() << "x values must be stricly increasing. Start "
                         "sorting ...\n";
      Algorithm_sptr sorter = createChildAlgorithm("SortXAxis");
      sorter->setProperty<Workspace_sptr>("InputWorkspace", inputWorkspace);
      sorter->setProperty<Workspace_sptr>("OutputWorkspace", inputWorkspace);
      sorter->execute();
      continue;
    }
  }
}

} // namespace Algorithms
} // namespace CurveFitting
} // namespace Mantid
