#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
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

  std::vector<std::string> inputWorkspaces = {"WorkspaceToMatch",
                                              "WorkspaceToInterpolate"};
  declareProperty("ReferenceWorkspace", "WorkspaceToMatch",
                  boost::make_shared<StringListValidator>(inputWorkspaces),
                  "OutputWorkspace will copy properties from the selected "
                  "workspace properties (e.g. instrument) except its "
                  "dimensions.");
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
  size_t binsNo = iws_valid->blocksize();

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

  const std::string refWorkspace = getProperty("ReferenceWorkspace");

  // set input workspaces
  MatrixWorkspace_sptr mws = getProperty("WorkspaceToMatch");
  MatrixWorkspace_sptr iws = getProperty("WorkspaceToInterpolate");
  MatrixWorkspace_sptr refws = getProperty(refWorkspace);

  int histNo = static_cast<int>(iws->getNumberHistograms());
  size_t binsNo = static_cast<int>(iws->blocksize());
  int binsNoInterp = static_cast<int>(mws->blocksize());

  // vector of multiple derivative workspaces
  std::vector<MatrixWorkspace_sptr> derivs(histNo);

  // warn user that we only use first spectra in matching workspace
  if (mws->getNumberHistograms() > 1) {
    g_log.warning()
        << "Algorithm can only interpolate against a single data set. "
           "Only the first data set will be used.\n";
  }

  // convert data to binned data to point data as required
  MatrixWorkspace_sptr mwspt = convertBinnedData(mws);
  MatrixWorkspace_sptr iwspt = convertBinnedData(iws);

  // for point data: avoid x-value sorting in CubicSpline and ensure sorting if
  // Linear2Points
  // attention: if histogram data only x values will be sorted
  mws = ensureXIncreasing(mws);
  iws = ensureXIncreasing(iws);
  // point data
  mwspt = ensureXIncreasing(mwspt);
  iwspt = ensureXIncreasing(iwspt);

  // setup OutputWorkspace
  // eventually keep x-Values of histograms
  size_t sizeX = mws->readX(0).size();
  size_t sizeY = mwspt->readY(0).size();
  // setup output workspace
  MatrixWorkspace_sptr outputWorkspace = WorkspaceFactory::Instance().create(
      refws, iwspt->getNumberHistograms(), sizeX, sizeY);
  // get vertical axis from WorkspaceToInterpolate
  Axis *outputAxis = iws->getAxis(1)->clone(iws.get());
  outputWorkspace->replaceAxis(1, outputAxis);

  Progress pgress(this, 0.0, 1.0, histNo);

  if (type == true && binsNo == 2) {
    g_log.information() << "Linear interpolation using 2 points.\n";
  } else {
    m_cspline = boost::make_shared<CubicSpline>();
  }

  // for each histogram in workspace, calculate interpolation and derivatives
  for (int i = 0; i < histNo; ++i) {
    if (type == true && binsNo == 2) {
      // set up the function that needs to be interpolated
      std::unique_ptr<gsl_interp_accel, void (*)(gsl_interp_accel *)> acc(
          gsl_interp_accel_alloc(), gsl_interp_accel_free);
      std::unique_ptr<gsl_interp, void (*)(gsl_interp *)> linear(
          gsl_interp_alloc(gsl_interp_linear, binsNo), gsl_interp_free);
      gsl_interp_linear->init(linear.get(), &(iwspt->x(i)[0]),
                              &(iwspt->y(i)[0]), binsNo);
      for (int k = 0; k < binsNoInterp; ++k) {
        gsl_interp_linear->eval(linear.get(), &(iwspt->x(i)[0]),
                                &(iwspt->y(i)[0]), binsNo, mwspt->x(0)[k],
                                acc.get(), &(outputWorkspace->mutableY(i)[k]));
        if (order > 0) {
          auto vAxis = new NumericAxis(order);
          derivs[i] =
              WorkspaceFactory::Instance().create(refws, order, sizeX, sizeY);
          for (int j = 0; j < order; ++j) {
            vAxis->setValue(j, j + 1);
            derivs[i]->setSharedX(j, mws->sharedX(0));
            if (j == 0)
              gsl_interp_linear->eval_deriv(
                  linear.get(), &(iwspt->x(i)[0]), &(iwspt->y(i)[0]), binsNo,
                  mwspt->x(0)[k], acc.get(), &(derivs[i]->mutableY(i)[k]));
            if (j == 1)
              gsl_interp_linear->eval_deriv2(
                  linear.get(), &(iwspt->x(i)[0]), &(iwspt->y(i)[0]), binsNo,
                  mwspt->x(0)[k], acc.get(), &(derivs[i]->mutableY(i)[k]));
          }
          derivs[i]->replaceAxis(1, vAxis);
        }
      }
    } else {
      setInterpolationPoints(iwspt, i);
      // compare the data set against our spline
      calculateSpline(mwspt, outputWorkspace, i);

      // check if we want derivatives
      if (order > 0) {
        auto vAxis2 = new NumericAxis(order);
        derivs[i] =
            WorkspaceFactory::Instance().create(iwspt, order, sizeX, sizeY);

        // calculate the derivatives for each order chosen
        for (int j = 0; j < order; ++j) {
          vAxis2->setValue(j, j + 1);
          calculateDerivatives(mwspt, derivs[i], j + 1);
          derivs[i]->setSharedX(j, mws->sharedX(0));
        }
        derivs[i]->replaceAxis(1, vAxis2);
      }
    }
    outputWorkspace->setSharedX(i, mws->sharedX(0));
    pgress.report();
  }

  // store the output workspaces
  if (order > 0) {
    // Store derivatives in a grouped workspace
    WorkspaceGroup_sptr wsg = WorkspaceGroup_sptr(new WorkspaceGroup);
    for (int i = 0; i < histNo; ++i) {
      wsg->addWorkspace(derivs[i]);
    }
    // set y values accorting to integration range must be set to zero
    setProperty("OutputWorkspaceDeriv", wsg);
  }

  // set y values according to the integration range
  setXRange(outputWorkspace, iws);
  setProperty("OutputWorkspace", outputWorkspace);
}

/**Convert a binned workspace to point data
 *
 * @param workspace :: The input workspace
 * @return The converted workspace containing point data
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
  } else
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
    // check that setting the x attribute is within our range
    if (i < size) {
      std::string xName = "x" + std::to_string(i);
      m_cspline->setAttributeValue(xName, xIn[i]);
    } else {
      throw std::range_error("SplineInterpolation: x index out of range.");
    }
    // Call parent setParameter implementation
    m_cspline->ParamFunction::setParameter(i, yIn[i], true);
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

/** Check if the supplied x value falls within the interpolation range.
 * Y values larger or smaller than the interpolation range will be set
 * to the first or last y value of the WorkspaceToInterpolate.
 * Both workspaces must have the same number of spectra
 *
 * @param inputWorkspace :: The input workspace
 * @param interpolationWorkspace :: The interpolation workspace
 */
void SplineInterpolation::setXRange(
    MatrixWorkspace_sptr inputWorkspace,
    MatrixWorkspace_const_sptr interpolationWorkspace) const {
  // setup input parameters
  size_t histNo = inputWorkspace->getNumberHistograms();
  size_t histNoInterp = interpolationWorkspace->getNumberHistograms();
  const size_t nData = inputWorkspace->y(0).size();
  const double *xValues = &(inputWorkspace->x(0)[0]);
  const size_t nintegData = interpolationWorkspace->y(0).size();
  const double *xintegValues = &(interpolationWorkspace->x(0)[0]);

  if (histNo == histNoInterp) {
    for (size_t n = 0; n < histNo; ++n) {
      int nOutsideLeft = 0, nOutsideRight = 0;
      for (size_t i = 0; i < nData; ++i) {
        // determine number of values outside of the integration range
        if (xValues[i] < xintegValues[0])
          nOutsideLeft++;
        else if (xValues[i] > xintegValues[nintegData - 1])
          nOutsideRight++;
      }
      const auto &yRef = interpolationWorkspace->y(n);
      if (nOutsideLeft > 0) {
        double *yValues = &(inputWorkspace->mutableY(n)[0]);
        std::fill_n(yValues, nOutsideLeft, yRef[0]);
        g_log.warning() << "Workspace index " << n << ": " << nOutsideLeft
                        << " x value(s) smaller than integration "
                           "range, will not be calculated.\n";
      }
      if (nOutsideRight > 0) {
        double *yValuesEnd =
            &(inputWorkspace->mutableY(n)[nData - nOutsideRight]);
        std::fill_n(yValuesEnd, nOutsideRight, yRef[nintegData - 1]);
        g_log.warning() << "Workspace index " << n << ": " << nOutsideRight
                        << " x value(s) larger than "
                           "integration range, will not be "
                           "calculated.\n";
      }
    }
  }
}

/** Sets up the spline object by with the parameters and attributes
 *
 * @param inputWorkspace :: The input workspace being checked
 * @return outputWorkspace :: The sorted output workspace
 */
MatrixWorkspace_sptr
SplineInterpolation::ensureXIncreasing(MatrixWorkspace_sptr inputWorkspace) {
  // this part can be deleted if SortXAxis checks if the workspace is already
  // sorted
  const size_t nData = inputWorkspace->y(0).size();
  const double *xValues = &(inputWorkspace->x(0)[0]);
  int sortFlag = 0;
  for (size_t i = 1; i < nData; ++i) {
    if (xValues[i] < xValues[i - 1]) {
      g_log.warning() << "x values are not stricly increasing.\n";
      sortFlag = 1;
      break;
    }
  }
  if (sortFlag == 1 && (inputWorkspace->isHistogramData() == 0)) {
    g_log.warning() << "Start sorting " << inputWorkspace->getName() << "\n";
    Algorithm_sptr sorter = createChildAlgorithm("SortXAxis");
    sorter->initialize();
    sorter->setProperty<Workspace_sptr>("InputWorkspace", inputWorkspace);
    sorter->execute();
    return sorter->getProperty("OutputWorkspace");
  } else {
    return inputWorkspace;
  }
}

} // namespace Algorithms
} // namespace CurveFitting
} // namespace Mantid
