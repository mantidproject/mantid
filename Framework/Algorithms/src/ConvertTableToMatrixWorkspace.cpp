// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/ConvertTableToMatrixWorkspace.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/UnitFactory.h"

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ConvertTableToMatrixWorkspace)

using namespace Kernel;
using namespace API;
using namespace HistogramData;
using namespace DataObjects;

void ConvertTableToMatrixWorkspace::init() {
  declareProperty(std::make_unique<WorkspaceProperty<API::ITableWorkspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "An input TableWorkspace.");
  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                   Direction::Output),
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

  const size_t nrows = inputWorkspace->rowCount();
  if (nrows == 0) {
    throw std::runtime_error("The input table is empty");
  }

  const auto X = inputWorkspace->getColumn(columnX)->numeric_fill<>();
  const auto Y = inputWorkspace->getColumn(columnY)->numeric_fill<>();

  MatrixWorkspace_sptr outputWorkspace = create<Workspace2D>(1, Points(nrows));

  outputWorkspace->mutableX(0).assign(X.cbegin(), X.cend());
  outputWorkspace->mutableY(0).assign(Y.cbegin(), Y.cend());

  if (!columnE.empty()) {
    outputWorkspace->mutableE(0) =
        inputWorkspace->getColumn(columnE)->numeric_fill<>();
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
