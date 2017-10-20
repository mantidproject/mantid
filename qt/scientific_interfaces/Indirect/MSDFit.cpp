#include "MSDFit.h"
#include "../General/UserInputValidator.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidQtWidgets/Common/RangeSelector.h"

#include <QFileInfo>

#include <qwt_plot.h>
#include <qwt_plot_curve.h>

using namespace Mantid::API;

namespace {
Mantid::Kernel::Logger g_log("MSDFit");
}

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {
MSDFit::MSDFit(QWidget *parent)
    : IndirectDataAnalysisTab(parent), m_msdTree(nullptr), m_parameterValues(),
      m_parameterToProperty() {
  m_uiForm.setupUi(parent);
}

void MSDFit::setup() {
  // Tree Browser
  m_msdTree = new QtTreePropertyBrowser();
  m_uiForm.properties->addWidget(m_msdTree);

  m_msdTree->setFactoryForManager(m_dblManager, m_dblEdFac);

  m_properties["StartX"] = m_dblManager->addProperty("StartX");
  m_dblManager->setDecimals(m_properties["StartX"], NUM_DECIMALS);
  m_properties["EndX"] = m_dblManager->addProperty("EndX");
  m_dblManager->setDecimals(m_properties["EndX"], NUM_DECIMALS);

  m_properties["Gaussian"] = createModel("Gaussian", {"Intensity", "MSD"});
  m_properties["Peters"] = createModel("Peters", {"Intensity", "MSD", "Beta"});
  m_properties["Yi"] = createModel("Yi", {"Intensity", "MSD", "Sigma"});

  auto fitRangeSelector = m_uiForm.ppPlotTop->addRangeSelector("MSDRange");

  modelSelection(m_uiForm.cbModelInput->currentIndex());

  connect(fitRangeSelector, SIGNAL(minValueChanged(double)), this,
          SLOT(minChanged(double)));
  connect(fitRangeSelector, SIGNAL(maxValueChanged(double)), this,
          SLOT(maxChanged(double)));
  connect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this,
          SLOT(updateRS(QtProperty *, double)));

  connect(m_uiForm.dsSampleInput, SIGNAL(dataReady(const QString &)), this,
          SLOT(newDataLoaded(const QString &)));
  connect(m_uiForm.cbModelInput, SIGNAL(currentIndexChanged(int)), this,
          SLOT(modelSelection(int)));
  connect(m_uiForm.pbSingleFit, SIGNAL(clicked()), this, SLOT(singleFit()));

  connect(m_uiForm.spPlotSpectrum, SIGNAL(valueChanged(int)), this,
          SLOT(setSelectedSpectra(int)));
  connect(m_uiForm.spPlotSpectrum, SIGNAL(valueChanged(int)), this,
          SLOT(updateProperties(int)));
  connect(m_uiForm.spPlotSpectrum, SIGNAL(valueChanged(int)), this,
          SLOT(updatePlot()));

  connect(m_uiForm.spSpectraMin, SIGNAL(valueChanged(int)), this,
          SLOT(specMinChanged(int)));
  connect(m_uiForm.spSpectraMax, SIGNAL(valueChanged(int)), this,
          SLOT(specMaxChanged(int)));

  connect(m_uiForm.pbPlot, SIGNAL(clicked()), this, SLOT(plotClicked()));
  connect(m_uiForm.pbSave, SIGNAL(clicked()), this, SLOT(saveClicked()));
  connect(m_uiForm.pbPlotPreview, SIGNAL(clicked()), this,
          SLOT(plotCurrentPreview()));
}

void MSDFit::run() {
  if (!validate())
    return;

  // Set the result workspace for Python script export
  auto model = m_uiForm.cbModelInput->currentText();
  QString dataName = m_uiForm.dsSampleInput->getCurrentDataName();

  int specMin = m_uiForm.spSpectraMin->value();
  int specMax = m_uiForm.spSpectraMax->value();

  m_pythonExportWsName =
      dataName.left(dataName.lastIndexOf("_")).toStdString() + "_s" +
      std::to_string(specMin) + "_to_s" + std::to_string(specMax) + "_" +
      model.toStdString() + "_msd";
  m_parameterToProperty = createParameterToPropertyMap(model);

  IAlgorithm_sptr msdAlg =
      msdFitAlgorithm(modelToAlgorithmProperty(model), specMin, specMax);
  m_batchAlgoRunner->addAlgorithm(msdAlg);

  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
          SLOT(algorithmComplete(bool)));
  m_batchAlgoRunner->executeBatchAsync();
}

void MSDFit::singleFit() {
  if (!validate())
    return;

  // Set the result workspace for Python script export
  auto model = m_uiForm.cbModelInput->currentText();
  QString dataName = m_uiForm.dsSampleInput->getCurrentDataName();
  int fitSpec = m_uiForm.spPlotSpectrum->value();

  m_pythonExportWsName =
      dataName.left(dataName.lastIndexOf("_")).toStdString() + "_s" +
      std::to_string(fitSpec) + "_" + model.toStdString() + "_msd";
  m_parameterToProperty = createParameterToPropertyMap(model);

  IAlgorithm_sptr msdAlg =
      msdFitAlgorithm(modelToAlgorithmProperty(model), fitSpec, fitSpec);
  m_batchAlgoRunner->addAlgorithm(msdAlg);

  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
          SLOT(algorithmComplete(bool)));
  m_batchAlgoRunner->executeBatchAsync();
}

/*
 * Creates an initialized MSDFit Algorithm, using the model with the
 * specified name, to be run from the specified minimum spectrum to
 * the specified maximum spectrum.
 *
 * @param model   The name of the model to be used by the algorithm.
 * @param specMin The minimum spectrum to fit.
 * @param specMax The maximum spectrum to fit.
 * @return        An MSDFit Algorithm using the specified model, which
 *                will run across all spectrum between the specified
 *                minimum and maximum.
 */
IAlgorithm_sptr MSDFit::msdFitAlgorithm(const std::string &model, int specMin,
                                        int specMax) {
  auto wsName = m_uiForm.dsSampleInput->getCurrentDataName().toStdString();
  double xStart = m_dblManager->value(m_properties["StartX"]);
  double xEnd = m_dblManager->value(m_properties["EndX"]);
  setMinimumSpectra(specMin);
  setMaximumSpectra(specMax);

  IAlgorithm_sptr msdAlg = AlgorithmManager::Instance().create("MSDFit");
  msdAlg->initialize();
  msdAlg->setProperty("InputWorkspace", wsName);
  msdAlg->setProperty("Model", model);
  msdAlg->setProperty("XStart", xStart);
  msdAlg->setProperty("XEnd", xEnd);
  msdAlg->setProperty("SpecMin", boost::numeric_cast<long>(specMin));
  msdAlg->setProperty("SpecMax", boost::numeric_cast<long>(specMax));
  msdAlg->setProperty("OutputWorkspace", m_pythonExportWsName);

  return msdAlg;
}

bool MSDFit::validate() {
  UserInputValidator uiv;

  uiv.checkDataSelectorIsValid("Sample input", m_uiForm.dsSampleInput);

  auto range = std::make_pair(m_dblManager->value(m_properties["StartX"]),
                              m_dblManager->value(m_properties["EndX"]));
  uiv.checkValidRange("a range", range);

  int specMin = m_uiForm.spSpectraMin->value();
  int specMax = m_uiForm.spSpectraMax->value();
  auto specRange = std::make_pair(specMin, specMax + 1);
  uiv.checkValidRange("spectrum range", specRange);

  QString errors = uiv.generateErrorMessage();
  showMessageBox(errors);

  return errors.isEmpty();
}

void MSDFit::loadSettings(const QSettings &settings) {
  m_uiForm.dsSampleInput->readSettings(settings.group());
}

/**
 * Handles the completion of the MSDFit algorithm
 *
 * @param error If the algorithm chain failed
 */
void MSDFit::algorithmComplete(bool error) {
  disconnect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
             SLOT(algorithmComplete(bool)));

  if (error)
    return;

  m_parameterValues = IndirectTab::extractParametersFromTable(
      m_pythonExportWsName + "_Parameters",
      m_parameterToProperty.keys().toSet(), minimumSpectra(), maximumSpectra());
  updateProperties(m_uiForm.spPlotSpectrum->value());
  updatePlot();

  // Enable plot and save
  m_uiForm.pbPlot->setEnabled(true);
  m_uiForm.pbSave->setEnabled(true);
}

void MSDFit::updatePlot() {
  const auto groupName = m_pythonExportWsName + "_Workspaces";
  IndirectDataAnalysisTab::updatePlot(groupName, m_uiForm.ppPlotTop,
                                      m_uiForm.ppPlotBottom);
  IndirectDataAnalysisTab::updatePlotRange("MSDRange", m_uiForm.ppPlotTop);
}

/**
 * Called when new data has been loaded by the data selector.
 *
 * Configures ranges for spin boxes before raw plot is done.
 *
 * @param wsName Name of new workspace loaded
 */
void MSDFit::newDataLoaded(const QString wsName) {
  auto workspace =
      Mantid::API::AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
          wsName.toStdString());

  int maxWsIndex = 0;

  if (workspace) {
    maxWsIndex = static_cast<int>(workspace->getNumberHistograms()) - 1;
  }

  setInputWorkspace(workspace);
  setPreviewPlotWorkspace(workspace);
  m_parameterValues.clear();
  m_parameterToProperty.clear();
  m_pythonExportWsName = "";

  m_uiForm.spPlotSpectrum->setMaximum(maxWsIndex);
  m_uiForm.spPlotSpectrum->setMinimum(0);
  m_uiForm.spPlotSpectrum->setValue(0);

  m_uiForm.spSpectraMin->setMaximum(maxWsIndex);
  m_uiForm.spSpectraMin->setMinimum(0);

  m_uiForm.spSpectraMax->setMaximum(maxWsIndex);
  m_uiForm.spSpectraMax->setMinimum(0);
  m_uiForm.spSpectraMax->setValue(maxWsIndex);

  updatePlot();
}

/**
 * Handles the user entering a new minimum spectrum index.
 *
 * Prevents the user entering an overlapping spectra range.
 *
 * @param value Minimum spectrum index
 */
void MSDFit::specMinChanged(int value) {
  m_uiForm.spSpectraMax->setMinimum(value);
}

/**
 * Handles the user entering a new maximum spectrum index.
 *
 * Prevents the user entering an overlapping spectra range.
 *
 * @param value Maximum spectrum index
 */
void MSDFit::specMaxChanged(int value) {
  m_uiForm.spSpectraMin->setMaximum(value);
}

void MSDFit::minChanged(double val) {
  m_dblManager->setValue(m_properties["StartX"], val);
}

void MSDFit::maxChanged(double val) {
  m_dblManager->setValue(m_properties["EndX"], val);
}

void MSDFit::updateRS(QtProperty *prop, double val) {
  auto fitRangeSelector = m_uiForm.ppPlotTop->getRangeSelector("MSDRange");

  if (prop == m_properties["StartX"])
    fitRangeSelector->setMinimum(val);
  else if (prop == m_properties["EndX"])
    fitRangeSelector->setMaximum(val);
}

/*
 * Creates a property representing a model with the specified name,
 * and which takes the specified parameters.
 *
 * @param modelName       The name of the model.
 * @param modelParameters The parameters taken by the model.
 * @return                The created property, which represents the
 *                        model in the property table.
 */
QtProperty *MSDFit::createModel(const QString &modelName,
                                const std::vector<QString> &modelParameters) {
  QtProperty *expGroup = m_grpManager->addProperty(modelName);

  for (auto &modelParam : modelParameters) {
    QString paramName = modelName + "." + modelParam;
    m_properties[paramName] = m_dblManager->addProperty(modelParam);
    m_dblManager->setDecimals(m_properties[paramName], NUM_DECIMALS);
    expGroup->addSubProperty(m_properties[paramName]);
  }

  return expGroup;
}

void MSDFit::modelSelection(int selected) {
  QString model = m_uiForm.cbModelInput->itemText(selected);
  m_msdTree->clear();

  m_msdTree->addProperty(m_properties["StartX"]);
  m_msdTree->addProperty(m_properties["EndX"]);
  m_msdTree->addProperty(m_properties[model]);
}

/*
 * Creates a map from the names of the specified model's parameters,
 * to the name of the property in the property table, associated with
 * this parameter.
 *
 * @param model The model whose parameters to create a map from.
 * @return      A map from the model parameter names to the names
 *              of their associated properties in the property table.
 */
QHash<QString, QString>
MSDFit::createParameterToPropertyMap(const QString &model) {
  QHash<QString, QString> parameterToProperty;
  parameterToProperty["Height"] = model + ".Intensity";
  parameterToProperty["MSD"] = model + ".MSD";

  if (model == "Peters")
    parameterToProperty["Beta"] = model + ".Beta";
  else if (model == "Yi") {
    parameterToProperty["Sigma"] = model + ".Sigma";
  }
  return parameterToProperty;
}

/*
 * Given the selected model in the interface, returns the name of
 * the associated model to pass to the MSDFit algorithm.
 *
 * @param model The name of the model as displayed in the interface.
 * @return      The name of the model as defined in the MSDFit algorithm.
 */
std::string MSDFit::modelToAlgorithmProperty(const QString &model) {

  if (model == "Gaussian")
    return "Gauss";
  else if (model == "Peters")
    return "Peters";
  else if (model == "Yi")
    return "Yi";
  else
    return "";
}

/*
 * Updates the property table using the parameter results for the
 * specified spectrum.
 *
 * @param specNo  The spectrum number of the parameters to update
 *                the property table with.
 */
void MSDFit::updateProperties(int specNo) {
  size_t index = boost::numeric_cast<size_t>(specNo);
  auto parameterNames = m_parameterValues.keys();

  if (parameterNames.isEmpty()) {
    return;
  }

  // Check whether parameter values exist for the specified spectrum number
  if (m_parameterValues[parameterNames[0]].contains(index)) {

    for (auto &paramName : parameterNames) {
      auto propertyName = m_parameterToProperty[paramName];
      m_dblManager->setValue(m_properties[propertyName],
                             m_parameterValues[paramName][index]);
    }
  }
}

/**
 * Handles saving of workspace
 */
void MSDFit::saveClicked() {

  if (checkADSForPlotSaveWorkspace(m_pythonExportWsName, false))
    addSaveWorkspaceToQueue(QString::fromStdString(m_pythonExportWsName));
  m_batchAlgoRunner->executeBatchAsync();
}

/**
 * Handles mantid plotting
 */
void MSDFit::plotClicked() {
  auto wsName = m_pythonExportWsName + "_Workspaces";
  if (checkADSForPlotSaveWorkspace(wsName, true)) {
    // Get the workspace
    auto groupWs =
        AnalysisDataService::Instance().retrieveWS<const WorkspaceGroup>(
            wsName);
    auto groupWsNames = groupWs->getNames();

    if (groupWsNames.size() != 1)
      plotSpectrum(QString::fromStdString(m_pythonExportWsName), 1);
    else
      plotSpectrum(QString::fromStdString(wsName), 0, 2);
  }
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
