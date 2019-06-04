// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/SumRowColumn.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/Unit.h"

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SumRowColumn)

using namespace Mantid::Kernel;
using namespace Mantid::API;

void SumRowColumn::init() {
  // Assume input workspace has correct spectra in it - no more and no less
  declareProperty(
      std::make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input),
      "The input workspace, which must contain all the spectra from the bank "
      "of interest - no more and no less (so 128x128 or 192x192).");
  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
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
}

void SumRowColumn::exec() {
  // First task is to integrate the input workspace
  MatrixWorkspace_const_sptr integratedWS = integrateWorkspace();

  const size_t numSpec = integratedWS->getNumberHistograms();
  // Check number of spectra is 128*128 or 192*192. Print warning if not.
  if (numSpec != 16384 && numSpec != 36864) {
    g_log.warning()
        << "The input workspace has " << numSpec << " spectra."
        << "This is not 128*128 or 192*192 - did you make a mistake?\n";
  }

  // This is the dimension if all rows/columns are included
  const int dim = static_cast<int>(std::sqrt(static_cast<double>(numSpec)));

  // Check the column range properties
  int start = getProperty("HOverVMin");
  int end = getProperty("HOverVMax");
  if (isEmpty(start))
    start = 0;
  if (isEmpty(end) || end > dim - 1)
    end = dim - 1;
  if (start > end) {
    g_log.error("H/V_Min must be less than H/V_Max");
    throw std::out_of_range("H/V_Min must be less than H/V_Max");
  }

  MatrixWorkspace_sptr outputWS =
      WorkspaceFactory::Instance().create(integratedWS, 1, dim, dim);
  // Remove the unit
  outputWS->getAxis(0)->unit().reset(new Mantid::Kernel::Units::Empty);

  // Get references to the vectors for the results
  auto &X = outputWS->mutableX(0);
  auto &Y = outputWS->mutableY(0);

  // Get the orientation
  const std::string orientation = getProperty("Orientation");
  const bool horizontal = (orientation == "D_H" ? 1 : 0);

  Progress progress(this, 0.0, 1.0, dim);
  for (int i = 0; i < dim; ++i) {
    // Copy X values
    X[i] = i;

    // Now loop over calculating Y's
    for (int j = start; j <= end; ++j) {
      const int index = (horizontal ? (i + j * dim) : (i * dim + j));
      Y[i] += integratedWS->y(index)[0];
    }
  }

  setProperty("OutputWorkspace", outputWS);
}

/** Call Integration as a Child Algorithm
 *  @return The integrated workspace
 */
MatrixWorkspace_sptr SumRowColumn::integrateWorkspace() {
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
