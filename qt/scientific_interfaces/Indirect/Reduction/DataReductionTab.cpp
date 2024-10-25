// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "DataReductionTab.h"
#include "DataReduction.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/OptionalBool.h"
#include "MantidQtWidgets/Common/WorkspaceUtils.h"

using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using namespace MantidQt::MantidWidgets::WorkspaceUtils;
using Mantid::Types::Core::DateAndTime;

namespace {
Mantid::Kernel::Logger g_log("DataReductionTab");
}

namespace MantidQt::CustomInterfaces {

DataReductionTab::DataReductionTab(IDataReduction *idrUI, QObject *parent) : InelasticTab(parent), m_idrUI(idrUI) {
  connect(this, SIGNAL(newInstrumentConfiguration()), this, SLOT(handleNewInstrumentConfiguration()));
}

DataReductionTab::DataReductionTab(IDataReduction *idrUI, std::unique_ptr<API::IAlgorithmRunner> algorithmRunner)
    : InelasticTab(), m_idrUI(idrUI), m_algorithmRunner(std::move(algorithmRunner)) {
  connect(this, SIGNAL(newInstrumentConfiguration()), this, SLOT(handleNewInstrumentConfiguration()));
}

DataReductionTab::~DataReductionTab() = default;

void DataReductionTab::setOutputPlotOptionsWorkspaces(std::vector<std::string> const &outputWorkspaces) {
  m_plotOptionsPresenter->setWorkspaces(outputWorkspaces);
}

void DataReductionTab::handleNewInstrumentConfiguration() { updateInstrumentConfiguration(); }

/**
 * Gets the current instrument workspace
 *
 * @returns Pointer to instrument workspace
 */
Mantid::API::MatrixWorkspace_sptr DataReductionTab::instrumentWorkspace() const {
  return m_idrUI->instrumentWorkspace();
}

/**
 * Gets details for the current instrument configuration defined in Convert To
 * Energy tab.
 *
 * @return Map of information ID to value
 */
QMap<QString, QString> DataReductionTab::getInstrumentDetails() const { return m_idrUI->getInstrumentDetails(); }

QString DataReductionTab::getInstrumentDetail(QString const &key) const {
  return getInstrumentDetail(getInstrumentDetails(), key);
}

QString DataReductionTab::getInstrumentDetail(QMap<QString, QString> const &instrumentDetails,
                                              QString const &key) const {
  validateInstrumentDetail(key);
  return instrumentDetails[key];
}

void DataReductionTab::validateInstrumentDetail(QString const &key) const {
  auto const instrumentName = getInstrumentName().toStdString();

  if (instrumentName.empty())
    throw std::runtime_error("Please select a valid facility and/or instrument.");
  else if (!hasInstrumentDetail(key))
    throw std::runtime_error("Could not find " + key.toStdString() + " for the " + instrumentName +
                             " instrument. Please select a valid instrument.");
}

bool DataReductionTab::hasInstrumentDetail(QString const &key) const {
  return hasInstrumentDetail(getInstrumentDetails(), key);
}

bool DataReductionTab::hasInstrumentDetail(QMap<QString, QString> const &instrumentDetails, QString const &key) const {
  return instrumentDetails.contains(key) && !instrumentDetails[key].isEmpty();
}

/**
 * Returns a pointer to the instrument configuration widget common to all tabs.
 *
 * @return Instrument config widget
 */
MantidWidgets::IInstrumentConfig *DataReductionTab::getInstrumentConfiguration() const {
  return m_idrUI->getInstrumentConfiguration();
}

QString DataReductionTab::getInstrumentName() const { return getInstrumentConfiguration()->getInstrumentName(); }

QString DataReductionTab::getAnalyserName() const { return getInstrumentConfiguration()->getAnalyserName(); }

QString DataReductionTab::getReflectionName() const { return getInstrumentConfiguration()->getReflectionName(); }

/**
 * Gets default peak and background ranges for an instrument in time of flight.
 *
 * @param instName Name of instrument
 * @param analyser Analyser component
 * @param reflection Reflection used
 *
 * @returns A map of range ID to value
 */
std::map<std::string, double> DataReductionTab::getRangesFromInstrument(QString instName, QString analyser,
                                                                        QString reflection) {
  // Get any unset parameters
  if (instName.isEmpty())
    instName = getInstrumentName();
  if (analyser.isEmpty())
    analyser = getAnalyserName();
  if (reflection.isEmpty())
    reflection = getReflectionName();

  std::map<std::string, double> ranges;

  // Get the instrument
  auto const instWorkspace = instrumentWorkspace();
  if (!instWorkspace) {
    return ranges;
  }
  auto inst = instWorkspace->getInstrument();

  // Get the analyser component
  auto comp = inst->getComponentByName(analyser.toStdString());
  if (!comp)
    return ranges;

  // Get the resolution of the analyser
  auto resParams = comp->getNumberParameter("resolution", true);
  if (resParams.size() < 1)
    return ranges;
  double resolution = resParams[0];

  std::vector<double> x;
  x.emplace_back(-6 * resolution);
  x.emplace_back(-5 * resolution);
  x.emplace_back(-2 * resolution);
  x.emplace_back(0);
  x.emplace_back(2 * resolution);
  std::vector<double> y;
  y.emplace_back(1);
  y.emplace_back(2);
  y.emplace_back(3);
  y.emplace_back(4);
  std::vector<double> e(4, 0);

  IAlgorithm_sptr createWsAlg = AlgorithmManager::Instance().create("CreateWorkspace");
  createWsAlg->setChild(true);
  createWsAlg->initialize();
  createWsAlg->setProperty("OutputWorkspace", "__energy");
  createWsAlg->setProperty("DataX", x);
  createWsAlg->setProperty("DataY", y);
  createWsAlg->setProperty("DataE", e);
  createWsAlg->setProperty("Nspec", 1);
  createWsAlg->setProperty("UnitX", "DeltaE");
  createWsAlg->execute();
  MatrixWorkspace_sptr energyWs = createWsAlg->getProperty("OutputWorkspace");

  IAlgorithm_sptr convertHistAlg = AlgorithmManager::Instance().create("ConvertToHistogram");
  convertHistAlg->setChild(true);
  convertHistAlg->initialize();
  convertHistAlg->setProperty("InputWorkspace", energyWs);
  convertHistAlg->setProperty("OutputWorkspace", "__energy");
  convertHistAlg->execute();
  energyWs = convertHistAlg->getProperty("OutputWorkspace");

  IAlgorithm_sptr loadInstAlg = AlgorithmManager::Instance().create("LoadInstrument");
  loadInstAlg->setChild(true);
  loadInstAlg->initialize();
  loadInstAlg->setProperty("Workspace", energyWs);
  loadInstAlg->setProperty("InstrumentName", instName.toStdString());
  loadInstAlg->setProperty("RewriteSpectraMap", OptionalBool(true));
  loadInstAlg->execute();
  energyWs = loadInstAlg->getProperty("Workspace");

  std::string idfDirectory = Mantid::Kernel::ConfigService::Instance().getString("instrumentDefinition.directory");
  QString ipfFilename =
      QString::fromStdString(idfDirectory) + instName + "_" + analyser + "_" + reflection + "_Parameters.xml";

  IAlgorithm_sptr loadParamAlg = AlgorithmManager::Instance().create("LoadParameterFile");
  loadParamAlg->setChild(true);
  loadParamAlg->initialize();
  loadParamAlg->setProperty("Workspace", energyWs);
  loadParamAlg->setProperty("Filename", ipfFilename.toStdString());
  loadParamAlg->execute();
  energyWs = loadParamAlg->getProperty("Workspace");

  auto spectraMinDbl = energyWs->getInstrument()->getNumberParameter("spectra-min")[0];
  Mantid::specnum_t spectraMin = boost::lexical_cast<Mantid::specnum_t>(spectraMinDbl);

  auto &spectrum = energyWs->getSpectrum(0);
  spectrum.setSpectrumNo(spectraMin);
  spectrum.clearDetectorIDs();
  spectrum.addDetectorID(spectraMin);

  IAlgorithm_sptr convUnitsAlg = AlgorithmManager::Instance().create("ConvertUnits");
  convUnitsAlg->setChild(true);
  convUnitsAlg->initialize();
  convUnitsAlg->setProperty("InputWorkspace", energyWs);
  convUnitsAlg->setProperty("OutputWorkspace", "__tof");
  convUnitsAlg->setProperty("Target", "TOF");
  convUnitsAlg->setProperty("EMode", "Indirect");
  if (auto const efixed = getEFixed(energyWs)) {
    convUnitsAlg->setProperty("EFixed", *efixed);
  }
  convUnitsAlg->execute();
  MatrixWorkspace_sptr tofWs = convUnitsAlg->getProperty("OutputWorkspace");

  const auto &tofData = tofWs->x(0);
  ranges["peak-start-tof"] = tofData[0];
  ranges["peak-end-tof"] = tofData[2];
  ranges["back-start-tof"] = tofData[3];
  ranges["back-end-tof"] = tofData[4];

  return ranges;
}

/**
 * Prevents the loading of data with incorrect naming if passed true
 *
 * @param filter :: true if you want to allow filtering
 */
void DataReductionTab::filterInputData(bool filter) { setFileExtensionsByName(filter); }

/**
 * Workspaces loading from data selectors load the history of that workspace.
 *
 * @param doLoadHistory :: true if you want to load the history
 */
void DataReductionTab::enableLoadHistoryProperty(bool doLoadHistory) { setLoadHistory(doLoadHistory); }

} // namespace MantidQt::CustomInterfaces
