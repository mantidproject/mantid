// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "RowPreprocessingAlgorithm.h"
#include "../../Reduction/IBatch.h"
#include "BatchJobAlgorithm.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AlgorithmProperties.h"
#include "MantidAPI/AlgorithmRuntimeProps.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/Workspace.h"
#include "MantidQtWidgets/Common/BatchAlgorithmRunner.h"
#include "Reduction/Item.h"
#include "Reduction/PreviewRow.h"

#include <string>
#include <vector>

using namespace MantidQt::CustomInterfaces::ISISReflectometry;
using MantidQt::API::IConfiguredAlgorithm;
using MantidQt::API::IConfiguredAlgorithm_sptr;

namespace {
void updateInputWorkspacesProperties(Mantid::API::IAlgorithmRuntimeProps &properties,
                                     std::vector<std::string> const &inputRunNumbers) {
  Mantid::API::AlgorithmProperties::update("InputRunList", inputRunNumbers, properties);
}

} // namespace

namespace MantidQt::CustomInterfaces::ISISReflectometry::PreprocessRow {

/** Create a configured algorithm for preprocessing a row. The algorithm
 * properties are set from the given row.
 * @param row : the row from the preview tab
 */
IConfiguredAlgorithm_sptr createConfiguredAlgorithm(IBatch const &, PreviewRow &row, Mantid::API::IAlgorithm_sptr alg) {
  // Create the algorithm
  if (!alg) {
    alg = Mantid::API::AlgorithmManager::Instance().create("ReflectometryISISPreprocess");
  }
  alg->setRethrows(true);
  alg->setAlwaysStoreInADS(false);
  alg->getPointerToProperty("OutputWorkspace")->createTemporaryValue();

  // Set the algorithm properties from the row
  auto properties = std::make_unique<Mantid::API::AlgorithmRuntimeProps>();
  updateInputWorkspacesProperties(*properties, row.runNumbers());

  // Return the configured algorithm
  auto jobAlgorithm =
      std::make_shared<BatchJobAlgorithm>(std::move(alg), std::move(properties), updateRowOnAlgorithmComplete, &row);
  return jobAlgorithm;
}

void updateRowOnAlgorithmComplete(const Mantid::API::IAlgorithm_sptr &algorithm, Item &item) {
  auto &row = dynamic_cast<PreviewRow &>(item);
  Mantid::API::Workspace_sptr outputWs = algorithm->getProperty("OutputWorkspace");
  validatePreviewWorkspace(outputWs);
  row.setLoadedWs(outputWs);
  // TODO reset the rest of the workspaces associated with the workflow
}
} // namespace MantidQt::CustomInterfaces::ISISReflectometry::PreprocessRow
