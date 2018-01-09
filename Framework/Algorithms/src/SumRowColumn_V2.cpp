//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/SumRowColumn_V2.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/Unit.h"

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SumRowColumn_V2)

using namespace Mantid::Kernel;
using namespace Mantid::API;

void SumRowColumn_V2::init() {
  // Assume input workspace has correct spectra in it - no more and no less
  declareProperty(
      make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input),
      "The input workspace, which must contain all the spectra from the bank "
      "of interest - no more and no less (so 128x128 or 192x192).");
  declareProperty(make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                   Direction::Output),
                  "The name of the workspace in which to store the result.");

  // Need to select whether to sum rows or columns
  std::vector<std::string> orientation{"D_H", "D_V"};
  declareProperty("Orientation", "",
                  boost::make_shared<StringListValidator>(orientation),
                  "Whether to sum rows (D_H) or columns (D_V).");

  // This is the range to select - the whole lot by default
  declareProperty(
      "XMin", EMPTY_DBL(),
      "The starting X value for each spectrum to include in the summation.");
  declareProperty(
      "XMax", EMPTY_DBL(),
      "The ending X value for each spectrum to include in the summation.");

  // For selecting a column range - the whole lot by default
  auto mustBePositive = boost::make_shared<BoundedValidator<int>>();
  mustBePositive->setLower(0);
  declareProperty("HOverVMin", EMPTY_INT(), mustBePositive,
                  "The first row to include in the summation when summing by "
                  "columns, or vice versa.");
  declareProperty("HOverVMax", EMPTY_INT(), mustBePositive,
                  "The last row to include in the summation when summing by "
                  "columns, or vice versa.");

  declareProperty("XDim", EMPTY_INT(), mustBePositive,
                  "The horizontal size of the detector.");
  declareProperty("YDim", EMPTY_INT(), mustBePositive,
                  "The vertical size of the detector.");
}

void SumRowColumn_V2::exec() {
  // First task is to integrate the input workspace
  MatrixWorkspace_const_sptr integratedWS = integrateWorkspace();

  const size_t numSpec = integratedWS->getNumberHistograms();

  int x_dim = getProperty("XDim");
  int y_dim = getProperty("YDim");
  int unit_cell_grouping = numSpec/(x_dim*y_dim);
  int dimension_scale = sqrt(unit_cell_grouping);
  // Check number of spectra is 128*128 or 192*192. Print warning if not.
  if (unit_cell_grouping%4 != 0) {
    g_log.warning()
        << "The input workspace has " << numSpec << " spectra."
        << "This is not a multiple of 4 of the expected number - did you make a mistake?\n";
  }

  // This is the dimension if all rows/columns are included
  if (dimension_scale%1 != 0){
      g_log.error()
        << "The dimension scale is not an integer.";
  }
  int x_dim_actual = x_dim / dimension_scale;
  int y_dim_actual = y_dim / dimension_scale;

  // Get the orientation
  const std::string orientation = getProperty("Orientation");
  const bool horizontal = (orientation == "D_H" ? 1 : 0);
  

  // Check the column range properties
  int start = getProperty("HOverVMin");
  int end = getProperty("HOverVMax");
  if (isEmpty(start))
    start = 0;

  if (horizontal) {
    if (isEmpty(end) || end > x_dim_actual - 1)
      end = x_dim_actual - 1;
  }
  else {
      if (isEmpty(end) || end > y_dim_actual - 1)
      end = y_dim_actual - 1;
  }

  if (start > end) {
      g_log.error("H/V_Min must be less than H/V_Max");
      throw std::out_of_range("H/V_Min must be less than H/V_Max");
    }
  

  MatrixWorkspace_sptr outputWS =
      WorkspaceFactory::Instance().create(integratedWS, 1, x_dim_actual, y_dim_actual);
  // Remove the unit
  outputWS->getAxis(0)->unit().reset(new Mantid::Kernel::Units::Empty);

  // Get references to the vectors for the results
  auto &X = outputWS->mutableX(0);
  auto &Y = outputWS->mutableY(0);

  

  
  int vector_size = horizontal ? y_dim_actual : x_dim_actual;
  Progress progress(this, 0.0, 1.0, vector_size);

  for (int i = 0; i < vector_size; ++i) {
    // Copy X values
    X[i] = i;

    // Now loop over calculating Y's
    for (int j = start; j <= end; ++j) {
      const int index = (horizontal ? (i + j * x_dim_actual) : (i * y_dim_actual + j));
      Y[i] += integratedWS->y(index)[0];
    }
  }

  setProperty("OutputWorkspace", outputWS);
}

/** Call Integration as a Child Algorithm
 *  @return The integrated workspace
 */
MatrixWorkspace_sptr SumRowColumn_V2::integrateWorkspace() {
  g_log.debug() << "Integrating input workspace\n";

  IAlgorithm_sptr childAlg = createChildAlgorithm("Integration");
  // pass inputed values straight to this Child Algorithm, checking must be done
  // there
  childAlg->setProperty<MatrixWorkspace_sptr>("InputWorkspace",
                                              getProperty("InputWorkspace"));
  childAlg->setProperty<double>("RangeLower", getProperty("XMin"));
  childAlg->setProperty<double>("RangeUpper", getProperty("XMax"));
  childAlg->executeAsChildAlg();
  return childAlg->getProperty("OutputWorkspace");
}

} // namespace Algorithms
} // namespace Mantid