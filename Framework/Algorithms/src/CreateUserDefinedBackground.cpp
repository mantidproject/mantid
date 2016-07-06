#include "MantidAlgorithms/CreateUserDefinedBackground.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidKernel/Interpolation.h"

#include <algorithm>

namespace Mantid {
namespace Algorithms {

using Mantid::Kernel::Direction;
using Mantid::API::WorkspaceProperty;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CreateUserDefinedBackground)

//----------------------------------------------------------------------------------------------

/// Algorithm's name for identification. @see Algorithm::name
const std::string CreateUserDefinedBackground::name() const {
  return "CreateUserDefinedBackground";
}

/// Algorithm's version for identification. @see Algorithm::version
int CreateUserDefinedBackground::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string CreateUserDefinedBackground::category() const {
  return "CorrectionFunctions\\BackgroundCorrections";
}

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
  declareProperty(Kernel::make_unique<WorkspaceProperty<API::MatrixWorkspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "Input workspace containing data and background");
  declareProperty(Kernel::make_unique<WorkspaceProperty<API::ITableWorkspace>>(
                      "BackgroundPoints", "", Direction::Input),
                  "Table containing user-defined background points");
  declareProperty(Kernel::make_unique<WorkspaceProperty<API::MatrixWorkspace>>(
                      "OutputBackgroundWorkspace", "", Direction::Output),
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

  setProperty("OutputBackgroundWorkspace",
              API::MatrixWorkspace_sptr(std::move(outputWS)));
}

/**
 * Cleans up input points table by sorting points and removing any (0, 0) blank
 * rows
 * @param table :: [input, output] Table of points to work on
 */
void CreateUserDefinedBackground::cleanUpTable(
    API::ITableWorkspace_sptr &table) const {
  // Sort the table
  std::vector<std::pair<std::string, bool>> sortArgs;
  sortArgs.emplace_back(table->getColumn(0)->name(), true);
  table->sort(sortArgs);

  // Delete blank (zero) rows
  std::vector<size_t> blankRows;
  const auto isZero = [](const double n) {
    return !(fabs(n) > std::numeric_limits<double>::epsilon());
  };
  for (size_t i = 0; i < table->rowCount(); i++) {
    double x, y;
    API::TableRow row = table->getRow(i);
    row >> x >> y;
    if (!isZero(x)) {
      break;
    } else if (isZero(y)) {
      blankRows.push_back(i);
    }
  }
  for (const auto &row : blankRows) {
    table->removeRow(row);
  }
}

/**
 * Extend background to limits of data: if it doesn't extend this far already,
 * insert first and last points of data into it.
 * @param background :: [input, output] Table of background points to work on
 * @param data :: [input] Input workspace with data
 */
void CreateUserDefinedBackground::extendBackgroundToData(
    API::ITableWorkspace_sptr &background,
    const API::MatrixWorkspace_const_sptr &data) const {
  const auto &xData = data->readX(0);
  const auto &yData = data->readY(0);
  const auto xMinMax = std::minmax_element(xData.begin(), xData.end());
  // If first point > data minimum, insert a new first point
  if (background->Double(0, 0) > *xMinMax.first) {
    background->insertRow(0);
    API::TableRow firstRow = background->getFirstRow();
    const auto dataPosition = xMinMax.first - xData.begin();
    firstRow << xData[dataPosition] << yData[dataPosition + 1];
  }
  // If last point < data maximum, append a new last point
  if (background->Double(background->rowCount() - 1, 0) < *xMinMax.second) {
    API::TableRow lastRow = background->appendRow();
    const auto dataPosition = xMinMax.second - xData.begin();
    lastRow << xData[dataPosition] << yData[dataPosition - 1];
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
CreateUserDefinedBackground::createBackgroundWorkspace(
    const API::ITableWorkspace_const_sptr &background,
    const API::MatrixWorkspace_const_sptr &data) const {
  auto outputWS = data->clone();

  const auto &xData = outputWS->readX(0);
  MantidVec yBackground;
  MantidVec eBackground(outputWS->blocksize(), 0);

  // Interpolate Y data in the table
  const auto &lerp = getInterpolator(background, data);
  const bool isHisto = outputWS->isHistogramData();
  for (size_t i = 0; i < data->blocksize(); i++) {
    const double x = isHisto ? (xData[i] + xData[i + 1]) * 0.5 : xData[i];
    double y = lerp.value(x);
    if (isHisto) {
      y *= (xData[i + 1] - xData[i]); // bin width
    }
    yBackground.push_back(y);
  }

  // Apply Y and E data to all spectra in the workspace
  for (size_t spec = 0; spec < outputWS->getNumberHistograms(); spec++) {
    outputWS->dataY(spec) = yBackground;
    outputWS->dataE(spec) = eBackground;
  }

  return API::MatrixWorkspace_sptr(std::move(outputWS));
}

/**
 * Set up and return an interpolation object using the given data
 * @param background :: [input] Background data to interpolate
 * @param workspace :: [input] Workspace to use for units
 * @returns :: Interpolation object ready for use
 */
Kernel::Interpolation CreateUserDefinedBackground::getInterpolator(
    const API::ITableWorkspace_const_sptr &background,
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
std::map<std::string, std::string>
CreateUserDefinedBackground::validateInputs() {
  std::map<std::string, std::string> errors;

  const static std::string pointsProp = "BackgroundPoints",
                           inputProp = "InputWorkspace";
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

} // namespace Algorithms
} // namespace Mantid
