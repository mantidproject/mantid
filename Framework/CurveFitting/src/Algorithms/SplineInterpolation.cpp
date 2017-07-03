#include "MantidAPI/TextAxis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidCurveFitting/Algorithms/SplineInterpolation.h"

namespace Mantid {
namespace CurveFitting {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SplineInterpolation)

using namespace API;
using namespace Kernel;
using Functions::CubicSpline;

//----------------------------------------------------------------------------------------------
/** Constructor
 */
SplineInterpolation::SplineInterpolation()
    : m_cspline(boost::make_shared<CubicSpline>()) {}

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
  declareProperty("DerivOrder", 2, validator,
                  "Order to derivatives to calculate.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void SplineInterpolation::exec() {
  // read in algorithm parameters
  int order = static_cast<int>(getProperty("DerivOrder"));

  // set input workspaces
  MatrixWorkspace_sptr mws = getProperty("WorkspaceToMatch");
  MatrixWorkspace_sptr iws = getProperty("WorkspaceToInterpolate");

  int histNo = static_cast<int>(iws->getNumberHistograms());

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
    // Create and instance of the cubic spline function
    m_cspline = boost::make_shared<CubicSpline>();
    // set the interpolation points
    setInterpolationPoints(iwspt, i);

    // compare the data set against our spline
    calculateSpline(mwspt, outputWorkspace, i);
    outputWorkspace->setSharedX(i, mws->sharedX(0));

    // check if we want derivatives
    if (order > 0) {
      derivs[i] = WorkspaceFactory::Instance().create(mws, order);
      auto vAxis = new API::NumericAxis(order);

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
  std::string derivWsName = getPropertyValue("OutputWorkspaceDeriv");
  if (order > 0 && derivWsName != "") {
    // Store derivatives in a grouped workspace
    WorkspaceGroup_sptr wsg = WorkspaceGroup_sptr(new WorkspaceGroup);
    for (int i = 0; i < histNo; ++i) {
      wsg->addWorkspace(derivs[i]);
    }
    setProperty("OutputWorkspaceDeriv", wsg);
  }

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
API::MatrixWorkspace_sptr
SplineInterpolation::setupOutputWorkspace(API::MatrixWorkspace_sptr mws,
                                          API::MatrixWorkspace_sptr iws) const {
  size_t numSpec = iws->getNumberHistograms();
  MatrixWorkspace_sptr outputWorkspace =
      WorkspaceFactory::Instance().create(mws, numSpec);

  // Use the vertical axis form the workspace to interpolate on the output WS
  Axis *vAxis = iws->getAxis(1)->clone(mws.get());
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
  m_cspline->setAttributeValue("n", size);

  for (int i = 0; i < size; ++i) {
    m_cspline->setXAttribute(i, xIn[i]);
    m_cspline->setParameter(i, yIn[i]);
  }
}

/** Calculate the derivatives of the given order from the interpolated points
 *
 * @param inputWorkspace :: The input workspace
 * @param outputWorkspace :: The output workspace
 * @param order :: The order of derivatives to calculate
 */
void SplineInterpolation::calculateDerivatives(
    API::MatrixWorkspace_const_sptr inputWorkspace,
    API::MatrixWorkspace_sptr outputWorkspace, int order) const {
  // get x and y parameters from workspaces
  const size_t nData = inputWorkspace->y(0).size();
  const double *xValues = &(inputWorkspace->x(0)[0]);
  double *yValues = &(outputWorkspace->mutableY(order - 1)[0]);

  // calculate the derivatives
  m_cspline->derivative1D(yValues, xValues, nData, order);
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
  m_cspline->function1D(yValues, xValues, nData);
}

} // namespace Algorithms
} // namespace CurveFitting
} // namespace Mantid
