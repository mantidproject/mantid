#include "MantidAPI/TextAxis.h"
#include "MantidAPI/Progress.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidCurveFitting/SplineInterpolation.h"

namespace Mantid {
namespace CurveFitting {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SplineInterpolation)

using namespace API;
using namespace Kernel;

//----------------------------------------------------------------------------------------------
/** Constructor
 */
SplineInterpolation::SplineInterpolation()
    : m_cspline(boost::make_shared<CubicSpline>()) {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
SplineInterpolation::~SplineInterpolation() {}

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
  declareProperty(
      new WorkspaceProperty<>("WorkspaceToMatch", "", Direction::Input),
      "The workspace which defines the points of the spline.");

  declareProperty(
      new WorkspaceProperty<>("WorkspaceToInterpolate", "", Direction::Input),
      "The workspace on which to perform the interpolation algorithm.");

  declareProperty(
      new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output),
      "The workspace containing the calculated points and derivatives");

  declareProperty(new WorkspaceProperty<WorkspaceGroup>("OutputWorkspaceDeriv",
                                                        "", Direction::Output,
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
           "Only the first data set will be used." << std::endl;
  }

  // convert data to binned data as required
  MatrixWorkspace_sptr mwspt = convertBinnedData(mws);
  MatrixWorkspace_const_sptr iwspt = convertBinnedData(iws);

  MatrixWorkspace_sptr outputWorkspace = setupOutputWorkspace(mws, histNo);

  Progress pgress(this, 0.0, 1.0, histNo);

  // for each histogram in workspace, calculate interpolation and derivatives
  for (int i = 0; i < histNo; ++i) {
    // Create and instance of the cubic spline function
    m_cspline = boost::make_shared<CubicSpline>();
    // set the interpolation points
    setInterpolationPoints(iwspt, i);

    // compare the data set against our spline
    calculateSpline(mwspt, outputWorkspace, i);
    outputWorkspace->setX(i, mws->readX(0));

    // check if we want derivatives
    if (order > 0) {
      // calculate the derivatives for each order chosen
      derivs[i] = setupOutputWorkspace(mws, order);
      for (int j = 0; j < order; ++j) {
        derivs[i]->setX(j, mws->readX(0));
        calculateDerivatives(mwspt, derivs[i], j + 1);
      }
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
 * @param inws :: The input workspace
 * @param size :: The number of spectra the workspace should be created with
 * @return The pointer to the newly created workspace
 */
API::MatrixWorkspace_sptr
SplineInterpolation::setupOutputWorkspace(API::MatrixWorkspace_sptr inws,
                                          int size) const {
  MatrixWorkspace_sptr outputWorkspace =
      WorkspaceFactory::Instance().create(inws, size);

  // create labels for output workspace
  API::TextAxis *tAxis = new API::TextAxis(size);
  for (int i = 0; i < size; ++i) {
    std::string index = boost::lexical_cast<std::string>(i);
    tAxis->setLabel(i, "Y" + index);
  }
  outputWorkspace->replaceAxis(1, tAxis);

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
    size_t histNo = workspace->getNumberHistograms();
    size_t size = workspace->readY(0).size();

    // make a new workspace for the point data
    MatrixWorkspace_sptr pointWorkspace =
        WorkspaceFactory::Instance().create(workspace, histNo, size, size);

    // loop over each histogram
    for (size_t i = 0; i < histNo; ++i) {
      const auto &xValues = workspace->readX(i);
      const auto &yValues = workspace->readY(i);

      auto &newXValues = pointWorkspace->dataX(i);
      auto &newYValues = pointWorkspace->dataY(i);

      // set x values to be average of bin bounds
      for (size_t j = 0; j < size; ++j) {
        newXValues[j] = (xValues[j] + xValues[j + 1]) / 2;
        newYValues[j] = yValues[j];
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
  const auto &xIn = inputWorkspace->readX(row);
  const auto &yIn = inputWorkspace->readY(row);
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
  size_t nData = inputWorkspace->readY(0).size();
  const double *xValues = inputWorkspace->readX(0).data();
  double *yValues = outputWorkspace->dataY(order - 1).data();

  // calculate the derivatives
  m_cspline->derivative1D(yValues, xValues, nData, order);
}

/** Calculate the interpolation of the input points against the spline
 *
 * @param inputWorkspace :: The input workspace
 * @param outputWorkspace :: The output workspace
 * @param row :: The row of spectra to use
 */
void
SplineInterpolation::calculateSpline(MatrixWorkspace_const_sptr inputWorkspace,
                                     MatrixWorkspace_sptr outputWorkspace,
                                     int row) const {
  // setup input parameters
  size_t nData = inputWorkspace->readY(0).size();
  const double *xValues = inputWorkspace->readX(0).data();
  double *yValues = outputWorkspace->dataY(row).data();

  // calculate the interpolation
  m_cspline->function1D(yValues, xValues, nData);
}

} // namespace CurveFitting
} // namespace Mantid
