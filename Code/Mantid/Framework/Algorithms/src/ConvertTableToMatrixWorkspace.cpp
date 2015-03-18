//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/ConvertTableToMatrixWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/MandatoryValidator.h"

#include <boost/mpl/if.hpp>
#include <boost/type_traits.hpp>
#include <sstream>

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ConvertTableToMatrixWorkspace)

using namespace Kernel;
using namespace API;

void ConvertTableToMatrixWorkspace::init() {
  declareProperty(new WorkspaceProperty<API::ITableWorkspace>(
                      "InputWorkspace", "", Direction::Input),
                  "An input TableWorkspace.");
  declareProperty(
      new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output),
      "An output Workspace2D.");
  declareProperty("ColumnX", "",
                  boost::make_shared<MandatoryValidator<std::string>>(),
                  "The column name for the X vector.");
  declareProperty("ColumnY", "",
                  boost::make_shared<MandatoryValidator<std::string>>(),
                  "The column name for the Y vector.");
  declareProperty("ColumnE", "",
                  "The column name for the E vector (optional).");
}

void ConvertTableToMatrixWorkspace::exec() {
  ITableWorkspace_const_sptr inputWorkspace = getProperty("InputWorkspace");
  std::string columnX = getProperty("ColumnX");
  std::string columnY = getProperty("ColumnY");
  std::string columnE = getProperty("ColumnE");

  size_t nrows = inputWorkspace->rowCount();
  if (nrows == 0) {
    throw std::runtime_error("The input table is empty");
  }
  std::vector<double> X(nrows);
  std::vector<double> Y(nrows);
  inputWorkspace->getColumn(columnX)->numeric_fill(X);
  inputWorkspace->getColumn(columnY)->numeric_fill(Y);

  MatrixWorkspace_sptr outputWorkspace =
      WorkspaceFactory::Instance().create("Workspace2D", 1, nrows, nrows);
  outputWorkspace->dataX(0).assign(X.begin(), X.end());
  outputWorkspace->dataY(0).assign(Y.begin(), Y.end());
  if (!columnE.empty()) {
    std::vector<double> E(nrows);
    inputWorkspace->getColumn(columnE)->numeric_fill(E);
    outputWorkspace->dataE(0).assign(E.begin(), E.end());
  }

  auto labelX = boost::dynamic_pointer_cast<Units::Label>(
      UnitFactory::Instance().create("Label"));
  labelX->setLabel(columnX);
  outputWorkspace->getAxis(0)->unit() = labelX;

  outputWorkspace->setYUnitLabel(columnY);

  setProperty("OutputWorkspace", outputWorkspace);
}

} // namespace Algorithms
} // namespace Mantid
