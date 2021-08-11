// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "RowPreprocessingAlgorithm.h"
#include "AlgorithmProperties.h"
#include "BatchJobAlgorithm.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidQtWidgets/Common/BatchAlgorithmRunner.h"
#include "Reduction/Item.h"
#include "Reduction/PreviewRow.h"

#include <string>
#include <vector>

using namespace MantidQt::CustomInterfaces::ISISReflectometry;
using namespace Mantid::API;
using MantidQt::API::IConfiguredAlgorithm_sptr;

namespace {
void updateInputWorkspacesProperties(AlgorithmRuntimeProps &properties,
                                     std::vector<std::string> const &inputRunNumbers) {
  AlgorithmProperties::update("InputRunList", inputRunNumbers, properties);
}

void updateRowCallback(const IAlgorithm_sptr &, Item &) {
  // TODO decide what to do with the output workspace
  // auto &row = dynamic_cast<Row &>(item);
  // auto const outputWs = AlgorithmProperties::getOutputWorkspace(algorithm, "OutputWorkspace");
}
} // namespace

namespace MantidQt::CustomInterfaces::ISISReflectometry {

/** Create a configured algorithm for processing a row. The algorithm
 * properties are set from the reduction configuration model and the
 * cell values in the given row.
 * @param model : the reduction configuration model
 * @param row : the row from the runs table
 */
IConfiguredAlgorithm_sptr createConfiguredAlgorithm(IBatch const & /*model*/, PreviewRow &row) {
  // Create the algorithm
  auto alg = Mantid::API::AlgorithmManager::Instance().create("ReflectometryISISPreprocess");
  alg->setRethrows(true);

  // Set the algorithm properties from the model
  auto properties = AlgorithmRuntimeProps();
  updateInputWorkspacesProperties(properties, row.runNumbers());

  // Return the configured algorithm
  auto jobAlgorithm = std::make_shared<BatchJobAlgorithm>(alg, properties, updateRowCallback, &row);
  return jobAlgorithm;
}
} // namespace MantidQt::CustomInterfaces::ISISReflectometry