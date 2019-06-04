// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/CreateSingleValuedWorkspace.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidDataObjects/WorkspaceSingleValue.h"
#include "MantidIndexing/IndexInfo.h"
#include "MantidKernel/BoundedValidator.h"

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CreateSingleValuedWorkspace)

void CreateSingleValuedWorkspace::init() {
  using namespace Mantid::Kernel;
  using namespace Mantid::API;
  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                   Direction::Output),
                  "Name to use for the output workspace");
  declareProperty("DataValue", 0.0, "The value to place in the workspace");
  auto mustBePositive = boost::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0.0);
  declareProperty("ErrorValue", 0.0, mustBePositive,
                  "The error value to place in the workspace (default 0.0)");
}

void CreateSingleValuedWorkspace::exec() {
  double dataValue = getProperty("DataValue");
  double errorValue = getProperty("ErrorValue");

  Indexing::IndexInfo indexInfo(1, Parallel::StorageMode::Cloned,
                                communicator());
  auto singleValued = DataObjects::create<DataObjects::WorkspaceSingleValue>(
      indexInfo, HistogramData::Points(1));

  singleValued->mutableX(0)[0] = 0.0;
  singleValued->mutableY(0)[0] = dataValue;
  singleValued->mutableE(0)[0] = errorValue;

  setProperty("OutputWorkspace", std::move(singleValued));
}

Parallel::ExecutionMode CreateSingleValuedWorkspace::getParallelExecutionMode(
    const std::map<std::string, Parallel::StorageMode> &storageModes) const {
  static_cast<void>(storageModes);
  return Parallel::ExecutionMode::Identical;
}

} // namespace Algorithms
} // namespace Mantid
