#include "MantidQtCustomInterfaces/Indirect/Stretch.h"
#include "MantidQtCustomInterfaces/UserInputValidator.h"

namespace {
Mantid::Kernel::Logger g_log("Stretch");
}

namespace MantidQt {
namespace CustomInterfaces {
Stretch::Stretch(QWidget *parent) : IndirectBayesTab(parent) {
  m_uiForm.setupUi(parent);

  // Create range selector
  auto eRangeSelector = m_uiForm.ppPlot->addRangeSelector("StretchERange");
  connect(eRangeSelector, SIGNAL(minValueChanged(double)), this,
          SLOT(minValueChanged(double)));
  connect(eRangeSelector, SIGNAL(maxValueChanged(double)), this,
          SLOT(maxValueChanged(double)));

  // Add the properties browser to the ui form
  m_uiForm.treeSpace->addWidget(m_propTree);

  m_properties["EMin"] = m_dblManager->addProperty("EMin");
  m_properties["EMax"] = m_dblManager->addProperty("EMax");
  m_properties["SampleBinning"] = m_dblManager->addProperty("Sample Binning");
  m_properties["Sigma"] = m_dblManager->addProperty("Sigma");
  m_properties["Beta"] = m_dblManager->addProperty("Beta");

  m_dblManager->setDecimals(m_properties["EMin"], NUM_DECIMALS);
  m_dblManager->setDecimals(m_properties["EMax"], NUM_DECIMALS);
  m_dblManager->setDecimals(m_properties["SampleBinning"], INT_DECIMALS);
  m_dblManager->setDecimals(m_properties["Sigma"], INT_DECIMALS);
  m_dblManager->setDecimals(m_properties["Beta"], INT_DECIMALS);

  m_propTree->addProperty(m_properties["EMin"]);
  m_propTree->addProperty(m_properties["EMax"]);
  m_propTree->addProperty(m_properties["SampleBinning"]);
  m_propTree->addProperty(m_properties["Sigma"]);
  m_propTree->addProperty(m_properties["Beta"]);

  // default values
  m_dblManager->setValue(m_properties["Sigma"], 50);
  m_dblManager->setMinimum(m_properties["Sigma"], 1);
  m_dblManager->setMaximum(m_properties["Sigma"], 200);
  m_dblManager->setValue(m_properties["Beta"], 50);
  m_dblManager->setMinimum(m_properties["Beta"], 1);
  m_dblManager->setMaximum(m_properties["Beta"], 200);
  m_dblManager->setValue(m_properties["SampleBinning"], 1);
  m_dblManager->setMinimum(m_properties["SampleBinning"], 1);

  // Connect the data selector for the sample to the mini plot
  connect(m_uiForm.dsSample, SIGNAL(dataReady(const QString &)), this,
          SLOT(handleSampleInputReady(const QString &)));
  connect(m_uiForm.chkSequentialFit, SIGNAL(toggled(bool)), m_uiForm.cbPlot,
          SLOT(setEnabled(bool)));
}

void Stretch::setup() {}

/**
 * Validate the form to check the program can be run
 *
 * @return :: Whether the form was valid
 */
bool Stretch::validate() {
  UserInputValidator uiv;
  uiv.checkDataSelectorIsValid("Sample", m_uiForm.dsSample);
  uiv.checkDataSelectorIsValid("Resolution", m_uiForm.dsResolution);

  QString errors = uiv.generateErrorMessage();
  if (!errors.isEmpty()) {
    emit showMessageBox(errors);
    return false;
  }

  return true;
}

/**
 * Collect the settings on the GUI and build a python
 * script that runs Stretch
 */
void Stretch::run() {
  using namespace Mantid::API;

  // Workspace input
  const auto sampleName = m_uiForm.dsSample->getCurrentDataName().toStdString();
  const auto resName = m_uiForm.dsResolution->getCurrentDataName().toStdString();

  // Collect input from options section
  const auto background = m_uiForm.cbBackground->currentText().toStdString();
  const auto plot = m_uiForm.cbPlot->currentText().toStdString();

  // Collect input from the properties browser
  const auto eMin =  m_properties["EMin"]->valueText().toDouble();
  const auto eMax =  m_properties["EMax"]->valueText().toDouble();
  const auto beta =  m_properties["Beta"]->valueText().toLong();
  const auto sigma = m_properties["Sigma"]->valueText().toLong();
  const auto nBins = m_properties["SampleBinning"]->valueText().toLong();

  // Bool options
  const auto save = m_uiForm.chkSave->isChecked();
  const auto elasticPeak = m_uiForm.chkElasticPeak->isChecked();
  const auto sequence = m_uiForm.chkSequentialFit->isChecked();


  // Construct OutputNames
  auto cutIndex = sampleName.find_last_of("_");
  auto baseName = sampleName.substr(0, cutIndex);
  auto fitWsName = baseName + "_Qst_Fit";
  auto contourWsName = baseName + "_Qst_Contour";

  auto stretch = AlgorithmManager::Instance().create("BayesStretch");
  stretch->initialize();
  stretch->setProperty("SampleWorkspace", sampleName);
  stretch->setProperty("ResolutionWorkspace", resName);
  stretch->setProperty("MinRange", eMin);
  stretch->setProperty("MaxRange", eMax);
  stretch->setProperty("SampleBins", nBins);
  stretch->setProperty("Elastic", elasticPeak);
  stretch->setProperty("Background", background);
  stretch->setProperty("NumberSigma", sigma);
  stretch->setProperty("NumberBeta", beta);
  stretch->setProperty("Loop", sequence);
  stretch->setProperty("Plot", plot);
  stretch->setProperty("Save", save);
  stretch->setProperty("OutputWorkspaceFit", fitWsName);
  stretch->setProperty("OutputWorkspaceContour", contourWsName);

  m_StretchAlg = stretch;
  m_batchAlgoRunner->addAlgorithm(stretch);
  m_batchAlgoRunner->executeBatchAsync();

}

/**
 * Set the data selectors to use the default save directory
 * when browsing for input files.
 *
 * @param settings :: The current settings
 */
void Stretch::loadSettings(const QSettings &settings) {
  m_uiForm.dsSample->readSettings(settings.group());
  m_uiForm.dsResolution->readSettings(settings.group());
}

/**
 * Plots the loaded file to the miniplot and sets the guides
 * and the range
 *
 * @param filename :: The name of the workspace to plot
 */
void Stretch::handleSampleInputReady(const QString &filename) {
  m_uiForm.ppPlot->addSpectrum("Sample", filename, 0);
  QPair<double, double> range = m_uiForm.ppPlot->getCurveRange("Sample");
  auto eRangeSelector = m_uiForm.ppPlot->getRangeSelector("StretchERange");
  setRangeSelector(eRangeSelector, m_properties["EMin"], m_properties["EMax"],
                   range);
  setPlotPropertyRange(eRangeSelector, m_properties["EMin"],
                       m_properties["EMax"], range);
}

/**
 * Updates the property manager when the lower guide is moved on the mini plot
 *
 * @param min :: The new value of the lower guide
 */
void Stretch::minValueChanged(double min) {
  m_dblManager->setValue(m_properties["EMin"], min);
}

/**
 * Updates the property manager when the upper guide is moved on the mini plot
 *
 * @param max :: The new value of the upper guide
 */
void Stretch::maxValueChanged(double max) {
  m_dblManager->setValue(m_properties["EMax"], max);
}

/**
 * Handles when properties in the property manager are updated.
 *
 * @param prop :: The property being updated
 * @param val :: The new value for the property
 */
void Stretch::updateProperties(QtProperty *prop, double val) {
  UNUSED_ARG(val);

  auto eRangeSelector = m_uiForm.ppPlot->getRangeSelector("StretchERange");

  if (prop == m_properties["EMin"] || prop == m_properties["EMax"]) {
    auto bounds = qMakePair(m_dblManager->value(m_properties["EMin"]),
                            m_dblManager->value(m_properties["EMax"]));
    setRangeSelector(eRangeSelector, m_properties["EMin"], m_properties["EMax"],
                     bounds);
  }
}

} // namespace CustomInterfaces
} // namespace MantidQt
