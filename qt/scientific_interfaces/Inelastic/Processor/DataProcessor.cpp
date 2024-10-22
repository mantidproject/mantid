// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "DataProcessor.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/OptionalBool.h"

using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using Mantid::Types::Core::DateAndTime;

namespace {
Mantid::Kernel::Logger g_log("DataProcessor");
}

namespace MantidQt::CustomInterfaces {

DataProcessor::DataProcessor(QObject *parent, std::unique_ptr<MantidQt::API::IAlgorithmRunner> algorithmRunner)
    : InelasticTab(parent), m_algorithmRunner(std::move(algorithmRunner)) {
  m_algorithmRunner->subscribe(this);
}

void DataProcessor::setOutputPlotOptionsPresenter(std::unique_ptr<OutputPlotOptionsPresenter> presenter) {
  m_plotOptionsPresenter = std::move(presenter);
}

void DataProcessor::notifyBatchComplete(API::IConfiguredAlgorithm_sptr &algorithm, bool error) {
  if (algorithm->algorithm()->name() != "SaveNexusProcessed") {
    m_runPresenter->setRunEnabled(true);
    runComplete(algorithm->algorithm(), error);
  }
}

void DataProcessor::clearOutputPlotOptionsWorkspaces() { m_plotOptionsPresenter->clearWorkspaces(); }

API::IConfiguredAlgorithm_sptr DataProcessor::setupSaveAlgorithm(const std::string &wsName,
                                                                 const std::string &filename) {
  // Setup the input workspace property
  auto saveProps = std::make_unique<Mantid::API::AlgorithmRuntimeProps>();
  saveProps->setPropertyValue("InputWorkspace", wsName);
  // Setup the algorithm
  auto saveAlgo = AlgorithmManager::Instance().create("SaveNexusProcessed");
  saveAlgo->initialize();

  if (filename.empty())
    saveAlgo->setProperty("Filename", wsName + ".nxs");
  else
    saveAlgo->setProperty("Filename", filename);

  API::IConfiguredAlgorithm_sptr confAlg = std::make_shared<API::ConfiguredAlgorithm>(saveAlgo, std::move(saveProps));
  return confAlg;
}

void DataProcessor::exportPythonDialog() { exportPythonScript(); }

void DataProcessor::setOutputPlotOptionsWorkspaces(std::vector<std::string> const &outputWorkspaces) {
  m_plotOptionsPresenter->setWorkspaces(outputWorkspaces);
}

/**
 * Prevents the loading of data with incorrect naming if passed true
 *
 * @param filter :: true if you want to allow filtering
 */
void DataProcessor::filterInputData(bool filter) { setFileExtensionsByName(filter); }

void DataProcessor::enableLoadHistoryProperty(bool doLoadHistory) { setLoadHistory(doLoadHistory); }

} // namespace MantidQt::CustomInterfaces
