// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCurveFitting/Algorithms/SplineSmoothing.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/BoundedValidator.h"

#include <algorithm>

namespace Mantid {
namespace CurveFitting {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SplineSmoothing)

using namespace API;
using namespace Kernel;
using Functions::BSpline;
//----------------------------------------------------------------------------------------------
/** Constructor
 */
SplineSmoothing::SplineSmoothing()
    : M_START_SMOOTH_POINTS(10), m_cspline(boost::make_shared<BSpline>()),
      m_inputWorkspace(), m_inputWorkspacePointData(),
      m_derivativeWorkspaceGroup(new WorkspaceGroup) {}

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string SplineSmoothing::name() const { return "SplineSmoothing"; }

/// Algorithm's version for identification. @see Algorithm::version
int SplineSmoothing::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string SplineSmoothing::category() const {
  return "Optimization;CorrectionFunctions\\BackgroundCorrections";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void SplineSmoothing::init() {
  declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "The workspace on which to perform the smoothing algorithm.");

  declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "The workspace containing the calculated points");

  declareProperty(make_unique<WorkspaceProperty<WorkspaceGroup>>(
                      "OutputWorkspaceDeriv", "", Direction::Output,
                      PropertyMode::Optional),
                  "The workspace containing the calculated derivatives");

  auto validator = boost::make_shared<BoundedValidator<int>>();
  validator->setLower(0);
  validator->setUpper(2);
  declareProperty("DerivOrder", 0, validator,
                  "Order to derivatives to calculate.");

  auto errorSizeValidator = boost::make_shared<BoundedValidator<double>>();
  errorSizeValidator->setLower(0.0);
  declareProperty("Error", 0.05, errorSizeValidator,
                  "The amount of error we wish to tolerate in smoothing");

  auto numOfBreaks = boost::make_shared<BoundedValidator<int>>();
  numOfBreaks->setLower(0);
  declareProperty("MaxNumberOfBreaks", 0, numOfBreaks,
                  "To set the positions of the break-points, default 0 "
                  "equally spaced real values in interval 0.0 - 1.0");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void SplineSmoothing::exec() {
  m_inputWorkspace = getProperty("InputWorkspace");

  int histNo = static_cast<int>(m_inputWorkspace->getNumberHistograms());
  int order = static_cast<int>(getProperty("DerivOrder"));

  m_inputWorkspacePointData = convertBinnedData(m_inputWorkspace);
  m_outputWorkspace = setupOutputWorkspace(m_inputWorkspacePointData, histNo);

  if (order > 0 && getPropertyValue("OutputWorkspaceDeriv").empty()) {
    throw std::runtime_error(
        "You must specify an output workspace for the spline derivatives.");
  }

  Progress pgress(this, 0.0, 1.0, histNo);
  for (int i = 0; i < histNo; ++i) {
    smoothSpectrum(i);
    calculateSpectrumDerivatives(i, order);
    pgress.report();
  }

  if (m_inputWorkspace->isHistogramData()) {
    convertToHistogram();
  }

  setProperty("OutputWorkspace", m_outputWorkspace);

  if (m_derivativeWorkspaceGroup->size() > 0) {
    setProperty("OutputWorkspaceDeriv", m_derivativeWorkspaceGroup);
  }
}

/** Smooth a single spectrum of the input workspace
 *
 * @param index :: index of the spectrum to smooth
 */
void SplineSmoothing::smoothSpectrum(const int index) {
  m_cspline = boost::make_shared<BSpline>();
  m_cspline->setAttributeValue("Uniform", false);

  // choose some smoothing points from input workspace
  selectSmoothingPoints(*m_inputWorkspacePointData, index);
  performAdditionalFitting(m_inputWorkspacePointData, index);

  m_outputWorkspace->setPoints(index, m_inputWorkspace->points(index));

  calculateSmoothing(*m_inputWorkspacePointData, *m_outputWorkspace, index);
}

/** Calculate the derivatives for each spectrum in the input workspace
 *
 * @param index :: index of the spectrum
 * @param order :: order of derivatives to calculate
 */
void SplineSmoothing::calculateSpectrumDerivatives(const int index,
                                                   const int order) {
  if (order > 0) {
    API::MatrixWorkspace_sptr derivs =
        setupOutputWorkspace(m_inputWorkspace, order);

    for (int j = 0; j < order; ++j) {
      derivs->setSharedX(j, m_inputWorkspace->sharedX(index));
      calculateDerivatives(*m_inputWorkspacePointData, *derivs, j + 1, index);
    }

    m_derivativeWorkspaceGroup->addWorkspace(derivs);
  }
}

/** Use a child fitting algorithm to tidy the smoothing
 *
 * @param ws :: The input workspace
 * @param row :: The row of spectra to use
 */
void SplineSmoothing::performAdditionalFitting(MatrixWorkspace_sptr ws,
                                               const int row) {
  // perform additional fitting of the points
  auto fit = createChildAlgorithm("Fit");
  fit->setProperty("Function",
                   boost::dynamic_pointer_cast<IFunction>(m_cspline));
  fit->setProperty("InputWorkspace", ws);
  fit->setProperty("MaxIterations", 5);
  fit->setProperty("WorkspaceIndex", row);
  fit->execute();
}

/**Copy the meta data for the input workspace to an output workspace and create
 *it with the desired number of spectra.
 * Also labels the axis of each spectra with Yi, where i is the index
 *
 * @param inws :: The input workspace as a shared pointer
 * @param size :: The number of spectra the workspace should be created with
 * @return The pointer to the newly created workspace
 */
API::MatrixWorkspace_sptr
SplineSmoothing::setupOutputWorkspace(const MatrixWorkspace_sptr &inws,
                                      const int size) const {
  // Must pass a shared pointer instead of a reference as the
  // workspace factory will not accept raw pointers.
  MatrixWorkspace_sptr outputWorkspace =
      WorkspaceFactory::Instance().create(inws, size);

  // create labels for output workspace
  auto tAxis = new API::TextAxis(size);
  for (int i = 0; i < size; ++i) {
    const std::string index = std::to_string(i);
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
SplineSmoothing::convertBinnedData(MatrixWorkspace_sptr workspace) {
  if (workspace->isHistogramData()) {
    auto alg = createChildAlgorithm("ConvertToPointData");
    alg->setProperty("InputWorkspace", workspace);
    alg->execute();
    return alg->getProperty("OutputWorkspace");
  } else {
    return workspace;
  }
}

/**
 * Converts the output workspace back to histogram data if it was
 * converted to point data previously
 */
void SplineSmoothing::convertToHistogram() {
  auto alg = createChildAlgorithm("ConvertToHistogram");
  alg->setProperty("InputWorkspace", m_outputWorkspace);
  alg->execute();
  m_outputWorkspace = alg->getProperty("OutputWorkspace");
}

/** Calculate smoothing of the data using the spline
 * Wraps CubicSpline function1D
 *
 * @param inputWorkspace :: The input workspace
 * @param outputWorkspace :: The output workspace
 * @param row :: The row of spectra to use
 */
void SplineSmoothing::calculateSmoothing(const MatrixWorkspace &inputWorkspace,
                                         MatrixWorkspace &outputWorkspace,
                                         size_t row) const {
  // define the spline's parameters
  const auto &xIn = inputWorkspace.x(row);
  const size_t nData = xIn.size();
  const double *xValues = &(xIn[0]);
  double *yValues = &(outputWorkspace.mutableY(row)[0]);

  // calculate the smoothing
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
    const MatrixWorkspace &inputWorkspace,
    API::MatrixWorkspace &outputWorkspace, const int order,
    const size_t row) const {
  const auto &xIn = inputWorkspace.x(row);
  const double *xValues = &(xIn[0]);
  double *yValues = &(outputWorkspace.mutableY(order - 1)[0]);
  const size_t nData = xIn.size();

  m_cspline->derivative1D(yValues, xValues, nData, order);
}

/** Checks if the difference of each data point between the smoothing points
 * falls within the error tolerance.
 *
 * @param start :: The index of start checking accuracy at
 * @param end :: The index to stop checking accuracy at
 * @param ys :: The y data points from the noisy data
 * @param ysmooth :: The corresponding y data points defined by the spline
 * @return Boolean meaning if the values were accurate enough
 */
bool SplineSmoothing::checkSmoothingAccuracy(const int start, const int end,
                                             const double *ys,
                                             const double *ysmooth) const {
  double error = getProperty("Error");

  // for all values between the selected indices
  for (int i = start; i < end; ++i) {
    // check if the difference between points is greater than our error
    // tolerance
    double ydiff = fabs(ys[i] - ysmooth[i]);
    if (ydiff > error && (end - start) > 1) {
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
void SplineSmoothing::addSmoothingPoints(const std::set<int> &points,
                                         const double *xs,
                                         const double *ys) const {
  // resize the number of attributes
  int num_points = static_cast<int>(points.size());
  std::vector<double> breakPoints;
  breakPoints.reserve(num_points);
  // set each of the x and y points to redefine the spline
  for (auto const &point : points) {
    breakPoints.emplace_back(xs[point]);
  }
  m_cspline->setAttribute("BreakPoints",
                          API::IFunction::Attribute(breakPoints));

  int i = 0;
  for (auto const &point : points) {
    m_cspline->setParameter(i, ys[point]);
    ++i;
  }
}

/** Defines the points used to make the spline by iteratively creating more
 *smoothing points
 * until all smoothing points fall within a certain error tolerance
 *
 * @param inputWorkspace :: The input workspace containing noisy data
 * @param row :: The row of spectra to use
 */
void SplineSmoothing::selectSmoothingPoints(
    const MatrixWorkspace &inputWorkspace, const size_t row) {
  std::set<int> smoothPts;
  const auto &xs = inputWorkspace.x(row);
  const auto &ys = inputWorkspace.y(row);

  // retrieving number of breaks
  int maxBreaks = static_cast<int>(getProperty("MaxNumberOfBreaks"));

  int xSize = static_cast<int>(xs.size());

  // evenly space initial points over data set
  int delta;

  // if retrieved value is default zero/default
  bool incBreaks = false;

  if (maxBreaks != 0) {
    // number of points to start with
    int numSmoothPts(maxBreaks);
    delta = xSize / numSmoothPts;
    // include maxBreaks when != 0
    incBreaks = true;
  } else {
    int numSmoothPts(M_START_SMOOTH_POINTS);
    delta = xSize / numSmoothPts;
  }

  for (int i = 0; i < xSize; i += delta) {
    smoothPts.insert(i);
  }
  smoothPts.insert(xSize - 1);

  bool resmooth(true);
  while (resmooth) {

    if (incBreaks) {
      if (smoothPts.size() > static_cast<unsigned>(maxBreaks + 2)) {
        break;
      }

    } else if (!incBreaks) {
      if (smoothPts.size() >= xs.size() - 1) {
        break;
      }
    }

    addSmoothingPoints(smoothPts, &xs[0], &ys[0]);
    resmooth = false;

    // calculate the spline and retrieve smoothed points
    boost::shared_array<double> ysmooth(new double[xSize]);
    m_cspline->function1D(ysmooth.get(), &xs[0], xSize);

    // iterate over smoothing points
    auto iter = smoothPts.cbegin();
    int start = *iter;

    for (++iter; iter != smoothPts.cend(); ++iter) {
      int end = *iter;

      // check each point falls within our range of error.
      bool accurate = checkSmoothingAccuracy(start, end, &ys[0], ysmooth.get());

      // if not, flag for resmoothing and add another point between these two
      // data points
      if (!accurate) {
        resmooth = true;
        smoothPts.insert((start + end) / 2);
      }

      start = end;
    }
  }
}

} // namespace Algorithms
} // namespace CurveFitting
} // namespace Mantid
