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
#include "MantidQtWidgets/Common/AlgorithmRuntimeProps.h"
#include "MantidQtWidgets/Common/BatchAlgorithmRunner.h"
#include "Reduction/Item.h"
#include "Reduction/PreviewRow.h"

#include <string>
#include <vector>

using namespace MantidQt::CustomInterfaces::ISISReflectometry;
using namespace Mantid::API;
using MantidQt::API::IConfiguredAlgorithm;
using MantidQt::API::IConfiguredAlgorithm_sptr;

namespace {
void updateInputWorkspacesProperties(MantidQt::API::IAlgorithmRuntimeProps &properties,
                                     std::vector<std::string> const &inputRunNumbers) {
  AlgorithmProperties::update("InputRunList", inputRunNumbers, properties);
}

} // namespace

namespace MantidQt::CustomInterfaces::ISISReflectometry::PreprocessRow {

/** Create a configured algorithm for processing a row. The algorithm
 * properties are set from the reduction configuration model and the
 * cell values in the given row.
 * @param model : the reduction configuration model
 * @param row : the row from the runs table
 */
IConfiguredAlgorithm_sptr createConfiguredAlgorithm(IBatch const & /*model*/, PreviewRow &row, IAlgorithm_sptr alg) {
  // Create the algorithm
  if (!alg) {
    alg = Mantid::API::AlgorithmManager::Instance().create("ReflectometryISISPreprocess");
  }
  alg->setRethrows(true);
  alg->setAlwaysStoreInADS(false);
  alg->getPointerToProperty("OutputWorkspace")->createTemporaryValue();

  // Set the algorithm properties from the model
  auto properties = std::make_unique<MantidQt::API::AlgorithmRuntimeProps>();
  updateInputWorkspacesProperties(*properties, row.runNumbers());

  // Return the configured algorithm
  auto jobAlgorithm =
      std::make_shared<BatchJobAlgorithm>(std::move(alg), std::move(properties), updateRowOnAlgorithmComplete, &row);
  return jobAlgorithm;
}

void updateRowOnAlgorithmComplete(const IAlgorithm_sptr &algorithm, Item &item) {
  auto &row = dynamic_cast<PreviewRow &>(item);
  MatrixWorkspace_sptr outputWs = algorithm->getProperty("OutputWorkspace");
  row.setLoadedWs(outputWs);
  // TODO reset the rest of the workspaces associated with the workflow
}
} // namespace MantidQt::CustomInterfaces::ISISReflectometry::PreprocessRow
