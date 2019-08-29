// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCurveFitting/Algorithms/SplineInterpolation.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/BoundedValidator.h"

#include <algorithm>
#include <stdexcept>

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

/// Algorithm's summary description. @see Algorithm::summary
const std::string SplineInterpolation::summary() const {
  return "Interpolates a set of spectra onto a spline defined by a second "
         "input workspace. Optionally, this algorithm can also calculate "
         "derivatives up to order 2 as a side product";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void SplineInterpolation::init() {
  declareProperty(std::make_unique<WorkspaceProperty<>>("WorkspaceToMatch", "",
                                                        Direction::Input),
                  "The workspace which defines the points of the spline.");

  declareProperty(
      std::make_unique<WorkspaceProperty<>>("WorkspaceToInterpolate", "",
                                            Direction::Input),
      "The workspace on which to perform the interpolation algorithm.");

  declareProperty(
      std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                            Direction::Output),
      "The workspace containing the calculated points and derivatives");

  declareProperty(std::make_unique<WorkspaceProperty<WorkspaceGroup>>(
                      "OutputWorkspaceDeriv", "", Direction::Output,
                      PropertyMode::Optional),
                  "The workspace containing the calculated derivatives");

  auto validator = boost::make_shared<BoundedValidator<int>>(0, 2);
  declareProperty("DerivOrder", 2, validator,
                  "Order to derivatives to calculate.");

  declareProperty("Linear2Points", false,
                  "Set to true to perform linear "
                  "interpolation if only 2 points are "
                  "present.");
}

//----------------------------------------------------------------------------------------------
/** Input validation for the WorkspaceToInterpolate
 */
std::map<std::string, std::string> SplineInterpolation::validateInputs() {
  // initialise map (result)
  std::map<std::string, std::string> result;

  // get inputs that need validation
  const bool linear = getProperty("Linear2Points");
  const int derivOrder = getProperty("DerivOrder");

  MatrixWorkspace_const_sptr iwsValid = getProperty("WorkspaceToInterpolate");
  if (iwsValid) {
    try {
      const size_t binsNo = iwsValid->blocksize();

      // The minimum number of points for cubic splines is 3
      if (binsNo < 2) {
        result["WorkspaceToInterpolate"] =
            "Workspace must have minimum 2 points.";
      } else if (binsNo == 2) {
        if (!linear) {
          result["WorkspaceToInterpolate"] =
              "Workspace has only 2 points, "
              "you can enable linear interpolation by "
              "setting the property Linear2Points. Otherwise "
              "provide a minimum of 3 points.";
        } else if (derivOrder == 2) {
          result["DerivOrder"] = "Linear interpolation is requested, hence "
                                 "derivative order can be maximum 1.";
        }
      }
    } catch (std::length_error &) {
      result["WorkspaceToInterpolate"] = "The input workspace does not have "
                                         "the same number of bins per spectrum";
    }
  } else {
    result["WorkspaceToInterpolate"] = "The input is not a MatrixWorkspace";
  }

  return result;
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void SplineInterpolation::exec() {
  // read in algorithm parameters
  const int derivOrder = getProperty("DerivOrder");
  const auto order = static_cast<size_t>(derivOrder);

  // set input workspaces
  MatrixWorkspace_sptr mws = getProperty("WorkspaceToMatch");
  MatrixWorkspace_sptr iws = getProperty("WorkspaceToInterpolate");

  const size_t histNo = iws->getNumberHistograms();
  const size_t binsNo = iws->blocksize();
  const size_t histNoToMatch = mws->getNumberHistograms();

  // vector of multiple derivative workspaces
  std::vector<MatrixWorkspace_sptr> derivs(histNo);

  // warn user that we only use first spectra in matching workspace
  if (histNoToMatch > 1) {
    g_log.warning()
        << "Algorithm can only interpolate against a single data set. "
           "Only the x-axis of the first spectrum will be used.\n";
  }

  MatrixWorkspace_sptr outputWorkspace = setupOutputWorkspace(mws, iws);

  Progress pgress(this, 0.0, 1.0, histNo);

  // first prepare the derivatives if needed
  if (order > 0) {
    for (size_t i = 0; i < histNo; ++i) {
      derivs[i] = WorkspaceFactory::Instance().create(mws, order);
      auto vAxis = std::make_unique<NumericAxis>(order);
      for (size_t j = 0; j < order; ++j) {
        vAxis->setValue(j, static_cast<int>(j) + 1.);
        derivs[i]->setSharedX(j, mws->sharedX(0));
      }
      derivs[i]->replaceAxis(1, std::move(vAxis));
    }
  }

  // convert data to binned data if initially histogrammed, do not alter the
  // original inputs; these should be used in subsequent processing, however,
  // note that the parent of the output workspace is still mws, and not mwspt!
  // meaning that it can be either histogram or point data dependent on the mws
  MatrixWorkspace_sptr mwspt = convertBinnedData(mws);
  MatrixWorkspace_const_sptr iwspt = convertBinnedData(iws);

  if (binsNo > 2) {
    // perform cubic spline interpolation
    // for each histogram in workspace, calculate interpolation and derivatives
    for (size_t i = 0; i < histNo; ++i) {
      // Create and instance of the cubic spline function
      m_cspline = boost::make_shared<CubicSpline>();
      // set the interpolation points
      setInterpolationPoints(iwspt, i);
      // compare the data set against our spline
      calculateSpline(mwspt, outputWorkspace, i);
      // calculate derivatives for each order
      for (size_t j = 0; j < order; ++j) {
        calculateDerivatives(mwspt, derivs[i], j + 1);
      }
      pgress.report();
    }
  } else {
    // perform linear interpolation

    // first check that the x-axis (first spectrum) is sorted ascending
    if (!std::is_sorted(mwspt->x(0).rawData().begin(),
                        mwspt->x(0).rawData().end())) {
      throw std::runtime_error(
          "X-axis of the workspace to match is not sorted. "
          "Consider calling SortXAxis before.");
    }

    for (size_t i = 0; i < histNo; ++i) {
      // set up the function that needs to be interpolated
      std::unique_ptr<gsl_interp_accel, void (*)(gsl_interp_accel *)> acc(
          gsl_interp_accel_alloc(), gsl_interp_accel_free);
      std::unique_ptr<gsl_interp, void (*)(gsl_interp *)> linear(
          gsl_interp_alloc(gsl_interp_linear, binsNo), gsl_interp_free);
      gsl_interp_linear->init(linear.get(), &(iwspt->x(i)[0]),
                              &(iwspt->y(i)[0]), binsNo);

      // figure out the interpolation range
      const std::pair<size_t, size_t> range =
          findInterpolationRange(iwspt, mwspt, i);

      // perform interpolation in the range
      for (size_t k = range.first; k < range.second; ++k) {
        gsl_interp_linear->eval(linear.get(), &(iwspt->x(i)[0]),
                                &(iwspt->y(i)[0]), binsNo, mwspt->x(0)[k],
                                acc.get(), &(outputWorkspace->mutableY(i)[k]));
        // calculate only 1st order derivative if needed
        if (order > 0) {
          gsl_interp_linear->eval_deriv(
              linear.get(), &(iwspt->x(i)[0]), &(iwspt->y(i)[0]), binsNo,
              mwspt->x(0)[k], acc.get(), &(derivs[i]->mutableY(0)[k]));
        }
      }
      // flat extrapolation outside the range
      extrapolateFlat(outputWorkspace, iwspt, i, range, order > 0, derivs);
      pgress.report();
    }
  }
  // Store the output workspaces
  if (order > 0 && !isDefault("OutputWorkspaceDeriv")) {
    // Store derivatives in a grouped workspace
    WorkspaceGroup_sptr wsg = WorkspaceGroup_sptr(new WorkspaceGroup);
    for (size_t i = 0; i < histNo; ++i) {
      wsg->addWorkspace(derivs[i]);
    }
    setProperty("OutputWorkspaceDeriv", wsg);
  }
  setProperty("OutputWorkspace", outputWorkspace);
}

/** Copy the meta data for the input workspace to an output workspace and create
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
  const size_t numSpec = iws->getNumberHistograms();
  MatrixWorkspace_sptr outputWorkspace =
      WorkspaceFactory::Instance().create(mws, numSpec);
  // share x-axes with the first spectrum of mws
  for (size_t i = 0; i < numSpec; ++i) {
    outputWorkspace->setSharedX(i, mws->sharedX(0));
  }
  // use the vertical (spectrum) axis form the iws
  auto vAxis = std::unique_ptr<Axis>(iws->getAxis(1)->clone(mws.get()));
  outputWorkspace->replaceAxis(1, std::move(vAxis));
  return outputWorkspace;
}

/** Convert a binned workspace to point data
 *
 * @param workspace :: The input workspace
 * @return The converted workspace containing point data
 */
MatrixWorkspace_sptr
SplineInterpolation::convertBinnedData(MatrixWorkspace_sptr workspace) {
  if (workspace->isHistogramData()) {
    g_log.warning("Histogram data provided, converting to point data");
    Algorithm_sptr converter = createChildAlgorithm("ConvertToPointData");
    converter->initialize();
    converter->setProperty("InputWorkspace", workspace);
    converter->execute();
    return converter->getProperty("OutputWorkspace");
  } else {
    return workspace;
  }
}

/** Sets the points defining the spline
 *
 * @param inputWorkspace :: The input workspace containing the points of the
 *spline
 * @param row :: The row of spectra to use
 */
void SplineInterpolation::setInterpolationPoints(
    MatrixWorkspace_const_sptr inputWorkspace, const size_t row) const {
  const auto &xIn = inputWorkspace->x(row);
  const auto &yIn = inputWorkspace->y(row);
  const size_t size = xIn.size();

  // pass x attributes and y parameters to CubicSpline
  m_cspline->setAttributeValue("n", static_cast<int>(size));

  for (size_t i = 0; i < size; ++i) {
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
    API::MatrixWorkspace_sptr outputWorkspace, const size_t order) const {
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
    MatrixWorkspace_sptr outputWorkspace, const size_t row) const {
  // setup input parameters
  const size_t nData = inputWorkspace->y(0).size();
  const double *xValues = &(inputWorkspace->x(0)[0]);
  double *yValues = &(outputWorkspace->mutableY(row)[0]);

  // calculate the interpolation
  m_cspline->function1D(yValues, xValues, nData);
}

/** Extrapolates flat for the points outside the x-range
 * This is used for linear case only, to be consistent with cubic spline case
 * @param ows : output workspace
 * @param iwspt : workspace to interpolate
 * @param row : the workspace index
 * @param indices : the pair of x-axis indices defining the extrapolation range
 * @param doDerivs : whether derivatives are requested
 * @param derivs : the vector of derivative workspaces
 */
void SplineInterpolation::extrapolateFlat(
    MatrixWorkspace_sptr ows, MatrixWorkspace_const_sptr iwspt,
    const size_t row, const std::pair<size_t, size_t> &indices,
    const bool doDerivs, std::vector<MatrixWorkspace_sptr> &derivs) const {

  const double yFirst = iwspt->y(row).front();
  const double yLast = iwspt->y(row).back();
  for (size_t bin = 0; bin < indices.first; ++bin) {
    ows->mutableY(row)[bin] = yFirst;
    if (doDerivs) {
      // if derivatives are requested
      derivs[row]->mutableY(0)[bin] = 0.;
    }
  }
  const size_t numBins = ows->blocksize();
  for (size_t bin = indices.second; bin < numBins; ++bin) {
    ows->mutableY(row)[bin] = yLast;
    if (doDerivs) {
      // if derivatives are requested
      derivs[row]->mutableY(0)[bin] = 0.;
    }
  }
}

/** Find the region that has to be interpolated
 * E.g. iwspt x-axis is from 50-100, while mwspt x-axis is 0-200, this will
 * return
 * the pair of the indices of mwspt, that are just above 50, and just below 100
 * This is used for linear case only, to be consistent with cubic spline case
 * @param iwspt : workspace to interpolate
 * @param mwspt : workspace to match
 * @param row : the workspace index
 * @return : pair of indices for representing the interpolation range
 */
std::pair<size_t, size_t>
SplineInterpolation::findInterpolationRange(MatrixWorkspace_const_sptr iwspt,
                                            MatrixWorkspace_sptr mwspt,
                                            const size_t row) {

  auto xAxisIn = iwspt->x(row).rawData();
  std::sort(xAxisIn.begin(), xAxisIn.end());
  const auto &xAxisOut = mwspt->x(0).rawData();

  size_t firstIndex = 0;
  size_t lastIndex = xAxisOut.size();

  if (xAxisOut.front() >= xAxisIn.back()) {
    lastIndex = firstIndex;
  } else if (xAxisOut.back() <= xAxisIn.front()) {
    firstIndex = lastIndex;
  } else {
    for (size_t i = 0; i < xAxisOut.size(); ++i) {
      if (xAxisOut[i] > xAxisIn.front()) {
        firstIndex = i;
        break;
      }
    }
    for (size_t i = 0; i < xAxisOut.size(); ++i) {
      if (xAxisOut[i] > xAxisIn.back()) {
        lastIndex = i;
        break;
      }
    }
  }

  std::string log = "Workspace index " + std::to_string(row) +
                    ": Will perform flat extrapolation outside bin range: " +
                    std::to_string(firstIndex) + " to " +
                    std::to_string(lastIndex) + "\n";

  g_log.debug(log);

  return std::make_pair(firstIndex, lastIndex);
}
} // namespace Algorithms
} // namespace CurveFitting
} // namespace Mantid
