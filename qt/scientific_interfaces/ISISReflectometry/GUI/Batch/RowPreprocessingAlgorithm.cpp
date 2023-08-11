// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "RowPreprocessingAlgorithm.h"
#include "../../Reduction/Experiment.h"
#include "../../Reduction/IBatch.h"
#include "../../Reduction/Instrument.h"
#include "BatchJobAlgorithm.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AlgorithmProperties.h"
#include "MantidAPI/AlgorithmRuntimeProps.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/MatrixWorkspace.h"
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

void updateInstrumentSettingsProperties(Mantid::API::IAlgorithmRuntimeProps &properties, Instrument const &instrument) {
  Mantid::API::AlgorithmProperties::update("CalibrationFile", instrument.calibrationFilePath(), properties);
}

} // namespace

namespace MantidQt::CustomInterfaces::ISISReflectometry::PreprocessRow {

/** Create a configured algorithm for preprocessing a row. The algorithm
 * properties are set from the reduction configuration model and the
 * given row.
 * @param model : the reduction configuration model
 * @param row : the row from the preview tab
 */
IConfiguredAlgorithm_sptr createConfiguredAlgorithm(IBatch const &model, PreviewRow &row,
                                                    Mantid::API::IAlgorithm_sptr alg) {
  // Create the algorithm
  if (!alg) {
    alg = Mantid::API::AlgorithmManager::Instance().create("ReflectometryISISPreprocess");
  }
  alg->setRethrows(true);
  alg->setAlwaysStoreInADS(false);
  alg->getPointerToProperty("OutputWorkspace")->createTemporaryValue();

  // Set the algorithm properties from the model
  auto properties = std::make_unique<Mantid::API::AlgorithmRuntimeProps>();
  updateInputWorkspacesProperties(*properties, row.runNumbers());
  updateInstrumentSettingsProperties(*properties, model.instrument());

  // Return the configured algorithm
  auto jobAlgorithm =
      std::make_shared<BatchJobAlgorithm>(std::move(alg), std::move(properties), updateRowOnAlgorithmComplete, &row);
  return jobAlgorithm;
}

void updateRowOnAlgorithmComplete(const Mantid::API::IAlgorithm_sptr &algorithm, Item &item) {
  auto &row = dynamic_cast<PreviewRow &>(item);
  Mantid::API::Workspace_sptr outputWs = algorithm->getProperty("OutputWorkspace");
  auto matrixWs = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(outputWs);
  if (!matrixWs)
    throw std::runtime_error("Unsupported workspace type; expected MatrixWorkspace");
  row.setLoadedWs(matrixWs);
  // TODO reset the rest of the workspaces associated with the workflow
}
} // namespace MantidQt::CustomInterfaces::ISISReflectometry::PreprocessRow
