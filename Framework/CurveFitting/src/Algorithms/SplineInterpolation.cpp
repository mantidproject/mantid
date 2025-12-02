// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCurveFitting/Algorithms/SplineInterpolation.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/Spline.h"

#include <algorithm>
#include <stdexcept>

namespace Mantid::CurveFitting::Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SplineInterpolation)

using namespace API;
using namespace Kernel;

//----------------------------------------------------------------------------------------------
/** Constructor
 */
SplineInterpolation::SplineInterpolation() {}

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string SplineInterpolation::name() const { return "SplineInterpolation"; }

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
  declareProperty(std::make_unique<WorkspaceProperty<>>("WorkspaceToMatch", "", Direction::Input),
                  "The workspace which defines the points of the spline.");

  declareProperty(std::make_unique<WorkspaceProperty<>>("WorkspaceToInterpolate", "", Direction::Input),
                  "The workspace on which to perform the interpolation algorithm.");

  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "", Direction::Output),
                  "The workspace containing the calculated points and derivatives");

  declareProperty(std::make_unique<WorkspaceProperty<WorkspaceGroup>>("OutputWorkspaceDeriv", "", Direction::Output,
                                                                      PropertyMode::Optional),
                  "The workspace containing the calculated derivatives");

  auto validator = std::make_shared<BoundedValidator<int>>(0, 2);
  declareProperty("DerivOrder", 2, validator, "Order to derivatives to calculate.");

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
        result["WorkspaceToInterpolate"] = "Workspace must have minimum 2 points.";
      } else if (binsNo == 2) {
        if (!linear) {
          result["WorkspaceToInterpolate"] = "Workspace has only 2 points, "
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
  if (histNoToMatch > 1 && !iws->isCommonBins()) {
    g_log.warning() << "The workspace to interpolate doesn't have common bins, SplineInterpolation algorithm will use "
                       "the x-axis of the first spectrum.\n";
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
      Mantid::Kernel::CubicSpline<double, double> spline(iwspt->x(i).rawData(), iwspt->y(i).rawData());
      // NOTE for legacy compatibility, this uses only the FIRST spectrum's X-axis for all other interpolations
      outputWorkspace->mutableY(i) = spline(mwspt->x(0).rawData());
      for (int j = 0; j < derivOrder; ++j) {
        derivs[i]->mutableY(j) = spline.deriv(mwspt->x(0).rawData(), j + 1);
      }
      pgress.report();
    }
  } else {
    // perform linear interpolation

    // first check that the x-axis (first spectrum) is sorted ascending
    if (!std::is_sorted(mwspt->x(0).rawData().begin(), mwspt->x(0).rawData().end())) {
      throw std::runtime_error("X-axis of the workspace to match is not sorted. "
                               "Consider calling SortXAxis before.");
    }

    for (size_t i = 0; i < histNo; ++i) {
      // figure out the interpolation range
      const std::pair<size_t, size_t> range = findInterpolationRange(iwspt, mwspt, i);

      // set up the function that needs to be interpolated
      Mantid::Kernel::LinearSpline<double, double> spline(iwspt->x(i).rawData(), iwspt->y(i).rawData());

      // perform interpolation in the range
      // NOTE for legacy compatibility, this uses only the FIRST spectrum's X-axis for all other interpolations
      std::vector<double> yNew(mwspt->x(0).size());
      std::span<double const> xInRange(mwspt->x(0).cbegin() + range.first, mwspt->x(0).cbegin() + range.second);
      std::vector<double> yInterp = spline(xInRange);
      std::move(yInterp.begin(), yInterp.end(), yNew.begin() + range.first);

      // flat extrapolation outside the range
      const double yFirst = iwspt->y(i).front();
      const double yLast = iwspt->y(i).back();
      std::fill(yNew.begin(), yNew.begin() + range.first, yFirst);
      std::fill(yNew.begin() + range.second, yNew.end(), yLast);

      // set the output
      outputWorkspace->mutableY(i) = yNew;

      // if derivatives are requested, only give first-order
      if (order > 0) {
        auto &deriv = derivs[i]->mutableY(0);
        // 0 outside the range
        std::fill(deriv.begin(), deriv.begin() + range.first, 0.0);
        std::fill(deriv.begin() + range.second, deriv.end(), 0.0);
        // eval inside range
        std::vector<double> derivInterp = spline.deriv(xInRange);
        std::move(derivInterp.begin(), derivInterp.end(), deriv.begin() + range.first);
      }
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
API::MatrixWorkspace_sptr SplineInterpolation::setupOutputWorkspace(const API::MatrixWorkspace_sptr &mws,
                                                                    const API::MatrixWorkspace_sptr &iws) const {
  const size_t numSpec = iws->getNumberHistograms();
  MatrixWorkspace_sptr outputWorkspace = WorkspaceFactory::Instance().create(mws, numSpec);
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
MatrixWorkspace_sptr SplineInterpolation::convertBinnedData(MatrixWorkspace_sptr workspace) {
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
std::pair<size_t, size_t> SplineInterpolation::findInterpolationRange(const MatrixWorkspace_const_sptr &iwspt,
                                                                      const MatrixWorkspace_sptr &mwspt,
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
                    ": Will perform flat extrapolation outside bin range: " + std::to_string(firstIndex) + " to " +
                    std::to_string(lastIndex) + "\n";

  g_log.debug(log);

  return std::make_pair(firstIndex, lastIndex);
}
} // namespace Mantid::CurveFitting::Algorithms
