// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/CreateUserDefinedBackground.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Interpolation.h"

#include <algorithm>

namespace Mantid::Algorithms {

// Key for the "normalize data to bin width" plot option
const std::string CreateUserDefinedBackground::AUTODISTRIBUTIONKEY = "graph1d.autodistribution";

using Mantid::API::WorkspaceProperty;
using Mantid::HistogramData::Frequencies;
using Mantid::HistogramData::FrequencyStandardDeviations;
using Mantid::HistogramData::Histogram;
using Mantid::Kernel::Direction;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CreateUserDefinedBackground)

//----------------------------------------------------------------------------------------------

/// Algorithm's name for identification. @see Algorithm::name
const std::string CreateUserDefinedBackground::name() const { return "CreateUserDefinedBackground"; }

/// Algorithm's version for identification. @see Algorithm::version
int CreateUserDefinedBackground::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string CreateUserDefinedBackground::category() const { return "CorrectionFunctions\\BackgroundCorrections"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string CreateUserDefinedBackground::summary() const {
  return "Creates a workspace of background data from a user-supplied set of "
         "points. This workspace can then be subtracted from the original "
         "data.";
}

//----------------------------------------------------------------------------------------------
/**
 * Initialize the algorithm's properties.
 */
void CreateUserDefinedBackground::init() {
  declareProperty(std::make_unique<WorkspaceProperty<API::MatrixWorkspace>>("InputWorkspace", "", Direction::Input),
                  "Input workspace containing data and background");
  declareProperty(std::make_unique<WorkspaceProperty<API::ITableWorkspace>>("BackgroundPoints", "", Direction::Input),
                  "Table containing user-defined background points");
  declareProperty(
      std::make_unique<WorkspaceProperty<API::MatrixWorkspace>>("OutputBackgroundWorkspace", "", Direction::Output),
      "Workspace containing background to be subtracted");
}

//----------------------------------------------------------------------------------------------
/**
 * Execute the algorithm.
 */
void CreateUserDefinedBackground::exec() {
  const API::MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  API::ITableWorkspace_sptr pointsTable = getProperty("BackgroundPoints");

  // Clean up input points table and extend to range of data
  cleanUpTable(pointsTable);
  extendBackgroundToData(pointsTable, inputWS);

  // Generate output workspace with background data
  const auto outputWS = createBackgroundWorkspace(pointsTable, inputWS);

  setProperty("OutputBackgroundWorkspace", API::MatrixWorkspace_sptr(outputWS));
}

/**
 * Cleans up input points table by sorting points and removing any (0, 0) blank
 * rows from the end of the table
 * (Only delete (0, 0) from the end as other (0, 0) are real points.)
 * @param table :: [input, output] Table of points to work on
 */
void CreateUserDefinedBackground::cleanUpTable(API::ITableWorkspace_sptr &table) const {
  // Delete blank (zero) rows at the end of the table
  std::vector<size_t> blankRows;
  const auto isZero = [](const double n) { return !(fabs(n) > std::numeric_limits<double>::epsilon()); };
  for (size_t i = table->rowCount() - 1; i > 0; i--) {
    double x, y;
    API::TableRow row = table->getRow(i);
    row >> x >> y;
    if (!isZero(x)) {
      break;
    } else if (isZero(y)) {
      blankRows.emplace_back(i);
    }
  }
  for (const auto &row : blankRows) {
    table->removeRow(row);
  }

  // Sort the table
  std::vector<std::pair<std::string, bool>> sortArgs;
  sortArgs.emplace_back(table->getColumn(0)->name(), true);
  table->sort(sortArgs);
}

/**
 * Extend background to limits of data: if it doesn't extend this far already,
 * insert first and last points of data into it.
 * @param background :: [input, output] Table of background points to work on
 * @param data :: [input] Input workspace with data
 */
void CreateUserDefinedBackground::extendBackgroundToData(API::ITableWorkspace_sptr &background,
                                                         const API::MatrixWorkspace_const_sptr &data) const {
  const auto &xPoints = data->points(0);

  // If first point > data minimum, insert a new first point
  if (background->Double(0, 0) > xPoints[0]) {
    background->insertRow(0);
    API::TableRow firstRow = background->getFirstRow();
    firstRow << xPoints.front() << background->Double(0, 1);
  }
  // If last point < data maximum, append a new last point
  const size_t endIndex = background->rowCount() - 1;
  if (background->Double(endIndex, 0) < xPoints.back()) {
    API::TableRow lastRow = background->appendRow();
    lastRow << xPoints.back() << background->Double(endIndex, 1);
  }
}

/**
 * Given a table of background points and the original workspace,
 * return a new workspace containing interpolated background data.
 * The same background is assumed for all spectra.
 * @param background :: [input] Table of background points
 * @param data :: [input] Original data workspace
 * @returns :: Workspace containing background to be subtracted
 */
API::MatrixWorkspace_sptr
CreateUserDefinedBackground::createBackgroundWorkspace(const API::ITableWorkspace_const_sptr &background,
                                                       const API::MatrixWorkspace_const_sptr &data) const {
  auto outputWS = data->clone();

  const auto &xPoints = outputWS->points(0);
  const auto &xBinEdges = outputWS->binEdges(0);
  std::vector<double> yBackground;
  std::vector<double> eBackground(xPoints.size(), 0);

  // Interpolate Y data in the table to get y for each point
  const auto &lerp = getInterpolator(background, data);
  for (const auto &x : xPoints) {
    const double y = lerp.value(x);
    yBackground.emplace_back(y);
  }

  auto histogram = outputWS->histogram(0);
  if (histogram.yMode() == Histogram::YMode::Frequencies) {
    histogram.setFrequencies(yBackground);
    histogram.setFrequencyStandardDeviations(eBackground);
  } else {
    if (data->isHistogramData() &&
        Kernel::ConfigService::Instance().getValue<bool>(AUTODISTRIBUTIONKEY).value_or(false)) {
      // Background data is actually frequencies, we put it into temporary to
      // benefit from automatic conversion in setCounts(), etc.
      histogram.setCounts(Frequencies(yBackground), xBinEdges);
      histogram.setCountStandardDeviations(FrequencyStandardDeviations(eBackground), xBinEdges);
    } else {
      histogram.setCounts(yBackground);
      histogram.setCountStandardDeviations(eBackground);
    }
  }

  // Apply Y and E data to all spectra in the workspace
  for (size_t spec = 0; spec < outputWS->getNumberHistograms(); spec++) {
    // Setting same histogram for all spectra, data is shared, saving memory
    outputWS->setHistogram(spec, histogram);
  }

  return API::MatrixWorkspace_sptr(std::move(outputWS));
}

/**
 * Set up and return an interpolation object using the given data
 * @param background :: [input] Background data to interpolate
 * @param workspace :: [input] Workspace to use for units
 * @returns :: Interpolation object ready for use
 */
Kernel::Interpolation
CreateUserDefinedBackground::getInterpolator(const API::ITableWorkspace_const_sptr &background,
                                             const API::MatrixWorkspace_const_sptr &workspace) const {
  Kernel::Interpolation lerp;
  lerp.setMethod("linear");
  lerp.setXUnit(workspace->getAxis(0)->unit()->unitID());
  lerp.setYUnit(workspace->getAxis(1)->unit()->unitID());

  // Set up data from table
  const auto xColumn = background->getColumn(0);
  const auto yColumn = background->getColumn(1);
  for (size_t i = 0; i < background->rowCount(); i++) {
    double x = xColumn->cell<double>(i);
    double y = yColumn->cell<double>(i);
    lerp.addPoint(x, y);
  }
  return lerp;
}

/**
 * Validate input properties:
 * - Table of points must have two numeric columns (X, Y)
 * - Table of points must contain at least two points
 * - Input workspace must contain at least one spectrum and two points
 * - Input workspace must have common bins in all spectra
 * @returns :: map of property names to errors (empty if no errors)
 */
std::map<std::string, std::string> CreateUserDefinedBackground::validateInputs() {
  std::map<std::string, std::string> errors;

  const static std::string pointsProp = "BackgroundPoints", inputProp = "InputWorkspace";
  const API::ITableWorkspace_const_sptr pointsTable = getProperty(pointsProp);
  if (pointsTable) {
    if (pointsTable->columnCount() != 2) {
      errors[pointsProp] = "Table of points must have two columns (X, Y)";
    }
    for (size_t col = 0; col < pointsTable->columnCount(); col++) {
      const std::string colType = pointsTable->getColumn(col)->type();
      if (colType != "double" && colType != "int") {
        errors[pointsProp] = "Table of points must have numeric columns";
      }
    }
    if (pointsTable->rowCount() < 2) {
      errors[pointsProp] = "Table of points must contain at least two points";
    }
  }

  const API::MatrixWorkspace_const_sptr inputWS = getProperty(inputProp);
  if (inputWS) {
    if (inputWS->getNumberHistograms() == 0 || inputWS->blocksize() < 2) {
      errors[inputProp] = "Input workspace must contain some data";
    }
    if (!inputWS->isCommonBins()) {
      errors[inputProp] = "Input workspace must have common bins";
    }
  }

  return errors;
}

} // namespace Mantid::Algorithms
